#pragma once

#include "Tab.hpp"

#include <vector>

struct SDL_Window;

namespace gem
{
	class App
	{
	public:
		App(int width, int height);
		~App();

		bool isDone() const { return _isDone; }
		void update();

	private:
		void drawMainWindow();

		SDL_Window *_window {nullptr};
		void *_glContext {nullptr};

		bool _isDone {false};

		std::vector<Tab> _tabs;
	};
}