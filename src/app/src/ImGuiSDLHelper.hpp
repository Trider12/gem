#pragma once

#include <SDL.h>
#include <SDL_opengl.h>

#include <imgui_impl_sdl.h>
#include <imgui_impl_opengl3.h>

namespace gem::ImGuiSDL
{
	static bool init(SDL_Window *&window, void *&glContext, int width, int height)
	{
		if (SDL_Init(SDL_INIT_VIDEO) != 0)
		{
			fprintf(stderr, "SDL Error: %s\n", SDL_GetError());
			return false;
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
		window = SDL_CreateWindow("gem", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, windowFlags);
		glContext = SDL_GL_CreateContext(window);
		SDL_GL_MakeCurrent(window, glContext);
		SDL_GL_SetSwapInterval(1); // Enable vsync

		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGui_ImplSDL2_InitForOpenGL(window, glContext);
		ImGui_ImplOpenGL3_Init(glslVersion);

		return true;
	}

	static void shutdown(SDL_Window *&window, void *&glContext)
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext();

		SDL_GL_DeleteContext(glContext);
		SDL_DestroyWindow(window);
		SDL_Quit();
	}

	static void newFrame()
	{
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();
	}

	static void render(SDL_Window *&window)
	{
		const ImVec4 clearColor = ImVec4();

		ImGui::Render();
		ImGuiIO &io = ImGui::GetIO();
		glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
		glClearColor(clearColor.x * clearColor.w, clearColor.y * clearColor.w, clearColor.z * clearColor.w, clearColor.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		SDL_GL_SwapWindow(window);
	}
}