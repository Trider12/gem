#pragma once

#include <string>
#include <vector>

namespace gem
{
	enum class GemtextLineType
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

	struct GemtextLine
	{
		GemtextLineType type {GemtextLineType::Text};
		std::string_view text; // text to be displayed
		std::string_view link; // only when type == LineType::Link
		bool linkHasSchema {false}; // only when type == LineType::Link
	};

	class GemtextParser
	{
	public:
		static void parse(std::vector<GemtextLine> &lines, const std::vector<char> &data);
	};
}