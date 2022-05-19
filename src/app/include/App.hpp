#pragma once

#include "Tab.hpp"

#include <vector>
#include <memory>

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
		void render();

		SDL_Window *_window {nullptr};
		void *_glContext {nullptr};

		bool _isDone {false};

		std::vector<Tab> _tabs;
	};
}