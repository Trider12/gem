#include "GeminiClient.hpp"

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
	static inline bool checkErrorCode(const std::error_code &ec, std::string_view successMessage, std::string_view failMessage, bool eofIsError = true)
	{
		if (!ec || (!eofIsError && ec == asio::error::eof))
		{
			printf("%s\n", successMessage.data());
			return true;
		}

		const std::string message = ec.message();
		fprintf(stderr, "%s: %s\n", failMessage.data(), message.c_str());

		return false;
	}

	static inline std::vector<std::string> splitString(const char *str, char delim = ' ')
	{
		std::istringstream iss(str);
		std::vector<std::string> strings;

		for (std::string string; std::getline(iss, string, delim); strings.push_back(string));

		return strings;
	}

	using socketPtr = std::shared_ptr<asio::ssl::stream<asio::ip::tcp::socket>>;

	static void receiveBodyAsync(socketPtr socket, std::string_view mime, const GeminiClient::CallbackType &callback)
	{
		auto buffer = new asio::streambuf();

		// capture socket to prolong its life
		asio::async_read(*socket, *buffer,
			[socket, buffer, mime, callback](const std::error_code &ec, std::size_t size)
			{
				if (checkErrorCode(ec, "Receiving Body successful", "Receiving Body failed", false))
				{
					const char *body = static_cast<const char *>(buffer->data().data());

					if (callback)
					{
						callback(true, mime, {body, size});
					}
				}
				else if (callback)
				{
					callback(false, mime, "");
				}

				delete buffer;
			}
		);
	}

	static void parseHeader(socketPtr socket, const char *header, size_t headerSize, const GeminiClient::CallbackType &callback)
	{
		if (header == nullptr || headerSize <= 0)
		{
			return;
		}

		// TODO: return codes handling

		switch (header[0])
		{
			case '1':
				break;
			case '2':
			{
				std::vector<std::string> strings = splitString(header);
				const std::string &mime = strings[1];

				if (mime.rfind("text/", 0) == 0)
				{
					receiveBodyAsync(socket, mime, callback);
				}
				else if (callback)
				{
					callback(false, mime, "");
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

			default:
				fprintf(stderr, "Malformed header :%s\n", header);
				break;
		}
	}

	static void receiveHeaderAsync(socketPtr socket, const GeminiClient::CallbackType &callback)
	{
		auto *buffer = new asio::streambuf(2048);

		asio::async_read_until(*socket, *buffer, "\r\n",
			[socket, buffer, callback](const std::error_code &ec, std::size_t size)
			{
				if (checkErrorCode(ec, "Receiving Header successful", "Receiving Header failed"))
				{
					const char *header = static_cast<const char *>(buffer->data().data());
					parseHeader(socket, header, size, callback);
				}
				else if (callback)
				{
					callback(false, "", "");
				}

				delete buffer;
			}
		);
	}

	static void sendRequestAsync(socketPtr socket, const std::string &url, const GeminiClient::CallbackType &callback)
	{
		std::string request = "gemini://" + url + "/\r\n";

		asio::async_write(*socket, asio::buffer(request.data(), request.size()),
			[socket, callback](const std::error_code &ec, std::size_t)
			{
				if (checkErrorCode(ec, "Write successful", "Write failed"))
				{
					receiveHeaderAsync(socket, callback);
				}
				else if (callback)
				{
					callback(false, "", "");
				}
			}
		);
	}

	static void handshakeAsync(socketPtr socket, const std::string &url, const GeminiClient::CallbackType &callback)
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
					callback(false, "", "");
				}
			}
		);
	}

	static void connectAsync(socketPtr socket, const std::string &url, const asio::ip::tcp::resolver::results_type &endpoints, const GeminiClient::CallbackType &callback)
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
					callback(false, "", "");
				}
			}
		);
	}

	static void resolveAsync(asio::io_context &ioContext, socketPtr socket, const std::string &url, size_t port, const GeminiClient::CallbackType &callback)
	{
		asio::ip::tcp::resolver *resolver = new asio::ip::tcp::resolver(ioContext);

		resolver->async_resolve(url, std::to_string(port),
			[socket, url, resolver, callback](const std::error_code &ec, const asio::ip::tcp::resolver::results_type &endpoints)
			{
				if (checkErrorCode(ec, "Resolve successful", "Resolve failed"))
				{
					connectAsync(socket, url, endpoints, callback);
				}
				else if (callback)
				{
					callback(false, "", "");
				}

				delete resolver;
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