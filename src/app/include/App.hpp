#pragma once

#include "AppContext.hpp"
#include "AppWindow.hpp"

#include <vector>

namespace gem
{
	class App
	{
	public:
		App();
		App(const App &other) = delete;
		~App();

		bool isDone() const { return _isDone; }
		void update();
		void newWindow();

	private:
		bool _isDone {false};
		AppContext _context;
		std::vector<AppWindow> _windows;
	};
}