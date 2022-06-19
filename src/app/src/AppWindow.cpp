#include "AppWindow.hpp"
#include "App.hpp"
#include "AppContext.hpp"

#include <fstream>
#include <filesystem>
#include <unordered_set>

#include <SDL.h>
#include <SDL_opengl.h>

#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_opengl3.h>
#include <imgui_stdlib.h>
#include <imgui_freetype.h>

#include <IconsFontAwesome4.h>

#include <nfd.h>

using namespace gem;

namespace
{
	static ImFontAtlas fontAtlas;
	static ImFont *fontInternal = nullptr, // regular + icons
		*fontRegular = nullptr, // regular + emojis
		*fontMono = nullptr, // mono + emojis
		*fontH1 = nullptr, // regular + emojis
		*fontH2 = nullptr, // regular + emojis
		*fontH3 = nullptr; // regular + emojis

	static void removeClosedTabs(std::vector<Tab> &tabs, std::unordered_set<uint32_t> tabsToRemoveIndices)
	{
		if (tabsToRemoveIndices.empty())
		{
			return;
		}

		uint32_t last = 0;

		for (uint32_t i = 0; i < tabs.size(); ++i, ++last)
		{
			while (tabsToRemoveIndices.find(i) != tabsToRemoveIndices.end())
			{
				tabsToRemoveIndices.erase(i);
				++i;
			}

			if (i >= tabs.size())
			{
				break;
			}

			tabs[last] = tabs[i];
		}

		tabs.resize(last);
	}

	static void drawLink(std::vector<Tab> &tabs, uint32_t currentTabIndex, const GemtextLine &line)
	{
		static const ImU32 linkColorU32 = ImGui::GetColorU32({0.f, 100.f / 255.f, 220.f / 255.f, 1.f});
		static const ImU32 transparentColorU32 = ImGui::GetColorU32({0.f, 0.f, 0.f, 0.f});

		const char *start = nullptr, *end = nullptr;

		if (!line.text.empty())
		{
			start = line.text.data();
			end = start + line.text.size();
		}
		else if (!line.link.empty())
		{
			start = line.link.data();
			end = start + line.link.size();
		}
		else
		{
			assert(false);
		}

		ImGui::PushStyleColor(ImGuiCol_Text, linkColorU32);
		ImGui::PushStyleColor(ImGuiCol_Button, transparentColorU32);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, transparentColorU32);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, transparentColorU32);

		if (ImGui::SmallButton(start, end))
		{
			tabs[currentTabIndex].loadNewPage(line.link, line.linkHasSchema);
		}

		ImGui::PopStyleColor(4);

		if (ImGui::IsItemHovered())
		{
			ImVec2 min = ImGui::GetItemRectMin();
			ImVec2 max = ImGui::GetItemRectMax();
			max.y -= 1;
			min.y = max.y;
			ImGui::GetWindowDrawList()->AddLine(min, max, linkColorU32, 1.0f);
			ImGui::SetTooltip("%.*s", static_cast<int>(line.link.size()), line.link.data());
		}

		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Open in a New Tab"))
			{
				Tab newTab;
				newTab.loadNewPage(line.link, line.linkHasSchema, tabs[currentTabIndex].getCurrentPage()->getUrl());
				tabs.insert(tabs.begin() + currentTabIndex + 1, std::move(newTab));
			}

			if (ImGui::MenuItem("Copy"))
			{
				ImGui::SetClipboardText(std::string(line.link.data()).c_str());
			}

			ImGui::EndPopup();
		}
	}

	static void drawText(const char *textStart, const char *textEnd)
	{
		// TODO: selectable text
		ImGui::PushTextWrapPos(0.0f); // enable word wrapping
		ImGui::TextUnformatted(textStart, textEnd);
		ImGui::PopTextWrapPos();
	}

	static void drawText(std::string_view text)
	{
		drawText(text.data(), text.data() + text.size());
	}

	static void drawText(const GemtextLine &line)
	{
		drawText(line.text);
	}

	static void drawTextCentered(const char *textStart, const char *textEnd)
	{
		float width = ImGui::GetContentRegionAvail().x;
		float size = ImGui::CalcTextSize(textStart, textEnd).x;
		float indent = std::max((width - size) * 0.5f, 20.f);

		ImGui::Indent(indent);
		ImGui::PushTextWrapPos(width - indent); // enable word wrapping
		ImGui::TextUnformatted(textStart, textEnd);
		ImGui::PopTextWrapPos();
		ImGui::Unindent(indent);
	}

	static void drawTextCentered(std::string_view text)
	{
		drawTextCentered(text.data(), text.data() + text.size());
	}

	static void drawBlock(const GemtextLine &line, size_t index)
	{
		const ImVec4 &blockBgColor = ImGui::GetStyleColorVec4(ImGuiCol_FrameBg);
		const char *textStart = line.text.data();
		const char *textEnd = textStart + line.text.size();

		ImVec2 size = ImGui::CalcTextSize(textStart, textEnd);
		ImGui::PushStyleColor(ImGuiCol_ChildBg, blockBgColor);
		ImGui::BeginChild(("Block" + std::to_string(index)).c_str(), {-10.f, size.y}, false, ImGuiWindowFlags_HorizontalScrollbar);
		ImGui::PushFont(fontMono);
		ImGui::TextUnformatted(textStart, textEnd);
		ImGui::PopFont();
		ImGui::EndChild();
		ImGui::PopStyleColor();
	}

	static void drawHeader(const GemtextLine &line, ImFont *font)
	{
		ImGui::PushFont(font);
		drawText(line);
		ImGui::PopFont();
	}

	static void drawList(const GemtextLine &line)
	{
		ImGui::Bullet();
		drawText(line);
	}

	static void drawQuote(const GemtextLine &line)
	{
		const ImU32 quoteBgColor = ImGui::GetColorU32(ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
		static const ImU32 quoteLineColor = ImGui::GetColorU32({1.f, 1.f, 1.f, 1.f});
		static constexpr float quoteLineWidth = 2.f;
		static constexpr float quoteLineSpacing = 4.f;
		static constexpr float quoteOffset = quoteLineWidth + quoteLineSpacing;

		ImDrawList *drawList = ImGui::GetWindowDrawList();
		ImDrawListSplitter splitter;
		splitter.Split(drawList, 2);

		splitter.SetCurrentChannel(drawList, 1);
		ImGui::Indent(quoteOffset);
		drawText(line);
		ImGui::Unindent(quoteOffset);

		splitter.SetCurrentChannel(drawList, 0);
		ImVec2 min = ImGui::GetItemRectMin();
		ImVec2 max = ImGui::GetItemRectMax();
		min.x -= quoteOffset;
		drawList->AddRectFilled(min, {min.x + quoteLineWidth, max.y}, quoteLineColor);
		min.x += quoteLineWidth;
		drawList->AddRectFilled(min, max, quoteBgColor);

		splitter.Merge(drawList);
	}

	static void drawGemtextPage(std::vector<Tab> &tabs, uint32_t currentTabIndex)
	{
		Tab &tab = tabs[currentTabIndex];
		std::shared_ptr<Page> page = tab.getCurrentPage();

		ImGui::PushFont(fontRegular);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {0.f, 0.f});
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0.f, 0.f});

		const std::vector<GemtextLine> &lines = page->getPageData<GemtextPageData>()->lines;

		for (size_t i = 0; i < lines.size(); i++)
		{
			const GemtextLine &line = lines[i];

			switch (line.type)
			{
				case GemtextLineType::Text:
					drawText(line);
					break;
				case GemtextLineType::Link:
					drawLink(tabs, currentTabIndex, line);
					break;
				case GemtextLineType::Block:
					drawBlock(line, i);
					break;
				case GemtextLineType::Header1:
					drawHeader(line, fontH1);
					break;
				case GemtextLineType::Header2:
					drawHeader(line, fontH2);
					break;
				case GemtextLineType::Header3:
					drawHeader(line, fontH3);
					break;
				case GemtextLineType::List:
					drawList(line);
					break;
				case GemtextLineType::Quote:
					drawQuote(line);
					break;
				default:
					assert(false);
			}
		}

		ImGui::PopStyleVar(2);
		ImGui::PopFont();
	}

	static void drawTextPage(Tab &tab)
	{
		std::shared_ptr<Page> page = tab.getCurrentPage();
		std::string_view teztData = page->getData();

		drawText(teztData);
	}

	static void drawImagePage(Tab &tab)
	{
		std::shared_ptr<Page> page = tab.getCurrentPage();
		ImagePageData *imageData = page->getPageData<ImagePageData>();

		ImGui::Image((void *)static_cast<intptr_t>(imageData->textureId), ImVec2(static_cast<float>(imageData->imageWidth), static_cast<float>(imageData->imageHeight)));
	}

	static void drawUnsupportedPage(Tab &tab)
	{
		std::shared_ptr<Page> page = tab.getCurrentPage();

		if (!page->isDownloaded())
		{
			// TODO: non-blocking call

			char *savePath;
			const char *filename = page->getLabel().data();
			nfdresult_t result = NFD_SaveDialogU8(&savePath, nullptr, 0, nullptr, filename);

			if (result == nfdresult_t::NFD_OKAY)
			{
				page->download(savePath);

				NFD_FreePathU8(savePath);
			}
			else
			{
				page->download(nullptr);
			}
		}
	}

	static void drawErrorPage(Tab &tab, std::string_view error)
	{
		const ImVec2 size = ImGui::GetContentRegionAvail();
		const float width = size.x, height = size.y;
		const float buttonWidth = 100.f;
		const float indent = (width + buttonWidth) * 0.5f;

		ImGui::Dummy({0.f, height * 0.33f});
		drawTextCentered(error);
		ImGui::Spacing();
		ImGui::Indent(indent);
		bool buttonClicked = ImGui::Button("Retry", {buttonWidth, 0.f});
		ImGui::Unindent(indent);

		if (buttonClicked)
		{
			tab.loadCurrentPage();
		}
	}

	static void drawNewTabPage()
	{
		// TODO: searchbar, etc
	}

	static void drawSettingsPage()
	{
		// TODO: default download path, link preview, themes, etc
	}

	static void drawPage(std::vector<Tab> &tabs, uint32_t currentTabIndex)
	{
		Tab &tab = tabs[currentTabIndex];
		std::shared_ptr<Page> page = tab.getCurrentPage();

		ImGui::BeginChild("Page", {0.f, 0.f}, false, ImGuiWindowFlags_HorizontalScrollbar);

		if (std::string_view error = page->getError(); !error.empty())
		{
			drawErrorPage(tab, error);
		}
		else
		{
			switch (page->getPageType())
			{
				case PageType::None:
					break;
				case PageType::Gemtext:
					drawGemtextPage(tabs, currentTabIndex);
					break;
				case PageType::Text:
					drawTextPage(tab);
					break;
				case PageType::Image:
					drawImagePage(tab);
					break;
				case PageType::Unsupported:
					drawUnsupportedPage(tab);
					break;
				case PageType::NewTab:
					drawNewTabPage();
					break;
				case PageType::Settings:
					drawSettingsPage();
					break;
				default:
					assert(false);
			}
		}

		ImGui::EndChild();
	}

	static bool drawSideMenu(App &app, UserData &userData, Tab &tab, std::vector<Tab> &tabs)
	{
		bool exitRequested = false;

		// TODO: hotkeys

		if (ImGui::BeginPopup("Menu"))
		{
			if (ImGui::MenuItem("New Tab"/*, "Ctrl + T"*/))
			{
				Tab newTab;
				newTab.loadNewPage(std::make_shared<Page>(Page::newTabPage));
				tabs.push_back(newTab);
			}
			if (ImGui::MenuItem("New Window"/*, "Ctrl + N"*/))
			{
				app.newWindow();
			}
			ImGui::Separator();
			if (ImGui::BeginMenu("Bookmarks"))
			{
				ImGui::PushFont(fontRegular);

				std::vector<Bookmark> &bookmarks = userData.bookmarks;

				if (bookmarks.size() == 0)
				{
					ImGui::MenuItem("Bookmarks List is Empty.", nullptr, false, false);
				}
				else
				{
					for (size_t i = 0; i < bookmarks.size(); i++)
					{
						const Bookmark &bookmark = bookmarks[i];

						if (ImGui::MenuItem(bookmark.name.c_str()))
						{
							tab.loadNewPage(bookmark.url, true);
						}

						if (ImGui::IsItemHovered())
						{
							ImGui::BeginTooltip();
							ImGui::TextUnformatted(bookmark.name.c_str());
							ImGui::TextUnformatted(bookmark.url.c_str());
							ImGui::EndTooltip();
						}

						if (ImGui::BeginPopupContextItem())
						{
							if (ImGui::MenuItem("Remove from Bookmarks"))
							{
								bookmarks.erase(bookmarks.begin() + i--);
							}

							ImGui::EndPopup();
						}
					}
				}

				ImGui::PopFont();
				ImGui::EndMenu();
			}
			if (ImGui::MenuItem("Find in Page"/*, "Ctrl + F"*/))
			{
				// TODO: find functionality
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Settings"))
			{
				// TODO: settings tab
			}
			if (ImGui::MenuItem("About"))
			{
				// TODO: about page
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Exit"/*, "Alt + F4"*/))
			{
				exitRequested = true;
			}

			ImGui::EndPopup();
		}

		return exitRequested;
	}

	static bool drawToolbar(App &app, UserData &userData, Tab &tab, std::vector<Tab> &tabs)
	{
		constexpr ImVec2 toolbarButtonSize = {40.f, 0.f};

		// TODO: remember page position

		if (tab.hasPrevPage())
		{
			if (ImGui::Button(ICON_FA_ARROW_LEFT, toolbarButtonSize))
			{
				tab.prevPage();
			}
		}
		else
		{
			ImGui::BeginDisabled();
			ImGui::Button(ICON_FA_ARROW_LEFT, toolbarButtonSize);
			ImGui::EndDisabled();
		}
		ImGui::SameLine();

		if (tab.hasNextPage())
		{
			if (ImGui::Button(ICON_FA_ARROW_RIGHT, toolbarButtonSize))
			{
				tab.nextPage();
			}
		}
		else
		{
			ImGui::BeginDisabled();
			ImGui::Button(ICON_FA_ARROW_RIGHT, toolbarButtonSize);
			ImGui::EndDisabled();
		}
		ImGui::SameLine();

		std::shared_ptr<Page> page = tab.getCurrentPage();

		if (PageType pageType = page->getPageType(); pageType == PageType::NewTab || pageType == PageType::Settings)
		{
			ImGui::BeginDisabled();
			ImGui::Button(ICON_FA_REPEAT, toolbarButtonSize);
			ImGui::EndDisabled();
		}
		else if (page->isLoaded())
		{
			if (ImGui::Button(ICON_FA_REPEAT, toolbarButtonSize))
			{
				tab.loadCurrentPage();
			}
		}
		else
		{
			if (ImGui::Button(ICON_FA_TIMES, toolbarButtonSize))
			{
				// TODO: cancellation
			}
		}
		ImGui::SameLine();

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 50);
		if (ImGui::InputTextWithHint("", "Enter address", &tab.getAddressBarText(), ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_EnterReturnsTrue))
		{
			if (std::string &url = tab.getAddressBarText(); !url.empty())
			{
				if (url.find("://") == std::string::npos)
				{
					url = "gemini://" + url;
				}

				tab.loadNewPage(url, true);
			}
		}
		ImGui::SameLine();

		if (ImGui::Button(ICON_FA_BARS, toolbarButtonSize))
		{
			ImGui::OpenPopup("Menu");
		}

		ImVec2 max = ImGui::GetItemRectMax();
		ImGui::SetNextWindowPos(max, ImGuiCond_Always, {1.f, 0.f});

		return drawSideMenu(app, userData, tab, tabs);
	}

	static void drawTabMenu(UserData &userData,
		std::vector<Tab> &tabs,
		uint32_t currentTabIndex,
		int32_t &forceSelectedTabIndex,
		std::unordered_set<uint32_t> &tabsToRemoveIndices
	)
	{
		if (ImGui::BeginPopupContextItem())
		{
			Tab &tab = tabs[currentTabIndex];

			if (ImGui::MenuItem("New Tab"))
			{
				Tab newTab;
				newTab.loadNewPage(std::make_shared<Page>(Page::newTabPage));
				tabs.push_back(newTab);
			}
			ImGui::Separator();

			if (ImGui::MenuItem("Reload Tab"))
			{
				tab.loadCurrentPage();
			}
			if (ImGui::MenuItem("Duplicate Tab"))
			{
				tabs.insert(tabs.begin() + currentTabIndex + 1, tab);
				forceSelectedTabIndex = currentTabIndex + 1;
			}
			ImGui::Separator();

			if (ImGui::MenuItem("Bookmark Tab"))
			{
				auto page = tab.getCurrentPage();
				userData.bookmarks.push_back({std::string(page->getLabel()), std::string(page->getUrl())});
			}
			ImGui::Separator();

			if (ImGui::MenuItem("Close Tab"))
			{
				tabsToRemoveIndices.insert(currentTabIndex);
			}
			if (ImGui::MenuItem("Close Tabs To Right"))
			{
				for (uint32_t i = currentTabIndex + 1; i < tabs.size(); i++)
				{
					tabsToRemoveIndices.insert(i);
				}
			}
			if (ImGui::MenuItem("Close Tabs To Left"))
			{
				for (uint32_t i = 0; i < currentTabIndex; i++)
				{
					tabsToRemoveIndices.insert(i);
				}
			}
			if (ImGui::MenuItem("Close Other Tabs"))
			{
				for (uint32_t i = 0; i < tabs.size(); i++)
				{
					if (currentTabIndex == i)
						continue;

					tabsToRemoveIndices.insert(i);
				}
			}
			if (ImGui::MenuItem("Close All Tabs"))
			{
				for (uint32_t i = 0; i < tabs.size(); i++)
				{
					tabsToRemoveIndices.insert(i);
				}
			}

			ImGui::EndPopup();
		}
	}

	static bool drawAppWindow(App &app, UserData &userData, std::vector<Tab> &tabs, int32_t &forceSelectedTabIndex)
	{
		constexpr ImGuiWindowFlags mainWindowFlags =
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoSavedSettings;
		constexpr ImGuiTabBarFlags tabBarFlags =
			ImGuiTabBarFlags_AutoSelectNewTabs |
			ImGuiTabBarFlags_NoCloseWithMiddleMouseButton |
			ImGuiTabBarFlags_FittingPolicyResizeDown;

		ImGui::SetNextWindowPos({0, 0});
		ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
		ImGui::Begin("MainWindow", nullptr, mainWindowFlags);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {10.f, 10.f});
		ImGui::BeginTabBar("MainTabBar", tabBarFlags);

		std::unordered_set<uint32_t> tabsToRemoveIndices;
		bool exitRequested = false;

		//TODO: increase the maximum number of displayed tabs
		for (uint32_t i = 0; i < tabs.size(); i++)
		{
			if (!tabs[i].isOpen())
			{
				tabsToRemoveIndices.insert(i);
				continue;
			}

			ImGuiTabBarFlags tabFlags = ImGuiTabItemFlags_NoPushId;

			// select the newly duplicated tab
			if (static_cast<uint32_t>(forceSelectedTabIndex) == i)
			{
				tabFlags |= ImGuiTabItemFlags_SetSelected;
				forceSelectedTabIndex = -1;
			}

			bool isOpen = tabs[i].isOpen();
			std::shared_ptr<Page> page = tabs[i].getCurrentPage();
			std::string_view label = page->getLabel().empty() ? page->getUrl() : page->getLabel();

			ImGui::SetNextItemWidth(200);
			ImGui::PushFont(fontRegular);
			ImGui::PushID(i);
			bool isTabSelected = ImGui::BeginTabItem(label.data(), &isOpen, tabFlags);
			ImGui::PopID();
			ImGui::PopFont();

			tabs[i].setOpen(isOpen);

			drawTabMenu(userData, tabs, i, forceSelectedTabIndex, tabsToRemoveIndices);

			if (isTabSelected)
			{
				// Toolbar
				exitRequested = drawToolbar(app, userData, tabs[i], tabs);

				// Page
				drawPage(tabs, i);

				ImGui::EndTabItem();
			}
		}

		if (ImGui::TabItemButton(ICON_FA_PLUS, ImGuiTabItemFlags_Trailing))
		{
			Tab newTab;
			newTab.loadNewPage(std::make_shared<Page>(Page::newTabPage));
			tabs.push_back(newTab);
		}

		ImGui::EndTabBar();
		ImGui::PopStyleVar();
		ImGui::End();

		removeClosedTabs(tabs, tabsToRemoveIndices);

		return exitRequested;
	}

	static void setThemeDeepDark()
	{
		ImVec4 *colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.19f, 0.19f, 0.19f, 0.92f);
		colors[ImGuiCol_Border] = ImVec4(0.19f, 0.19f, 0.19f, 0.29f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.24f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.54f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
		colors[ImGuiCol_CheckMark] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
		colors[ImGuiCol_Button] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
		colors[ImGuiCol_Header] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 0.00f, 0.00f, 0.36f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.22f, 0.23f, 0.33f);
		colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
		colors[ImGuiCol_Tab] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.20f, 0.20f, 0.36f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_TableHeaderBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
		colors[ImGuiCol_TableBorderStrong] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
		colors[ImGuiCol_TableBorderLight] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
		colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
		colors[ImGuiCol_NavHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.35f);
	}

	static void setDefaultStyles()
	{
		ImGuiStyle &style = ImGui::GetStyle();
		style.FramePadding = ImVec2(5.f, 5.f);
		style.CellPadding = ImVec2(5.f, 5.f);
		style.ItemSpacing = ImVec2(5.f, 5.f);
		style.ItemInnerSpacing = ImVec2(5.f, 5.f);
		style.ScrollbarSize = 15.f;
		style.GrabMinSize = 10.f;
		style.WindowBorderSize = 0.f;
		style.WindowRounding = 0.f;
		style.ChildRounding = 0.f;
		style.FrameRounding = 4.f;
		style.PopupRounding = 4.f;
		style.TabRounding = 4.f;
	}
}

inline constexpr SDL_WindowFlags operator|(SDL_WindowFlags a, SDL_WindowFlags b)
{
	return static_cast<SDL_WindowFlags>(static_cast<int>(a) | static_cast<int>(b));
}

AppWindow::AppWindow(App *app, AppContext *context) :
	_app {app},
	_appContext {context}
{
	const Settings &settings = _appContext->settings;

	_positionX = settings.windowPosition.x;
	_positionY = settings.windowPosition.y;
	_width = settings.windowResolution.x;
	_height = settings.windowResolution.y;
	_displayIndex = settings.displayIndex;
	_isMaximized = settings.displayMode == DisplayMode::Maximized;

	// GL 3.0 + GLSL 130
	static constexpr const char *glslVersion = "#version 130";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	static constexpr SDL_WindowFlags windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;

	_window = SDL_CreateWindow("gem", SDL_WINDOWPOS_CENTERED_DISPLAY(_displayIndex), SDL_WINDOWPOS_CENTERED_DISPLAY(_displayIndex), _width, _height, windowFlags);

	if (_positionX == -1 || _positionY == -1)
	{
		SDL_GetWindowPosition(_window, &_positionX, &_positionY);
	}
	else
	{
		SDL_SetWindowPosition(_window, _positionX, _positionY);
	}

	if (_isMaximized)
	{
		SDL_MaximizeWindow(_window);
	}

	_windowId = SDL_GetWindowID(_window);
	_glContext = SDL_GL_CreateContext(_window);

	SDL_GL_MakeCurrent(_window, _glContext);
	SDL_GL_SetSwapInterval(1); // Enable vsync

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	_imguiContext = ImGui::CreateContext(&fontAtlas);
	ImGui::SetCurrentContext(_imguiContext);

	ImGui_ImplSDL2_InitForOpenGL(_window, _glContext);
	ImGui_ImplOpenGL3_Init(glslVersion);

	ImGui::GetIO().IniFilename = nullptr;

	setDefaultStyles();
	setThemeDeepDark();

	_isVisible = true;
	SDL_ShowWindow(_window);

	Tab newTab;
	newTab.loadNewPage(std::make_shared<Page>(Page::newTabPage));
	_tabs.push_back(newTab);
}

AppWindow::AppWindow(AppWindow &&other) noexcept
{
	*this = std::move(other);
}

AppWindow::~AppWindow()
{
	if (_imguiContext != nullptr)
	{
		ImGui::SetCurrentContext(_imguiContext);
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext();
	}

	SDL_GL_DeleteContext(_glContext);
	SDL_DestroyWindow(_window);
}

AppWindow &AppWindow::operator=(AppWindow &&other)
{
	if (this != &other)
	{
		_app = other._app;
		_appContext = other._appContext;
		_tabs = std::move(other._tabs);
		_window = other._window;
		_windowId = other._windowId;
		_glContext = other._glContext;
		_imguiContext = other._imguiContext;
		_positionX = other._positionX;
		_positionY = other._positionY;
		_width = other._width;
		_height = other._height;
		_displayIndex = other._displayIndex;
		_isMaximized = other._isMaximized;
		_isVisible = other._isVisible;

		other._app = nullptr;
		other._appContext = nullptr;
		other._window = nullptr;
		other._glContext = nullptr;
		other._imguiContext = nullptr;
	}

	return *this;
}

void AppWindow::handleEvent(SDL_Event &e)
{
	if (e.window.windowID == _windowId)
	{
		ImGui::SetCurrentContext(_imguiContext);
		ImGui_ImplSDL2_ProcessEvent(&e);

		if (e.type == SDL_WINDOWEVENT)
		{
			switch (e.window.event)
			{
				case SDL_WINDOWEVENT_MOVED:
					_positionX = e.window.data1;
					_positionY = e.window.data2;
					break;
				case SDL_WINDOWEVENT_SIZE_CHANGED:
					_width = e.window.data1;
					_height = e.window.data2;
					break;
				case SDL_WINDOWEVENT_MAXIMIZED:
					_isMaximized = true;
					break;
				case SDL_WINDOWEVENT_RESTORED:
					_isMaximized = false;
					break;
				case SDL_WINDOWEVENT_CLOSE:
					_isVisible = false;
					SDL_HideWindow(_window);
					break;
				case SDL_WINDOWEVENT_DISPLAY_CHANGED:
					_displayIndex = e.window.data1;
					break;
				default:
					break;
			}
		}
	}
}

void AppWindow::update()
{
	if (_tabs.empty())
	{
		_isVisible = false;
	}
}

void AppWindow::render()
{
	SDL_GL_MakeCurrent(_window, _glContext);
	ImGui::SetCurrentContext(_imguiContext);
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();

	if (drawAppWindow(*_app, _appContext->userData, _tabs, _forceSelectedTabIndex))
	{
		_isVisible = false;
	}

	//ImGui::ShowDemoWindow();

	ImGui::Render();
	glViewport(0, 0, _width, _height);
	glClearColor(0.f, 0.f, 0.f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	SDL_GL_SwapWindow(_window);
}

void AppWindow::saveContext()
{
	Settings &settings = _appContext->settings;
	settings.displayIndex = _displayIndex;
	settings.displayMode = _isMaximized ? DisplayMode::Maximized : DisplayMode::Windowed;

	if (_isMaximized)
	{
		// HACK: restore the window to get normal position and size
		SDL_RestoreWindow(_window);
		SDL_GetWindowPosition(_window, &_positionX, &_positionY);
		SDL_GetWindowSize(_window, &_width, &_height);
	}

	settings.windowPosition = {_positionX, _positionY};
	settings.windowResolution = {_width, _height};
}

void AppWindow::loadFonts()
{
	constexpr const char *fontPathRegular = "assets/fonts/NotoSansMono-Regular.ttf";
	constexpr const char *fontPathMono = "assets/fonts/NotoSansMono-Regular.ttf";
	constexpr const char *fontPathEmoji = "assets/fonts/NotoEmoji-Regular.ttf";
	constexpr const char *fontPathIcons = "assets/fonts/fontawesome-webfont.ttf";

	constexpr float fontSizeInternal = 20.f;
	constexpr float fontSizeRegular = 20.f;
	constexpr float fontSizeMono = fontSizeRegular;
	constexpr float fontSizeH1 = 32.f;
	constexpr float fontSizeH2 = 28.f;
	constexpr float fontSizeH3 = 24.f;

	assert(std::filesystem::exists(fontPathRegular));
	assert(std::filesystem::exists(fontPathMono));
	assert(std::filesystem::exists(fontPathEmoji));
	assert(std::filesystem::exists(fontPathIcons));

	std::ifstream ifs;
	ifs.open(fontPathRegular, std::ios::binary | std::ios::ate);
	int fontDataSizeRegular = static_cast<int>(ifs.tellg());
	char *fontDataRegular = new char[fontDataSizeRegular];
	ifs.seekg(0, std::ios::beg);
	ifs.read(fontDataRegular, fontDataSizeRegular);
	ifs.close();
	ifs.open(fontPathEmoji, std::ios::binary | std::ios::ate);
	int fontDataSizeEmoji = static_cast<int>(ifs.tellg());
	char *fontDataEmoji = new char[fontDataSizeEmoji];
	ifs.seekg(0, std::ios::beg);
	ifs.read(fontDataEmoji, fontDataSizeEmoji);
	ifs.close();

	static const ImWchar fontRangesRegular[] = {0x1, 0xFFFF, 0};
	static ImFontConfig fontConfigRegular;
	fontConfigRegular.FontDataOwnedByAtlas = false;

	static const ImWchar fontRangesEmoji[] = {0x1, 0x1FFFF, 0};
	static ImFontConfig fontConfigEmoji;
	fontConfigEmoji.FontDataOwnedByAtlas = false;
	fontConfigEmoji.MergeMode = true;
	fontConfigEmoji.OversampleH = fontConfigEmoji.OversampleV = 1;
	fontConfigEmoji.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_LoadColor;

	static const ImWchar fontRangesIcons[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
	static ImFontConfig fontConfigIcons;
	fontConfigIcons.MergeMode = true;

	fontInternal = fontAtlas.AddFontFromMemoryTTF(fontDataRegular, fontDataSizeRegular, fontSizeInternal, &fontConfigRegular, fontRangesRegular);
	fontAtlas.AddFontFromFileTTF(fontPathIcons, fontSizeInternal, &fontConfigIcons, fontRangesIcons);

	fontRegular = fontAtlas.AddFontFromMemoryTTF(fontDataRegular, fontDataSizeRegular, fontSizeRegular, &fontConfigRegular, fontRangesRegular);
	fontAtlas.AddFontFromMemoryTTF(fontDataEmoji, fontDataSizeEmoji, fontSizeRegular, &fontConfigEmoji, fontRangesEmoji);

	fontMono = fontAtlas.AddFontFromFileTTF(fontPathMono, fontSizeMono, nullptr, fontRangesRegular);
	fontAtlas.AddFontFromMemoryTTF(fontDataEmoji, fontDataSizeEmoji, fontSizeMono, &fontConfigEmoji, fontRangesEmoji);

	fontH1 = fontAtlas.AddFontFromMemoryTTF(fontDataRegular, fontDataSizeRegular, fontSizeH1, &fontConfigRegular, fontRangesRegular);
	fontAtlas.AddFontFromMemoryTTF(fontDataEmoji, fontDataSizeEmoji, fontSizeH1, &fontConfigEmoji, fontRangesEmoji);

	fontH2 = fontAtlas.AddFontFromMemoryTTF(fontDataRegular, fontDataSizeRegular, fontSizeH2, &fontConfigRegular, fontRangesRegular);
	fontAtlas.AddFontFromMemoryTTF(fontDataEmoji, fontDataSizeEmoji, fontSizeH2, &fontConfigEmoji, fontRangesEmoji);

	fontH3 = fontAtlas.AddFontFromMemoryTTF(fontDataRegular, fontDataSizeRegular, fontSizeH3, &fontConfigRegular, fontRangesRegular);
	fontAtlas.AddFontFromMemoryTTF(fontDataEmoji, fontDataSizeEmoji, fontSizeH3, &fontConfigEmoji, fontRangesEmoji);

	fontAtlas.Build();

	delete[] fontDataRegular;
	delete[] fontDataEmoji;
}