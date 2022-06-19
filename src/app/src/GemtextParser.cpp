#include "GemtextParser.hpp"

#include <cassert>

namespace
{
	static inline bool isBlank(const std::vector<char> &data, size_t index)
	{
		return std::isblank(static_cast<unsigned char>(data[index]));
	}
}

void gem::GemtextParser::parse(std::vector<GemtextLine> &lines, const std::vector<char> &data)
{
	const size_t size = data.size();
	std::string_view view(data.data(), size);

	GemtextLineType lineType = GemtextLineType::Text;
	bool newLineStarted = true, blockModeOn = false, foundSpaceBetweenLinkAndText = false;
	int64_t lineTextStart = -1, lineLinkEnd = -1, lineLinkStart = -1;

	lines.clear();

	for (size_t i = 0; i < size; i++)
	{
		if (data[i] == '\n' || i == size - 1) // line ended or eof
		{
			if (!blockModeOn) // not inside a block
			{
				size_t lineEnd = (i == size - 1 ? size : i);

				switch (lineType)
				{
					case GemtextLineType::Text:
					{
						if (newLineStarted) // empty line
						{
							lines.push_back({lineType, ""});
						}
						else
						{
							lines.push_back({lineType, view.substr(lineTextStart, lineEnd - lineTextStart)});
						}

						break;
					}
					case GemtextLineType::Link:
					{
						GemtextLine line;

						assert(lineLinkStart >= 0);
						if (lineLinkEnd > lineLinkStart) // link with text
						{
							line = {lineType, view.substr(lineTextStart, lineEnd - lineTextStart), view.substr(lineLinkStart, lineLinkEnd - lineLinkStart)};
						}
						else // just link
						{
							line = {lineType, "", view.substr(lineLinkStart, lineEnd - lineLinkStart)};
						}

						line.linkHasSchema = line.link.find("//") != std::string::npos;
						lines.push_back(line);

						lineLinkStart = -1;
						lineLinkEnd = -1;
						foundSpaceBetweenLinkAndText = false;

						break;
					}
					case GemtextLineType::Block:
						lineEnd -= 3; // skip trailing ```
					default:
						if (lineTextStart >= 0)
						{
							lines.push_back({lineType, view.substr(lineTextStart, lineEnd - lineTextStart)});
						}

						break;
				}

				lineTextStart = -1;
				lineType = GemtextLineType::Text;
			}

			newLineStarted = true;
			continue;
		}

		if (newLineStarted) // beginning of a new line
		{
			if (blockModeOn)
			{
				if (lineTextStart == -1)
				{
					lineTextStart = i; // start block text
				}

				if (i + 2 < size)
				{
					if (data[i] == '`' && data[i + 1] == '`' && data[i + 2] == '`')
					{
						blockModeOn = false;
						i += 2; // skip the 2nd and the 3rd '`'
					}
				}
				else // page ended without closing a block
				{
					blockModeOn = false;
					i = size; // skip to the end
					lines.push_back({lineType, view.substr(lineTextStart, i - lineTextStart)});
					lineTextStart = -1;
				}

				newLineStarted = false;
				continue; // ignore everything inside a block
			}

			if (isBlank(data, i)) // ignore spaces or tabs
			{
				continue;
			}

			lineType = GemtextLineType::Text;

			if (data[i] == '#')
			{
				lineType = GemtextLineType::Header1;
			}

			if (data[i] == '>')
			{
				lineType = GemtextLineType::Quote;
			}

			if (i + 1 < size)
			{
				if (data[i] == '=' && data[i + 1] == '>')
				{
					lineType = GemtextLineType::Link;
				}

				if (data[i] == '#' && data[i + 1] == '#')
				{
					lineType = GemtextLineType::Header2;
				}

				if (data[i] == '*' && data[i + 1] == ' ')
				{
					lineType = GemtextLineType::List;
				}
			}

			if (i + 2 < size)
			{
				if (data[i] == '#' && data[i + 1] == '#' && data[i + 2] == '#')
				{
					lineType = GemtextLineType::Header3;
				}

				if (data[i] == '`' && data[i + 1] == '`' && data[i + 2] == '`')
				{
					lineType = GemtextLineType::Block;
				}
			}

			switch (lineType)
			{
				case GemtextLineType::Text:
					lineTextStart = i; // start immediately
					break;
				case GemtextLineType::Link:
					i++; // skip '>'
					break;
				case GemtextLineType::Block:
					i += 2; // skip the 2nd and the 3rd '`'
					blockModeOn = true;
					break;
				case GemtextLineType::Header1:
					break;
				case GemtextLineType::Header2:
					i++; // skip the 2nd '#'
					break;
				case GemtextLineType::Header3:
					i += 2; //skip the 2nd and the 3rd '#'
					break;
				case GemtextLineType::List:
					i++; // skip ' '
					break;
				case GemtextLineType::Quote:
					break;
			}

			newLineStarted = false;
		}
		else // somewhere inside a line
		{
			switch (lineType)
			{
				case GemtextLineType::Link:
				{
					if (lineLinkStart == -1)
					{
						if (!isBlank(data, i)) // skip leading spaces
						{
							lineLinkStart = i;
						}
					}
					else if (lineTextStart == -1)
					{
						if (isBlank(data, i))
						{
							if (lineLinkEnd == -1)
							{
								lineLinkEnd = i;
							}

							foundSpaceBetweenLinkAndText = true;
						}
						else if (foundSpaceBetweenLinkAndText)
						{
							lineTextStart = i;
							foundSpaceBetweenLinkAndText = false;
						}
					}
					break;
				}
				case GemtextLineType::Header1:
				case GemtextLineType::Header2:
				case GemtextLineType::Header3:
				case GemtextLineType::List:
				case GemtextLineType::Quote:
				{
					if (lineTextStart == -1)
					{
						if (!isBlank(data, i)) // skip leading spaces
						{
							lineTextStart = i;
						}
					}
					break;
				}
				default:
					break;
			}
		}
	}
}