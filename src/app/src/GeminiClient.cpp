#include "GeminiClient.hpp"
#include "Utilities.hpp"

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
	static inline std::string_view extractHostName(std::string_view url)
	{
		size_t p1 = url.find_first_of("//") + 2;
		size_t p2 = url.find_first_of("/", p1);

		return url.substr(p1, p2 - p1);
	}

	using SocketPtr = std::shared_ptr<asio::ssl::stream<asio::ip::tcp::socket>>;

	static void receiveBodyAsync(SocketPtr socket, const std::string &code, const std::string &mime, const GeminiClient::CallbackType &callback)
	{
		auto buffer = std::make_shared<std::string>();

		// capture socket to prolong its life
		asio::async_read(*socket, asio::dynamic_buffer(*buffer),
			[socket, buffer, code, mime, callback](const std::error_code &ec, std::size_t)
			{
				if (checkErrorCode(ec, "Receiving Body successful", "Receiving Body failed", false))
				{
					if (callback)
					{
						callback(true, code, mime, buffer);
					}
				}
				else if (callback)
				{
					callback(false, code, mime, nullptr);
				}
			}
		);
	}

	static void parseHeader(SocketPtr socket, const char *header, size_t headerSize, const GeminiClient::CallbackType &callback)
	{
		const char *code = "";
		const char *mime = "";

		if (header != nullptr && headerSize > 0)
		{
			std::vector<std::string> strings = stringSplit(header);

			assert(strings.size() > 0);
			assert(strings[0].size() > 0);
			code = strings[0].c_str();

			// TODO: return codes handling

			switch (code[0])
			{
				case '1':
					break;
				case '2':
				{
					assert(strings.size() > 1);

					mime = strings[1].c_str();

					if (stringStartsWith(mime, "text/"))
					{
						receiveBodyAsync(socket, code, mime, callback);
						return;
					}

					break;
				}
				case '3':
					break;
				case '4':
					break;
				case '5':
					break;
				case '6':
					break;
			}
		}

		fprintf(stderr, "Malformed header :%s\n", header);

		if (callback)
		{
			callback(false, code, mime, nullptr);
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
					parseHeader(socket, buffer->data(), size, callback);
				}
				else if (callback)
				{
					callback(false, "", "", nullptr);
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
					callback(false, "", "", nullptr);
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
					callback(false, "", "", nullptr);
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
					callback(false, "", "", nullptr);
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
					callback(false, "", "", nullptr);
				}
			}
		);
	}

	static inline asio::ssl::context createSslContext()
	{
		asio::ssl::context context(asio::ssl::context::sslv23);
		context.set_default_verify_paths();
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