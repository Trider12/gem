#include "App.hpp"
#include "ImGuiSDLHelper.hpp"
#include "MainWindow.hpp"
#include "GeminiClient.hpp"

using namespace gem;

App::App(int width, int height) :
	_isDone {!ImGuiSDL::init(_window, _glContext, width, height)}
{
	ImGuiIO &io = ImGui::GetIO();
	io.IniFilename = nullptr;

	gui::loadFonts();
	gui::setThemeDeepDark();
}

App::~App()
{
	ImGuiSDL::shutdown(_window, _glContext);
}

namespace
{
	bool isFirstOpening = true;
}

void App::update()
{
	for (SDL_Event event; SDL_PollEvent(&event);)
	{
		ImGui_ImplSDL2_ProcessEvent(&event);

		if (event.type == SDL_QUIT ||
			(event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(_window)))
		{
			_isDone = true;
		}
	}

	if (_tabs.empty())
	{
		if (isFirstOpening)
		{
			Tab tab;
			tab.loadNewPage(std::make_shared<Page>(Page::newTabPage));
			_tabs.push_back(tab);
			isFirstOpening = false;
		}
		else
		{
			_isDone = true;
			return;
		}
	}

	GeminiClient::poll();

	render();
}

void App::render()
{
	ImGuiSDL::newFrame();

	gui::drawMainWindow(_tabs);
	//ImGui::ShowDemoWindow();

	ImGuiSDL::render(_window);
}