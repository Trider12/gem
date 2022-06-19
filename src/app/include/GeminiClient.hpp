#pragma once

#include "StatusCode.hpp"

#include <cstddef>
#include <string>
#include <memory>

#include <asio/io_context.hpp>
#include <asio/ssl.hpp>
#include <asio/ip/tcp.hpp>

namespace gem
{
	class GeminiClient
	{
	public:
		enum class ClientCode
		{
			SUCCESS = 0,
			HOST_NAME_RESOLUTION_ERROR = 1,
			CONNECTION_ERROR = 2,
			TLS_HANDSHAKE_ERROR = 3,
			REQUEST_ERROR = 4,
			RESPONSE_HEADER_ERROR = 5,
			RESPONSE_BODY_ERROR = 6,
			RESPONSE_HEADER_MALFORMED = 7
		};

		using ConnectionCallback = std::function<void(ClientCode clientCode)>;
		using ResponseHeaderCallback = std::function<void(ClientCode clientCode, StatusCode statusCode, std::string meta)>;
		using ResponseBodyCallback = std::function<void(ClientCode clientCode, std::shared_ptr<std::vector<char>> data)>;

		void connectAsync(const ConnectionCallback &callback, std::string url, size_t port = 1965);
		void receiveResponseHeaderAsync(const ResponseHeaderCallback &callback);
		void receiveResponseBodyAsync(const ResponseBodyCallback &callback);

		static void poll();

	private:
		asio::ssl::stream<asio::ip::tcp::socket> *_socket {nullptr};

		static asio::io_context _ioContext;
		static asio::ssl::context _sslContext;
	};
}