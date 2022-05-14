#include "Tab.hpp"

#include <cassert>

namespace
{
	static inline bool isBlank(const std::string &str, size_t index)
	{
		return std::isblank(static_cast<unsigned char>(str[index]));
	}
}

using namespace gem;

void Tab::setError(std::string errorString)
{
	error = "Error " + errorString; //TODO: improve this
}

void Tab::setData(std::shared_ptr<std::string> &dataPtr)
{
	lines.clear();
	data = dataPtr;
	std::string_view view(*data);
	const size_t size = data->size();

	LineType lineType = LineType::Text;
	bool newLineStarted = true, blockModeOn = false, foundSpaceBetweenLinkAndText = false;
	int64_t lineTextStart = -1, lineLinkEnd = -1, lineLinkStart = -1;

	for (size_t i = 0; i < size; i++)
	{
		if ((*data)[i] == '\n' || i == size - 1) // line ended or eof
		{
			if (!blockModeOn) // not inside a block
			{
				if (lineType == LineType::Text && newLineStarted) // empty line
				{
					lines.push_back({lineType, ""});
				}
				else
				{
					if (lineTextStart >= 0) // if a line has actual text after special symbols
					{
						size_t lineTextEnd = (i == size - 1 ? size : i);

						switch (lineType)
						{
							case LineType::Link:
							{
								assert(lineLinkEnd >= 0);
								assert(lineLinkStart >= 0);
								lines.push_back({lineType, view.substr(lineTextStart, lineTextEnd - lineTextStart), view.substr(lineLinkStart, lineLinkEnd - lineLinkStart)});
								lineLinkStart = -1;
								foundSpaceBetweenLinkAndText = false;
								break;
							}
							case LineType::Block:
							{
								lines.push_back({lineType, view.substr(lineTextStart, lineTextEnd - 3 - lineTextStart)}); // skip trailing ```
								break;
							}
							default:
							{
								lines.push_back({lineType, view.substr(lineTextStart, lineTextEnd - lineTextStart)});
								break;
							}
						}

						lineTextStart = -1;
					}
				}

				lineType = LineType::Text;
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
					if ((*data)[i] == '`' && (*data)[i + 1] == '`' && (*data)[i + 2] == '`')
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

			if (isBlank(*data, i)) // ignore spaces or tabs
			{
				continue;
			}

			lineType = LineType::Text;

			if ((*data)[i] == '#')
			{
				lineType = LineType::Header1;
			}

			if ((*data)[i] == '>')
			{
				lineType = LineType::Quote;
			}

			if (i + 1 < size)
			{
				if ((*data)[i] == '=' && (*data)[i + 1] == '>')
				{
					lineType = LineType::Link;
				}

				if ((*data)[i] == '#' && (*data)[i + 1] == '#')
				{
					lineType = LineType::Header2;
				}

				if ((*data)[i] == '*' && (*data)[i + 1] == ' ')
				{
					lineType = LineType::List;
				}
			}

			if (i + 2 < size)
			{
				if ((*data)[i] == '#' && (*data)[i + 1] == '#' && (*data)[i + 2] == '#')
				{
					lineType = LineType::Header3;
				}

				if ((*data)[i] == '`' && (*data)[i + 1] == '`' && (*data)[i + 2] == '`')
				{
					lineType = LineType::Block;
				}
			}

			switch (lineType)
			{
				case LineType::Text:
					lineTextStart = i; // start immediately
					break;
				case LineType::Link:
					i++; // skip '>'
					break;
				case LineType::Block:
					i += 2; // skip the 2nd and the 3rd '`'
					blockModeOn = true;
					break;
				case LineType::Header1:
					break;
				case LineType::Header2:
					i++; // skip the 2nd '#'
					break;
				case LineType::Header3:
					i += 2; //skip the 2nd and the 3rd '#'
					break;
				case LineType::List:
					i++; // skip ' '
					break;
				case LineType::Quote:
					break;
			}

			newLineStarted = false;
		}
		else // somewhere inside a line
		{
			switch (lineType)
			{
				case LineType::Link:
				{
					if (lineLinkStart == -1)
					{
						if (!isBlank(*data, i)) // skip leading spaces
						{
							lineLinkStart = i;
						}
					}
					else if (lineTextStart == -1)
					{
						if (isBlank(*data, i))
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
				case LineType::Header1:
				case LineType::Header2:
				case LineType::Header3:
				case LineType::List:
				case LineType::Quote:
				{
					if (lineTextStart == -1)
					{
						if (!isBlank(*data, i)) // skip leading spaces
						{
							lineTextStart = i;
						}
					}
					break;
				}
			}
		}
	}
}