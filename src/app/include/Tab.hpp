#pragma once

#include <string>
#include <vector>
#include <memory>

namespace gem
{
	enum class LineType
	{
		Text = 0,
		Link,
		Block,
		Header1,
		Header2,
		Header3,
		List,
		Quote
	};

	struct Line
	{
		LineType type {LineType::Text};
		std::string_view text; // text to be displayed
		std::string_view link; // only when type == LineType::Link
	};

	struct Tab
	{
		void setError(std::string errorString);
		void setData(std::shared_ptr<std::string> &dataPtr);

		std::string url;
		std::string label;
		std::string error;
		std::shared_ptr<std::string> data;

		std::vector<Line> lines;

		bool isOpen {true};
	};
}