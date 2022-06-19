#include "GeminiClient.hpp"
#include "Utilities.hpp"

#include <charconv>

#include <asio/buffer.hpp>
#include <asio/connect.hpp>
#include <asio/read.hpp>
#include <asio/read_until.hpp>
#include <asio/streambuf.hpp>
#include <asio/write.hpp>

using namespace gem;

namespace
{
	static inline bool checkErrorCode(const asio::error_code &ec, std::string_view failMessage = "", bool eofIsError = true)
	{
		if (!ec || (!eofIsError && ec == asio::error::eof))
		{
			return true;
		}

		const std::string message = ec.message();
		fprintf(stderr, "%s: %s\n", failMessage.data(), message.c_str());

		return false;
	}

	static void parseHeader(std::string_view header, const GeminiClient::ResponseHeaderCallback &callback)
	{
		if (!header.empty())
		{
			int code;
			std::string meta;
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

			if (result.ec != std::errc::invalid_argument)
			{
				switch (code)
				{
					case 10:
					case 11:
					case 20:
					case 30:
					case 31:
					case 40:
					case 41:
					case 42:
					case 43:
					case 44:
					case 50:
					case 51:
					case 52:
					case 53:
					case 59:
					case 60:
					case 61:
					case 62:
						callback(GeminiClient::ClientCode::SUCCESS, static_cast<StatusCode>(code), meta);
						return;
					default:
						break;
				}
			}
		}

		fprintf(stderr, "Malformed header: \"%s\"\n", header.data());

		callback(GeminiClient::ClientCode::RESPONSE_HEADER_ERROR, StatusCode::NONE, "");
	}

	static void sendRequestAsync(asio::ssl::stream<asio::ip::tcp::socket> *socket, const std::string &url, const GeminiClient::ConnectionCallback &callback)
	{
		std::string request = url + "\r\n";

		asio::async_write(*socket, asio::buffer(request),
			[callback](const std::error_code &ec, std::size_t)
			{
				if (checkErrorCode(ec, "Request failed"))
				{
					callback(GeminiClient::ClientCode::SUCCESS);
				}
				else
				{
					callback(GeminiClient::ClientCode::REQUEST_ERROR);
				}
			}
		);
	}

	static void handshakeAsync(asio::ssl::stream<asio::ip::tcp::socket> *socket, const std::string &url, const GeminiClient::ConnectionCallback &callback)
	{
		socket->async_handshake(asio::ssl::stream_base::client,
			[socket, url, callback](const std::error_code &ec)
			{
				if (checkErrorCode(ec, "TLS handshake failed"))
				{
					sendRequestAsync(socket, url, callback);
				}
				else
				{
					callback(GeminiClient::ClientCode::TLS_HANDSHAKE_ERROR);
				}
			}
		);
	}

	static void connectAsync(asio::ssl::stream<asio::ip::tcp::socket> *socket, const std::string &url, const asio::ip::tcp::resolver::results_type &endpoints, const GeminiClient::ConnectionCallback &callback)
	{
		asio::async_connect(socket->next_layer(), endpoints,
			[socket, url, callback](const std::error_code &ec, const asio::ip::tcp::endpoint &)
			{
				if (checkErrorCode(ec, "Connection failed"))
				{
					handshakeAsync(socket, url, callback);
				}
				else
				{
					callback(GeminiClient::ClientCode::CONNECTION_ERROR);
				}
			}
		);
	}

	static void resolveAsync(asio::io_context &ioContext, asio::ssl::stream<asio::ip::tcp::socket> *socket, const std::string &url, size_t port, const GeminiClient::ConnectionCallback &callback)
	{
		auto resolver = std::make_shared<asio::ip::tcp::resolver>(ioContext);
		std::string_view hostName = extractHostName(url);

		resolver->async_resolve(hostName, std::to_string(port),
			[socket, url, resolver, callback](const std::error_code &ec, const asio::ip::tcp::resolver::results_type &endpoints)
			{
				if (checkErrorCode(ec, "Host name resolution failed"))
				{
					connectAsync(socket, url, endpoints, callback);
				}
				else
				{
					callback(GeminiClient::ClientCode::HOST_NAME_RESOLUTION_ERROR);
				}
			}
		);
	}

	static bool verifyCertificate([[maybe_unused]] bool preverified, asio::ssl::verify_context &context)
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

void GeminiClient::connectAsync(const ConnectionCallback &callback, std::string url, size_t port /*= 1965*/)
{
	delete _socket;
	_socket = new asio::ssl::stream<asio::ip::tcp::socket>(_ioContext, _sslContext);

	resolveAsync(_ioContext, _socket, url, port, callback);
}

void GeminiClient::receiveResponseHeaderAsync(const ResponseHeaderCallback &callback)
{
	auto buffer = new std::string();

	asio::async_read_until(*_socket, asio::dynamic_buffer(*buffer), "\r\n",
		[buffer, callback](const std::error_code &ec, std::size_t size)
		{
			if (checkErrorCode(ec, "Receiving response header failed"))
			{
				parseHeader(std::string_view(buffer->data(), size), callback);
			}
			else
			{
				callback(ClientCode::RESPONSE_HEADER_ERROR, StatusCode::NONE, "");
			}

			delete buffer;
		}
	);
}

void GeminiClient::receiveResponseBodyAsync(const ResponseBodyCallback &callback)
{
	auto buffer = std::make_shared<std::vector<char>>();

	// capture socket to prolong its life
	asio::async_read(*_socket, asio::dynamic_buffer(*buffer),
		[buffer, callback](const std::error_code &ec, std::size_t)
		{
			if (checkErrorCode(ec, "Receiving response body failed", false))
			{
				callback(GeminiClient::ClientCode::SUCCESS, buffer);
			}
			else
			{
				callback(GeminiClient::ClientCode::RESPONSE_BODY_ERROR, nullptr);
			}
		}
	);
}

void GeminiClient::poll()
{
	_ioContext.poll();
	_ioContext.restart();
}