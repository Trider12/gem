#include "GeminiClient.hpp"
#include "Utilities.hpp"

#include <charconv>

#include <asio/buffer.hpp>
#include <asio/connect.hpp>
#include <asio/read.hpp>
#include <asio/read_until.hpp>
#include <asio/streambuf.hpp>
#include <asio/write.hpp>
#include <asio/ip/tcp.hpp>

using namespace gem;

namespace
{
	static inline bool checkErrorCode(const asio::error_code &ec, [[maybe_unused]] std::string_view successMessage = "", std::string_view failMessage = "", bool eofIsError = true)
	{
		if (!ec || (!eofIsError && ec == asio::error::eof))
		{
			//puts(successMessage.data());
			return true;
		}

		const std::string message = ec.message();
		fprintf(stderr, "%s: %s\n", failMessage.data(), message.c_str());

		return false;
	}

	using SocketPtr = std::shared_ptr<asio::ssl::stream<asio::ip::tcp::socket>>;

	static void receiveBodyAsync(SocketPtr socket, int code, const std::string &meta, const GeminiClient::CallbackType &callback)
	{
		auto buffer = std::make_shared<std::string>();

		// capture socket to prolong its life
		asio::async_read(*socket, asio::dynamic_buffer(*buffer),
			[socket, buffer, code, meta, callback](const std::error_code &ec, std::size_t)
			{
				if (checkErrorCode(ec, "Receiving Body successful", "Receiving Body failed", false))
				{
					if (callback)
					{
						callback(code, meta, buffer);
					}
				}
				else if (callback)
				{
					callback(code, meta, nullptr);
				}
			}
		);
	}

	static void parseHeader(SocketPtr socket, std::string_view header, const GeminiClient::CallbackType &callback)
	{
		int code = 5;
		std::string meta;

		if (!header.empty())
		{
			std::from_chars_result result;

			if (size_t spacePos = header.find(' '); spacePos != std::string::npos)
			{
				result = std::from_chars(header.data(), header.data() + spacePos, code);
				meta = header.substr(spacePos + 1);
			}
			else
			{
				result = std::from_chars(header.data(), header.data() + header.size(), code);
			}

			assert(result.ec != std::errc::invalid_argument);

			// TODO: return codes handling

			switch (code)
			{
				case 20:
					if (stringStartsWith(meta, "text/"))
					{
						receiveBodyAsync(socket, code, meta, callback);
						return;
					}
					break;
				case 10:
				case 11:
				case 30:
				case 31:
				case 40:
				case 41:
				case 42:
				case 43:
				case 44:
				case 51:
				case 52:
				case 53:
				case 59:
				case 60:
				case 61:
				case 62:
					break;
				default:
					assert(false);
					break;
			}
		}

		fprintf(stderr, "Malformed header: \"%s\"\n", header.data());

		if (callback)
		{
			callback(code, std::string(meta), nullptr);
		}
	}

	static void receiveHeaderAsync(SocketPtr socket, const GeminiClient::CallbackType &callback)
	{
		auto buffer = std::make_shared<std::string>();

		asio::async_read_until(*socket, asio::dynamic_buffer(*buffer), "\r\n",
			[socket, buffer, callback](const std::error_code &ec, std::size_t size)
			{
				if (checkErrorCode(ec, "Receiving Header successful", "Receiving Header failed"))
				{
					parseHeader(socket, std::string_view(buffer->data(), size), callback);
				}
				else if (callback)
				{
					callback(5, "", nullptr);
				}
			}
		);
	}

	static void sendRequestAsync(SocketPtr socket, const std::string &url, const GeminiClient::CallbackType &callback)
	{
		std::string request = url + "\r\n";

		asio::async_write(*socket, asio::buffer(request),
			[socket, callback](const std::error_code &ec, std::size_t)
			{
				if (checkErrorCode(ec, "Write successful", "Write failed"))
				{
					receiveHeaderAsync(socket, callback);
				}
				else if (callback)
				{
					callback(4, "", nullptr);
				}
			}
		);
	}

	static void handshakeAsync(SocketPtr socket, const std::string &url, const GeminiClient::CallbackType &callback)
	{
		socket->async_handshake(asio::ssl::stream_base::client,
			[socket, url, callback](const std::error_code &ec)
			{
				if (checkErrorCode(ec, "Handshake successful", "Handshake failed"))
				{
					sendRequestAsync(socket, url, callback);
				}
				else if (callback)
				{
					callback(3, "", nullptr);
				}
			}
		);
	}

	static void connectAsync(SocketPtr socket, const std::string &url, const asio::ip::tcp::resolver::results_type &endpoints, const GeminiClient::CallbackType &callback)
	{
		asio::async_connect(socket->next_layer(), endpoints,
			[socket, url, callback](const std::error_code &ec, const asio::ip::tcp::endpoint &)
			{
				if (checkErrorCode(ec, "Connect successful", "Connect failed"))
				{
					handshakeAsync(socket, url, callback);
				}
				else if (callback)
				{
					callback(2, "", nullptr);
				}
			}
		);
	}

	static void resolveAsync(asio::io_context &ioContext, SocketPtr socket, const std::string &url, size_t port, const GeminiClient::CallbackType &callback)
	{
		auto resolver = std::make_shared<asio::ip::tcp::resolver>(ioContext);
		std::string_view hostName = extractHostName(url);

		resolver->async_resolve(hostName, std::to_string(port),
			[socket, url, resolver, callback](const std::error_code &ec, const asio::ip::tcp::resolver::results_type &endpoints)
			{
				if (checkErrorCode(ec, "Resolve successful", "Resolve failed"))
				{
					connectAsync(socket, url, endpoints, callback);
				}
				else if (callback)
				{
					callback(1, "", nullptr);
				}
			}
		);
	}

	static bool verifyCertificate(bool preverified, asio::ssl::verify_context &context)
	{
		char name[256];
		X509 *cert = X509_STORE_CTX_get_current_cert(context.native_handle());
		X509_NAME_oneline(X509_get_subject_name(cert), name, 256);
		printf("Verifying %s\n", name);

		return true;
	}

	static inline asio::ssl::context createSslContext()
	{
		asio::ssl::context context(asio::ssl::context::tlsv12_client);
		context.set_options(asio::ssl::context::default_workarounds);
		context.set_default_verify_paths();
		context.set_verify_mode(asio::ssl::context::verify_none);
		context.set_verify_callback(verifyCertificate);
		context.use_certificate_file("assets/certificates/gem.crt", asio::ssl::context_base::file_format::pem);
		context.use_private_key_file("assets/certificates/gem.key", asio::ssl::context_base::file_format::pem);

		return context;
	}
}

asio::io_context GeminiClient::_ioContext;
asio::ssl::context GeminiClient::_sslContext = createSslContext();

void GeminiClient::connectAsync(std::string url, size_t port /*= 1965*/, const CallbackType &callback /*= nullptr*/)
{
	auto socket = std::make_shared<asio::ssl::stream<asio::ip::tcp::socket>>(_ioContext, _sslContext);
	resolveAsync(_ioContext, socket, url, port, callback);
}

void GeminiClient::poll()
{
	_ioContext.poll();
	_ioContext.restart();
}