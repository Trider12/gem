#pragma once

#include "Page.hpp"

namespace gem
{
	class Tab
	{
	public:
		bool isOpen();
		void setOpen(bool open);

		bool hasNextPage();
		bool hasPrevPage();

		void nextPage();
		void prevPage();

		std::shared_ptr<Page> getCurrentPage();
		void loadCurrentPage();

		void loadNewPage(std::string_view url, bool hasSchema, std::string_view baseUrl = "");
		void loadNewPage(std::shared_ptr<Page> page);

		std::string &getAddressBarText();

	private:
		bool _isOpen {true};
		std::string _addressBarText;
		std::vector<std::shared_ptr<Page>> _pages;
		int32_t _currentPageIndex {-1};
	};
}