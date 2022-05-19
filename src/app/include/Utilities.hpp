#pragma once

#include <string_view>
#include <sstream>
#include <vector>
#include <cassert>

namespace gem
{
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

	static inline std::string_view extractHostName(std::string_view url)
	{
		size_t p1 = url.find("//");
		assert(p1 != std::string::npos);
		p1 += 2;

		if (size_t p2 = url.find('/', p1); p2 < url.size())
		{
			return url.substr(p1, p2 - p1);
		}

		return url.substr(p1);
	}
}