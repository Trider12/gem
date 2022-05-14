#pragma once

#include <string_view>
#include <sstream>
#include <vector>

#include <asio/error_code.hpp>

namespace gem
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

	static inline void stringTrimLeft(std::string &str) {
		str.erase(str.begin(), std::find_if(str.begin(), str.end(),
			[](unsigned char ch) {
				return !std::isspace(ch);
			}
		));
	}

	static inline void stringTrimRight(std::string &str) {
		str.erase(std::find_if(str.rbegin(), str.rend(),
			[](unsigned char ch) {
				return !std::isspace(ch);
			}
		).base(), str.end());
	}

	static inline void stringTrim(std::string &str) {
		stringTrimLeft(str);
		stringTrimRight(str);
	}

	static inline std::vector<std::string> stringSplit(const std::string &str, char delim = ' ')
	{
		std::istringstream iss(str);
		std::vector<std::string> strings;

		for (std::string token; std::getline(iss, token, delim); strings.push_back(token));

		return strings;
	}

	static inline bool stringStartsWith(std::string_view str, std::string_view prefix)
	{
		return str.rfind(prefix, 0) == 0;
	}
}