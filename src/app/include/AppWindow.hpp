#pragma once

#include "Tab.hpp"

#include <vector>

#include <SDL_events.h>

// TODO: fix multiple windows with SDL_renderer
#ifdef GEM_SDLRENDERER
struct SDL_Renderer;
#endif
struct SDL_Window;
struct ImGuiContext;

namespace gem
{
	class App;
	struct AppContext;

	class AppWindow
	{
	public:
		AppWindow(App *app, AppContext *context);
		AppWindow(const AppWindow &other) = delete;
		AppWindow(AppWindow &&other) noexcept;
		~AppWindow();

		AppWindow &operator=(const AppWindow &other) = delete;
		AppWindow &operator=(AppWindow &&other);

		static void loadFonts();

		void handleEvent(SDL_Event &e);
		void update();
		void render();

		bool isVisible() const { return _isVisible; }
		void saveContext();
	private:
		App *_app;
		AppContext *_appContext;
		std::vector<Tab> _tabs;
		int32_t _forceSelectedTabIndex = -1;

#ifdef GEM_SDLRENDERER
		SDL_Renderer *_renderer;
#else
		void *_glContext {nullptr};
#endif

		SDL_Window *_window {nullptr};
		uint32_t _windowId {0};
		ImGuiContext *_imguiContext {nullptr};

		int32_t _positionX {0};
		int32_t _positionY {0};
		int32_t _width {0};
		int32_t _height {0};
		uint32_t _displayIndex {0};
		bool _isMaximized {false};
		bool _isVisible {false};
	};
}