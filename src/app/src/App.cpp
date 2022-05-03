#include "App.hpp"

#include <cstdio>
#include <filesystem>

#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_opengl3.h>
#include <IconsFontAwesome4.h>

#include <SDL.h>
#include <SDL_opengl.h>

using namespace gem;

void setImguiStyles()
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
	style.WindowPadding = ImVec2(8.00f, 8.00f);
	style.FramePadding = ImVec2(5.00f, 2.00f);
	style.CellPadding = ImVec2(6.00f, 6.00f);
	style.ItemSpacing = ImVec2(6.00f, 6.00f);
	style.ItemInnerSpacing = ImVec2(6.00f, 6.00f);
	style.TouchExtraPadding = ImVec2(0.00f, 0.00f);
	style.IndentSpacing = 25;
	style.ScrollbarSize = 15;
	style.GrabMinSize = 10;
	style.WindowBorderSize = 0.f;
	style.ChildBorderSize = 1;
	style.PopupBorderSize = 1;
	style.FrameBorderSize = 1;
	style.TabBorderSize = 1;
	style.WindowRounding = 0.f;
	style.ChildRounding = 4;
	style.FrameRounding = 3;
	style.PopupRounding = 4;
	style.ScrollbarRounding = 9;
	style.GrabRounding = 3;
	style.LogSliderDeadzone = 4;
	style.TabRounding = 4;
}

App::App(int width, int height)
{
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		fprintf(stderr, "SDL Error: %s\n", SDL_GetError());
		_isDone = true;
		return;
	}

	// GL 3.0 + GLSL 130
	const char *glslVersion = "#version 130";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

	// Create window with graphics context
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_WindowFlags windowFlags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	_window = SDL_CreateWindow("gem", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, windowFlags);
	_glContext = SDL_GL_CreateContext(_window);
	SDL_GL_MakeCurrent(_window, _glContext);
	SDL_GL_SetSwapInterval(1); // Enable vsync

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	const float fontSize = 20.f;

	ImGuiIO &io = ImGui::GetIO();
	//io.Fonts->AddFontDefault();
	io.Fonts->AddFontFromFileTTF("assets/fonts/Cousine-Regular.ttf", fontSize);

	static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
	ImFontConfig icons_config;
	icons_config.MergeMode = true;
	icons_config.PixelSnapH = true;

	io.Fonts->AddFontFromFileTTF("assets/fonts/fontawesome-webfont.ttf", fontSize, &icons_config, icons_ranges);

	io.IniFilename = nullptr;

	setImguiStyles();

	ImGui_ImplSDL2_InitForOpenGL(_window, _glContext);
	ImGui_ImplOpenGL3_Init(glslVersion);
}

App::~App()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DeleteContext(_glContext);
	SDL_DestroyWindow(_window);
	SDL_Quit();
}

namespace
{
	const ImVec4 clearColor = ImVec4();

	constexpr ImGuiWindowFlags mainWindowFlags =
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoSavedSettings;

	constexpr ImGuiTabBarFlags tabBarFlags =
		ImGuiTabBarFlags_Reorderable |
		ImGuiTabBarFlags_AutoSelectNewTabs |
		ImGuiTabBarFlags_NoCloseWithMiddleMouseButton |
		ImGuiTabBarFlags_FittingPolicyResizeDown;

	const Tab emptyTab {"New Tab", ""};

	static bool isFirstOpening = true;
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

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();

	drawMainWindow();

	//ImGui::ShowDemoWindow();
	//ImGui::ShowStyleEditor();

	ImGui::Render();
	ImGuiIO &io = ImGui::GetIO();
	glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
	glClearColor(clearColor.x * clearColor.w, clearColor.y * clearColor.w, clearColor.z * clearColor.w, clearColor.w);
	glClear(GL_COLOR_BUFFER_BIT);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	SDL_GL_SwapWindow(_window);
}

void App::drawMainWindow()
{
	ImGuiIO &io = ImGui::GetIO();

	ImGui::SetNextWindowPos({0, 0});
	ImGui::SetNextWindowSize(io.DisplaySize);
	ImGui::Begin("MainWindow", nullptr, mainWindowFlags);

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {10, 10});
	ImGui::BeginTabBar("MainTabBar", tabBarFlags);

	if (_tabs.empty())
	{
		if (isFirstOpening)
		{
			_tabs.push_back(emptyTab);
			isFirstOpening = false;
		}
		else
		{
			_isDone = true;
		}
	}

	for (auto it = _tabs.begin(); it != _tabs.end();)
	{
		if (!it->isOpen)
		{
			it = _tabs.erase(it);
			continue;
		}

		Tab &tab = *it;

		ImGui::PushID(&tab);

		ImGui::SetNextItemWidth(200);
		if (ImGui::BeginTabItem(tab.name.c_str(), &tab.isOpen, ImGuiTabItemFlags_NoPushId))
		{
			// Toolbar
			ImGui::Button(ICON_FA_ARROW_LEFT, {40.f, 0.f});
			ImGui::SameLine();
			ImGui::Button(ICON_FA_ARROW_RIGHT, {40.f, 0.f});
			ImGui::SameLine();
			ImGui::Button(ICON_FA_REPEAT, {40.f, 0.f});
			ImGui::SameLine();
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 50);
			ImGui::InputTextWithHint("", "Enter address", &tab.url);
			ImGui::SameLine();
			ImGui::Button(ICON_FA_COG, {40.f, 0.f});
			ImGui::Separator();

			// Page
			ImGui::TextWrapped("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed quis malesuada magna. Aenean ornare massa at enim gravida, eu congue quam viverra. Etiam dui mauris, imperdiet sit amet felis tempus, interdum sollicitudin nunc. Suspendisse efficitur efficitur massa quis suscipit. Aliquam dignissim, massa aliquet dapibus fringilla, metus erat mollis mauris, iaculis semper metus nulla at mauris. Aenean ut accumsan purus. Morbi efficitur ligula at enim cursus pretium et quis ex.");

			ImGui::EndTabItem();
		}

		ImGui::PopID();

		++it;
	}

	if (ImGui::TabItemButton(ICON_FA_PLUS, ImGuiTabItemFlags_Trailing))
	{
		_tabs.push_back(emptyTab);
	}

	ImGui::EndTabBar();
	ImGui::PopStyleVar();

	ImGui::End();
}