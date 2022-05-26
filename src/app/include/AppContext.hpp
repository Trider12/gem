#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace gem
{
	struct Bookmark
	{
		std::string name;
		std::string url;
	};

	struct UserData
	{
		void save(std::string_view path);
		void load(std::string_view path);

		std::vector<Bookmark> bookmarks;
	};

	struct Point
	{
		int32_t x, y;
	};

	enum class DisplayMode : uint8_t
	{
		Windowed = 0,
		Maximized = 1
	};

	struct Settings
	{
		void save(std::string_view path);
		void load(std::string_view path);

		Point windowPosition {-1, -1}; // center
		Point windowResolution {1280, 720};
		uint32_t displayIndex {0};
		DisplayMode displayMode {DisplayMode::Windowed};
	};

	struct AppContext
	{
		Settings settings;
		UserData userData;
	};
}