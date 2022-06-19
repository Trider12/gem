#include "Tab.hpp"
#include "Utilities.hpp"

#include <string_view>

using namespace gem;

bool Tab::isOpen()
{
	return _isOpen;
}

void Tab::setOpen(bool open)
{
	_isOpen = open;
}

bool Tab::hasNextPage()
{
	return _currentPageIndex < _pages.size() - 1;
}

bool Tab::hasPrevPage()
{
	return _currentPageIndex > 0;
}

void Tab::nextPage()
{
	_currentPageIndex++;
	std::shared_ptr<Page> page = getCurrentPage();
	_addressBarText = std::string(page->getUrl());

	if (!page->isLoaded())
	{
		loadCurrentPage();
	}
}

void Tab::prevPage()
{
	_currentPageIndex--;
	std::shared_ptr<Page> page = getCurrentPage();
	_addressBarText = std::string(page->getUrl());

	if (!page->isLoaded())
	{
		loadCurrentPage();
	}
}

std::shared_ptr<Page> Tab::getCurrentPage()
{
	assert(_currentPageIndex >= 0 && _currentPageIndex < _pages.size());
	return _pages[_currentPageIndex];
}

void Tab::loadCurrentPage()
{
	std::shared_ptr<Page> page = getCurrentPage();

	if (page->getUrl().empty())
	{
		return;
	}

	page->load();
}

void Tab::loadNewPage(std::string_view url, bool hasSchema, std::string_view baseUrl)
{
	std::string newUrl;

	if (hasSchema)
	{
		newUrl = url;
	}
	else
	{
		if (baseUrl.empty())
		{
			baseUrl = getCurrentPage()->getUrl();
		}

		if (url[0] == '/') // relative to host name
		{
			newUrl = "gemini://" + std::string(extractHostName(baseUrl)) + std::string(url);
		}
		else // relative to current page
		{
			auto getDirName = [](std::string_view path)
			{
				return path.substr(0, path.find_last_of('/', path.size() - 2) + 1);
			};

			if (*baseUrl.rbegin() != '/') // current page is not a directory
			{
				baseUrl = getDirName(baseUrl);
			}

			std::string_view relativeUrl = url;

			if (relativeUrl == ".")
			{
				relativeUrl = "";
			}
			else if (relativeUrl == "..")
			{
				baseUrl = getDirName(baseUrl);
				relativeUrl = "";
			}
			else
			{
				if (stringStartsWith(relativeUrl, "./"))
				{
					relativeUrl = relativeUrl.substr(2);
				}

				while (stringStartsWith(relativeUrl, "../"))
				{
					baseUrl = getDirName(baseUrl);
					relativeUrl = relativeUrl.substr(3);
				}
			}

			newUrl = std::string(baseUrl) + std::string(relativeUrl);
		}
	}

	_pages.resize(_currentPageIndex + 1);
	_pages.push_back(std::make_shared<Page>(newUrl));
	_currentPageIndex++;
	_addressBarText = std::string(getCurrentPage()->getUrl());

	loadCurrentPage();
}

void Tab::loadNewPage(std::shared_ptr<Page> page)
{
	_pages.resize(_currentPageIndex + 1);
	_pages.push_back(page);
	_currentPageIndex++;
	_addressBarText = std::string(getCurrentPage()->getUrl());

	if (!page->isLoaded())
	{
		loadCurrentPage();
	}
}

std::string &Tab::getAddressBarText()
{
	return _addressBarText;
}