#pragma once

#include "Tab.hpp"

#include <functional>

namespace gem::gui
{
	using LoadLinkCallback = std::function<void(std::shared_ptr<Tab>)>;

	const Tab emptyTab {"", "New Tab"};

	void loadFonts();
	void setThemeDeepDark();

	void drawMainWindow(std::vector<std::shared_ptr<Tab>> &tabs, const LoadLinkCallback &callback);
}