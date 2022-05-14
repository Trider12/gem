#include "App.hpp"
#include "ImGuiSDLHelper.hpp"
#include "MainWindow.hpp"
#include "GeminiClient.hpp"
#include "Utilities.hpp"

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

	static void loadTab(std::shared_ptr<Tab> tab)
	{
		if (!gem::stringStartsWith(tab->url, "gemini://"))
		{
			tab->url = "gemini://" + tab->url;
		}

		// capture shared_ptr by value to prolong its life in case the tab was closed before the callback
		GeminiClient().connectAsync(tab->url, 1965,
			[tab](bool success, std::string code, std::string, std::shared_ptr<std::string> body)
			{
				if (success)
				{
					tab->setData(body);
				}
				else
				{
					tab->setError(code);
				}
			}
		);
	}
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
			_tabs.push_back(std::make_shared<Tab>(gui::emptyTab));
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

	gui::drawMainWindow(_tabs, loadTab);
	//ImGui::ShowDemoWindow();

	ImGuiSDL::render(_window);
}