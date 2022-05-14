#pragma once

#include <cstddef>
#include <string>

#include <asio/io_context.hpp>
#include <asio/ssl.hpp>

namespace gem
{
	class GeminiClient
	{
	public:
		using CallbackType = std::function<void(bool success, std::string code, std::string mime, std::shared_ptr<std::string> body)>;

		void connectAsync(std::string url, size_t port = 1965, const CallbackType &callback = nullptr);

		static void poll();

	private:
		static asio::io_context _ioContext;
		static asio::ssl::context _sslContext;
	};
}