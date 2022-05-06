#pragma once

#include <string>

namespace gem
{
	struct Tab
	{
		std::string url;
		std::string label;
		std::string content;

		bool isOpen {true};
	};
}