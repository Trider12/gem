#pragma once

#include "Tab.hpp"

#include <functional>

namespace gem::gui
{
	void loadFonts();
	void setThemeDeepDark();

	void drawMainWindow(std::vector<Tab> &tabs);
}