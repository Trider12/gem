#include "App.hpp"
#include "ImGuiSDLHelper.hpp"
#include "GeminiClient.hpp"
#include "Utilities.hpp"

#include <cstdio>
#include <filesystem>
#include <unordered_set>

#include <imgui.h>
#include <imgui_stdlib.h>

using namespace gem;

App::App(int width, int height) :
	_isDone {!ImGuiSDL::init(_window, _glContext, width, height)}
{
}

App::~App()
{
	ImGuiSDL::shutdown(_window, _glContext);
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

	GeminiClient::poll();

	ImGuiSDL::newFrame();

	drawMainWindow();
	//ImGui::ShowDemoWindow();

	ImGuiSDL::render(_window);
}

namespace
{
	const Tab emptyTab {"", "New Tab", ""};

	static bool isFirstOpening = true;
	static int duplicatedTabIndex = -1;

	static void loadTab(std::shared_ptr<Tab> tab)
	{
		if (!gem::stringStartsWith(tab->url, "gemini://"))
		{
			tab->url = "gemini://" + tab->url;
		}

		// capture shared_ptr by value to prolong its life in case the tab was closed before the callback
		GeminiClient().connectAsync(tab->url, 1965,
			[tab](bool success, std::string code, std::string, std::string body)
			{
				if (success)
				{
					gem::stringTrim(body);
					tab->content = body;
					tab->label = body.substr(0, body.find('\n'));
				}
				else
				{
					tab->content = "Error " + code;
				}
			}
		);
	}

	std::unordered_set<int> tabsToRemoveIndices;

	static void removeClosedTabs(std::vector<std::shared_ptr<Tab>> &tabs)
	{
		if (tabsToRemoveIndices.empty())
		{
			return;
		}

		int last = 0;
		for (int i = 0; i < tabs.size(); ++i, ++last)
		{
			while (tabsToRemoveIndices.find(i) != tabsToRemoveIndices.end())
			{
				tabsToRemoveIndices.erase(i);
				++i;
			}

			if (i >= tabs.size()) break;

			tabs[last] = tabs[i];
		}

		tabs.resize(last);
	}
}

void App::drawMainWindow()
{
	ImGuiIO &io = ImGui::GetIO();

	constexpr ImGuiWindowFlags mainWindowFlags =
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoSavedSettings;

	ImGui::SetNextWindowPos({0, 0});
	ImGui::SetNextWindowSize(io.DisplaySize);
	ImGui::Begin("MainWindow", nullptr, mainWindowFlags);

	constexpr ImGuiTabBarFlags tabBarFlags =
		ImGuiTabBarFlags_AutoSelectNewTabs |
		ImGuiTabBarFlags_NoCloseWithMiddleMouseButton |
		ImGuiTabBarFlags_FittingPolicyResizeDown;

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {10, 10});
	ImGui::BeginTabBar("MainTabBar", tabBarFlags);

	if (_tabs.empty())
	{
		if (isFirstOpening)
		{
			_tabs.push_back(std::make_shared<Tab>(emptyTab));
			isFirstOpening = false;
		}
		else
		{
			_isDone = true;
		}
	}

	for (int i = 0; i < _tabs.size(); i++)
	{
		if (!_tabs[i]->isOpen)
		{
			tabsToRemoveIndices.insert(i);
			continue;
		}

		ImGuiTabBarFlags tabFlags = ImGuiTabItemFlags_NoPushId;

		// select the newly duplicated tab
		if (duplicatedTabIndex == i)
		{
			tabFlags |= ImGuiTabItemFlags_SetSelected;
			duplicatedTabIndex = -1;
		}

		ImGui::PushID(i);
		ImGui::SetNextItemWidth(200);
		bool isTabSelected = ImGui::BeginTabItem(_tabs[i]->label.c_str(), &_tabs[i]->isOpen, tabFlags);
		ImGui::PopID();

		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Reload"))
			{
				loadTab(_tabs[i]);
				ImGui::CloseCurrentPopup();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Duplicate"))
			{
				_tabs.insert(_tabs.begin() + i + 1, std::make_shared<Tab>(emptyTab));
				duplicatedTabIndex = i + 1;
				ImGui::CloseCurrentPopup();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Close Tab"))
			{
				tabsToRemoveIndices.insert(i);
				ImGui::CloseCurrentPopup();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Close All Tabs"))
			{
				_isDone = true;
				ImGui::CloseCurrentPopup();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Close All But This"))
			{
				for (int j = 0; j < _tabs.size(); j++)
				{
					if (i == j)
						continue;

					tabsToRemoveIndices.insert(j);
				}

				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		if (isTabSelected)
		{
			// Toolbar
			ImGui::Button(ICON_FA_ARROW_LEFT, {40.f, 0.f});
			ImGui::SameLine();
			ImGui::Button(ICON_FA_ARROW_RIGHT, {40.f, 0.f});
			ImGui::SameLine();

			if (ImGui::Button(ICON_FA_REPEAT, {40.f, 0.f}))
			{
				loadTab(_tabs[i]);
			}
			ImGui::SameLine();

			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 50);
			if (ImGui::InputTextWithHint("", "Enter address", &_tabs[i]->url, ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_EnterReturnsTrue))
			{
				loadTab(_tabs[i]);
			}
			ImGui::SameLine();

			ImGui::Button(ICON_FA_COG, {40.f, 0.f});

			// Page
			ImGui::BeginChild(1);
			ImGui::InputTextMultiline("##hidden", &_tabs[i]->content, ImGui::GetContentRegionAvail(), ImGuiInputTextFlags_ReadOnly);
			ImGui::EndChild();

			ImGui::EndTabItem();
		}
	}

	if (ImGui::TabItemButton(ICON_FA_PLUS, ImGuiTabItemFlags_Trailing))
	{
		_tabs.push_back(std::make_shared<Tab>(emptyTab));
	}

	ImGui::EndTabBar();
	ImGui::PopStyleVar();

	ImGui::End();

	removeClosedTabs(_tabs);
}