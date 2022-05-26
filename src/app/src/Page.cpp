#include "Tab.hpp"
#include "Utilities.hpp"

#include <cassert>

using namespace gem;

namespace
{
	static inline bool isBlank(const std::string &str, size_t index)
	{
		return std::isblank(static_cast<unsigned char>(str[index]));
	}

	static inline Page createNewTabPage()
	{
		Page page;
		page.label = "New Tab";
		page.isLoaded = true;

		return page;
	}
}

const Page Page::newTabPage = createNewTabPage();

Page::Page(std::string url) :url {url}
{
}

void Page::setData(size_t code, std::string meta, std::shared_ptr<std::string> &body)
{
	_code = code;
	_meta = meta;
	lines.clear();

	if (_code == 20)
	{
		_data = body;
	}
	else
	{
		_data = std::make_shared<std::string>("Error " + std::to_string(code));
	}

	if (!_data)
	{
		return;
	}

	std::string_view view(*_data);
	const size_t size = _data->size();

	LineType lineType = LineType::Text;
	bool newLineStarted = true, blockModeOn = false, foundSpaceBetweenLinkAndText = false;
	int64_t lineTextStart = -1, lineLinkEnd = -1, lineLinkStart = -1;

	for (size_t i = 0; i < size; i++)
	{
		if ((*_data)[i] == '\n' || i == size - 1) // line ended or eof
		{
			if (!blockModeOn) // not inside a block
			{
				size_t lineEnd = (i == size - 1 ? size : i);

				switch (lineType)
				{
					case LineType::Text:
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
					case LineType::Link:
					{
						Line line;

						assert(lineLinkStart >= 0);
						if (lineLinkEnd > lineLinkStart) // link with text
						{
							line = {lineType, view.substr(lineTextStart, lineEnd - lineTextStart), view.substr(lineLinkStart, lineLinkEnd - lineLinkStart)};
						}
						else // just link
						{
							line = {lineType, "", view.substr(lineLinkStart, lineEnd - lineLinkStart)};
						}

						line.isAbsolute = line.link.find("//") != std::string::npos;
						lines.push_back(line);

						lineLinkStart = -1;
						lineLinkEnd = -1;
						foundSpaceBetweenLinkAndText = false;

						break;
					}
					case LineType::Block:
						lineEnd -= 3; // skip trailing ```
					default:
						if (lineTextStart >= 0)
						{
							lines.push_back({lineType, view.substr(lineTextStart, lineEnd - lineTextStart)});
						}

						break;
				}

				lineTextStart = -1;
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
					if ((*_data)[i] == '`' && (*_data)[i + 1] == '`' && (*_data)[i + 2] == '`')
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

			if (isBlank(*_data, i)) // ignore spaces or tabs
			{
				continue;
			}

			lineType = LineType::Text;

			if ((*_data)[i] == '#')
			{
				lineType = LineType::Header1;
			}

			if ((*_data)[i] == '>')
			{
				lineType = LineType::Quote;
			}

			if (i + 1 < size)
			{
				if ((*_data)[i] == '=' && (*_data)[i + 1] == '>')
				{
					lineType = LineType::Link;
				}

				if ((*_data)[i] == '#' && (*_data)[i + 1] == '#')
				{
					lineType = LineType::Header2;
				}

				if ((*_data)[i] == '*' && (*_data)[i + 1] == ' ')
				{
					lineType = LineType::List;
				}
			}

			if (i + 2 < size)
			{
				if ((*_data)[i] == '#' && (*_data)[i + 1] == '#' && (*_data)[i + 2] == '#')
				{
					lineType = LineType::Header3;
				}

				if ((*_data)[i] == '`' && (*_data)[i + 1] == '`' && (*_data)[i + 2] == '`')
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
						if (!isBlank(*_data, i)) // skip leading spaces
						{
							lineLinkStart = i;
						}
					}
					else if (lineTextStart == -1)
					{
						if (isBlank(*_data, i))
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
						if (!isBlank(*_data, i)) // skip leading spaces
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

	for (const Line &line : lines)
	{
		if (line.type != LineType::Block && !line.text.empty())
		{
			label = line.text;
			break;
		}
	}
}