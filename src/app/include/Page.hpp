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
		bool isAbsolute {false}; // only when type == LineType::Link
	};

	struct Page
	{
	public:
		Page() = default;
		Page(std::string url);
		Page(const Page &page) = default;

		void setData(size_t code, std::string meta, std::shared_ptr<std::string> &body);

		std::string url;
		std::string label;
		std::string hostname;

		std::vector<Line> lines;
		bool isLoaded {false};

		static const Page newTabPage;

	private:
		size_t _code = 0;
		std::string _meta;
		std::shared_ptr<std::string> _data;
	};
}