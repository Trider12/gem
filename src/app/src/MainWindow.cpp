#include "MainWindow.hpp"

#include <fstream>
#include <filesystem>
#include <unordered_set>

#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_freetype.h>

#include <IconsFontAwesome4.h>

using namespace gem;

namespace
{
	static ImFont *fontDefault = nullptr,
		*fontMono = nullptr,
		*fontIcons = nullptr,
		*fontH1 = nullptr,
		*fontH2 = nullptr,
		*fontH3 = nullptr;

	int duplicatedTabIndex = -1;
	std::unordered_set<int> tabsToRemoveIndices;

	static void removeClosedTabs(std::vector<std::shared_ptr<gem::Tab>> &tabs)
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

	void drawLink(const Line &line)
	{
		static const ImU32 linkColorU32 = ImGui::GetColorU32({0.f, 0.f, 238.f / 255.f, 1.f});
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
		ImGui::PushStyleColor(ImGuiCol_Border, transparentColorU32);
		ImGui::PushStyleColor(ImGuiCol_BorderShadow, transparentColorU32);
		ImGui::SmallButton(start, end);
		ImGui::PopStyleColor(6);

		if (ImGui::IsItemHovered())
		{
			ImVec2 min = ImGui::GetItemRectMin();
			ImVec2 max = ImGui::GetItemRectMax();
			min.y = max.y;
			ImGui::GetWindowDrawList()->AddLine(min, max, linkColorU32, 1.0f);
		}
	}

	void drawText(const Line &line)
	{
		ImGui::PushTextWrapPos(0.0f); // enable word wrapping
		ImGui::TextUnformatted(line.text.data(), line.text.data() + line.text.size());
		ImGui::PopTextWrapPos();
	}

	void drawBlock(const Line &line, size_t index)
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

	void drawHeader(const Line &line, ImFont *font)
	{
		ImGui::PushFont(font);
		drawText(line);
		ImGui::PopFont();
	}

	void drawList(const Line &line)
	{
		ImGui::Bullet();
		drawText(line);
	}

	void drawQuote(const Line &line)
	{
		const ImU32 quoteBgColor = ImGui::GetColorU32(ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
		static const ImU32 quoteLineColor = ImGui::GetColorU32({1.f, 1.f, 1.f, 1.f});
		static constexpr float quotePadding = 5.f;

		ImDrawList *drawList = ImGui::GetWindowDrawList();
		ImDrawListSplitter splitter;
		splitter.Split(drawList, 2);

		splitter.SetCurrentChannel(drawList, 1);
		ImGui::Indent(2 * quotePadding);
		drawText(line);
		ImGui::Unindent(2 * quotePadding);

		splitter.SetCurrentChannel(drawList, 0);
		ImVec2 min = ImGui::GetItemRectMin();
		ImVec2 max = ImGui::GetItemRectMax();
		min.x -= 2 * quotePadding;
		drawList->AddRectFilled(min, {min.x + quotePadding, max.y}, quoteLineColor);
		min.x += quotePadding;
		drawList->AddRectFilled(min, max, quoteBgColor);

		splitter.Merge(drawList);
	}

	void drawPage(const Tab &tab)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {0.f, 0.f});
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0.f, 0.f});
		ImGui::BeginChild("Page");

		for (size_t i = 0; i < tab.lines.size(); i++)
		{
			const Line &line = tab.lines[i];

			switch (line.type)
			{
				case LineType::Text:
					drawText(line);
					break;
				case LineType::Link:
					drawLink(line);
					break;
				case LineType::Block:
					drawBlock(line, i);
					break;
				case LineType::Header1:
					drawHeader(line, fontH1);
					break;
				case LineType::Header2:
					drawHeader(line, fontH2);
					break;
				case LineType::Header3:
					drawHeader(line, fontH3);
					break;
				case LineType::List:
					drawList(line);
					break;
				case LineType::Quote:
					drawQuote(line);
					break;
				default:
					assert(false);
			}
		}

		ImGui::EndChild();
		ImGui::PopStyleVar(2);
	}

	void drawToolbar(std::shared_ptr<Tab> tab, const gui::LoadLinkCallback &callback)
	{
		ImGui::PushFont(fontIcons);

		ImGui::Button(ICON_FA_ARROW_LEFT, {40.f, 0.f});
		ImGui::SameLine();
		ImGui::Button(ICON_FA_ARROW_RIGHT, {40.f, 0.f});
		ImGui::SameLine();

		if (ImGui::Button(ICON_FA_REPEAT, {40.f, 0.f}))
		{
			if (callback)
			{
				callback(tab);
			}
		}
		ImGui::SameLine();

		ImGui::PopFont();

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 50);
		if (ImGui::InputTextWithHint("", "Enter address", &tab->url, ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_EnterReturnsTrue))
		{
			if (callback)
			{
				callback(tab);
			}
		}
		ImGui::SameLine();

		ImGui::PushFont(fontIcons);

		ImGui::Button(ICON_FA_COG, {40.f, 0.f});

		ImGui::PopFont();
	}

	void drawContextMenu(std::vector<std::shared_ptr<Tab>> &tabs, int index, const gui::LoadLinkCallback &callback)
	{
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Reload"))
			{
				if (callback)
				{
					callback(tabs[index]);
				}
				ImGui::CloseCurrentPopup();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Duplicate"))
			{
				tabs.insert(tabs.begin() + index + 1, std::make_shared<Tab>(*tabs[index]));
				duplicatedTabIndex = index + 1;
				ImGui::CloseCurrentPopup();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Close Tab"))
			{
				tabsToRemoveIndices.insert(index);
				ImGui::CloseCurrentPopup();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Close All Tabs"))
			{
				for (int i = 0; i < tabs.size(); i++)
				{
					tabsToRemoveIndices.insert(i);
				}
				ImGui::CloseCurrentPopup();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Close All But This"))
			{
				for (int i = 0; i < tabs.size(); i++)
				{
					if (index == i)
						continue;

					tabsToRemoveIndices.insert(i);
				}

				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
	}
}

void gem::gui::drawMainWindow(std::vector<std::shared_ptr<Tab>> &tabs, const LoadLinkCallback &callback)
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

	for (int i = 0; i < tabs.size(); i++)
	{
		if (!tabs[i]->isOpen)
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

		ImGui::SetNextItemWidth(200);
		ImGui::PushID(i);
		bool isTabSelected = ImGui::BeginTabItem(tabs[i]->label.c_str(), &tabs[i]->isOpen, tabFlags);
		ImGui::PopID();

		drawContextMenu(tabs, i, callback);

		if (isTabSelected)
		{
			// Toolbar
			drawToolbar(tabs[i], callback);

			// Page
			drawPage(*tabs[i]);

			ImGui::EndTabItem();
		}
	}

	ImGui::PushFont(fontIcons);

	if (ImGui::TabItemButton(ICON_FA_PLUS, ImGuiTabItemFlags_Trailing))
	{
		tabs.push_back(std::make_shared<Tab>(emptyTab));
	}

	ImGui::PopFont();

	ImGui::EndTabBar();
	ImGui::PopStyleVar();
	ImGui::End();

	removeClosedTabs(tabs);
}

void gem::gui::loadFonts()
{
	constexpr const char *fontPathDefault = "assets/fonts/NotoSansMono-Regular.ttf";
	constexpr const char *fontPathMono = "assets/fonts/NotoSansMono-Regular.ttf";
	constexpr const char *fontPathEmoji = "assets/fonts/NotoEmoji-Regular.ttf";
	constexpr const char *fontPathIcons = "assets/fonts/fontawesome-webfont.ttf";

	constexpr float fontSizeH1 = 32.f;
	constexpr float fontSizeH2 = 28.f;
	constexpr float fontSizeH3 = 24.f;
	constexpr float fontSizeDefault = 20.f;
	constexpr float fontSizeMono = fontSizeDefault;

	assert(std::filesystem::exists(fontPathDefault));
	assert(std::filesystem::exists(fontPathMono));
	assert(std::filesystem::exists(fontPathEmoji));
	assert(std::filesystem::exists(fontPathIcons));

	std::ifstream ifs(fontPathDefault, std::ios::binary | std::ios::ate);
	int size = static_cast<int>(ifs.tellg());
	char *fontData = new char[size];
	ifs.seekg(0, std::ios::beg);
	ifs.read(fontData, size);
	ifs.close();

	static const ImWchar glyphRanges[] = {0x1, 0xFFFF, 0};
	static ImFontConfig fontConfig;
	fontConfig.FontDataOwnedByAtlas = false;

	static const ImWchar emojiRanges[] = {0x1, 0x1FFFF, 0};
	static ImFontConfig emojiFontConfig;
	emojiFontConfig.OversampleH = emojiFontConfig.OversampleV = 1;
	emojiFontConfig.MergeMode = true;
	emojiFontConfig.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_LoadColor;

	static const ImWchar iconsRanges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
	static ImFontConfig iconsFontConfig;
	iconsFontConfig.PixelSnapH = true;

	ImGuiIO &io = ImGui::GetIO();
	fontDefault = io.Fonts->AddFontFromMemoryTTF(fontData, size, fontSizeDefault, &fontConfig, glyphRanges);
	io.Fonts->AddFontFromFileTTF(fontPathEmoji, fontSizeDefault, &emojiFontConfig, emojiRanges);
	fontIcons = io.Fonts->AddFontFromFileTTF(fontPathIcons, fontSizeDefault, &iconsFontConfig, iconsRanges);
	fontMono = io.Fonts->AddFontFromFileTTF(fontPathMono, fontSizeMono, &fontConfig, glyphRanges);
	fontH1 = io.Fonts->AddFontFromMemoryTTF(fontData, size, fontSizeH1, &fontConfig, glyphRanges);
	fontH2 = io.Fonts->AddFontFromMemoryTTF(fontData, size, fontSizeH2, &fontConfig, glyphRanges);
	fontH3 = io.Fonts->AddFontFromMemoryTTF(fontData, size, fontSizeH3, &fontConfig, glyphRanges);

	delete[] fontData;
}

void gem::gui::setThemeDeepDark()
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
	//colors[ImGuiCol_DockingPreview] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
	//colors[ImGuiCol_DockingEmptyBg] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
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

	ImGuiStyle &style = ImGui::GetStyle();
	style.FramePadding = ImVec2(5.f, 5.f);
	style.CellPadding = ImVec2(5.f, 5.f);
	style.ItemSpacing = ImVec2(5.f, 5.f);
	style.ItemInnerSpacing = ImVec2(5.f, 5.f);
	style.TouchExtraPadding = ImVec2(5.f, 5.f);
	style.ScrollbarSize = 15.f;
	style.GrabMinSize = 10.f;
	style.WindowBorderSize = 0.f;
	style.WindowRounding = 0.f;
	style.ChildRounding = 0.f;
	style.FrameRounding = 4.f;
	style.PopupRounding = 4.f;
	style.TabRounding = 4.f;
}