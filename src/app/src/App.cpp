#include "App.hpp"
#include "GeminiClient.hpp"

#include <stdexcept>
#include <iterator>

#include <SDL.h>
#include <SDL_filesystem.h>

#include <imgui_impl_sdl.h>

#include <nfd.h>

using namespace gem;

namespace
{
	static const std::string appPath = SDL_GetPrefPath(nullptr, "gem");
	static const std::string settingsPath = appPath + "Settings.json";
	static const std::string userDataPath = appPath + "UserData.json";

	static uint32_t newWindowsCount = 0;
}

App::App()
{
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		char buffer[128];
		snprintf(buffer, sizeof(buffer), "SDL Error: %s\n", SDL_GetError());
		fputs(buffer, stderr);
		throw std::runtime_error(buffer);
	}

	if (NFD_Init() != NFD_OKAY) {
		char buffer[128];
		snprintf(buffer, sizeof(buffer), "NFD Error: %s\n", NFD_GetError());
		fputs(buffer, stderr);
		throw std::runtime_error(buffer);
	}

	_context.settings.load(settingsPath);
	_context.userData.load(userDataPath);

	AppWindow::loadFonts();

	newWindow();
}

App::~App()
{
	_context.settings.save(settingsPath);
	_context.userData.save(userDataPath);

	NFD_Quit();
	SDL_Quit();
}

void App::update()
{
	for (SDL_Event event; SDL_PollEvent(&event);)
	{
		if (event.type == SDL_QUIT)
		{
			_isDone = true;
		}
		else
		{
			for (AppWindow &window : _windows)
			{
				window.handleEvent(event);
			}
		}
	}

	GeminiClient::poll();

	// TODO: multi threading

	for (AppWindow &window : _windows)
	{
		window.update();
		window.render();
	}

	for (; newWindowsCount > 0; newWindowsCount--)
	{
		_windows.emplace_back(this, &_context);
	}

	auto firstToDelete = std::remove_if(_windows.begin(), _windows.end(), [](const AppWindow &window) { return !window.isVisible(); });

	if (std::distance(firstToDelete, _windows.begin()) == 0) // closing all windows
	{
		_windows.rbegin()->saveContext(); // save last window context
	}

	_windows.erase(firstToDelete, _windows.end());

	_isDone = _windows.empty();
}

void App::newWindow()
{
	newWindowsCount++;
}