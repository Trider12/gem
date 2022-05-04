#pragma once

#include <string>

namespace gem
{
	struct Tab
	{
		std::string name;
		std::string url;
		std::string content;

		bool isOpen {true};
	};
}