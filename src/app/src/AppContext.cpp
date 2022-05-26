#include "AppContext.hpp"

#include <fstream>
#include <filesystem>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>

using namespace gem;

namespace
{
	static constexpr const char *DisplayModeNames[] = {"Windowed", "Maximized"};

	static DisplayMode stringToDisplayMode(const char *name)
	{
		static constexpr std::size_t DisplayModeNamesSize = sizeof(DisplayModeNames) / sizeof(*DisplayModeNames);

		auto it = std::find_if(DisplayModeNames, DisplayModeNames + DisplayModeNamesSize, [name](const char *str) { return strcmp(str, name) == 0; });

		if (std::size_t index = std::distance(DisplayModeNames, it); index < DisplayModeNamesSize)
		{
			return static_cast<DisplayMode>(index);
		}

		return DisplayMode::Windowed;
	}
}

void UserData::save(std::string_view path)
{
	rapidjson::StringBuffer buffer;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);

	writer.StartObject();

	writer.Key("bookmarks");
	writer.StartArray();

	for (const Bookmark &bookmark : bookmarks)
	{
		writer.StartObject();
		writer.Key("name");
		writer.String(bookmark.name);
		writer.Key("url");
		writer.String(bookmark.url);
		writer.EndObject();
	}

	writer.EndArray();

	writer.EndObject();

	std::ofstream ofs(path.data(), std::ios::binary);
	ofs.write(buffer.GetString(), buffer.GetSize());
}

void UserData::load(std::string_view path)
{
	if (path.empty() || !std::filesystem::exists(path))
	{
		return;
	}

	std::ifstream ifs(path.data(), std::ios::binary | std::ios::ate);
	std::size_t size = ifs.tellg();
	char *buffer = new char[size];
	ifs.seekg(0, std::ios::beg);
	ifs.read(buffer, size);

	rapidjson::Document doc;
	doc.Parse(buffer, size);

	const auto bookmarksArray = doc["bookmarks"].GetArray();
	bookmarks.clear();
	bookmarks.reserve(bookmarksArray.Size());

	for (const auto &bookmarkObject : bookmarksArray)
	{
		bookmarks.push_back(
			{
				bookmarkObject["name"].GetString(),
				bookmarkObject["url"].GetString()
			});
	}
}

void Settings::save(std::string_view path)
{
	rapidjson::StringBuffer buffer;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);

	writer.StartObject();

	writer.Key("windowPosition");
	writer.StartObject();
	writer.Key("x");
	writer.Int(windowPosition.x);
	writer.Key("y");
	writer.Int(windowPosition.y);
	writer.EndObject();

	writer.Key("windowResolution");
	writer.StartObject();
	writer.Key("x");
	writer.Int(windowResolution.x);
	writer.Key("y");
	writer.Int(windowResolution.y);
	writer.EndObject();

	writer.Key("displayIndex");
	writer.Uint(displayIndex);
	writer.Key("displayMode");
	writer.String(DisplayModeNames[static_cast<int>(displayMode)]);

	writer.EndObject();

	std::ofstream ofs(path.data(), std::ios::binary);
	ofs.write(buffer.GetString(), buffer.GetSize());
}

void Settings::load(std::string_view path)
{
	if (path.empty() || !std::filesystem::exists(path))
	{
		return;
	}

	std::ifstream ifs(path.data(), std::ios::binary | std::ios::ate);
	std::size_t size = ifs.tellg();
	char *buffer = new char[size];
	ifs.seekg(0, std::ios::beg);
	ifs.read(buffer, size);

	rapidjson::Document doc;
	doc.Parse(buffer, size);

	if (doc.HasMember("windowPosition"))
	{
		auto windowPositionObject = doc["windowPosition"].GetObject();
		windowPosition.x = windowPositionObject["x"].GetInt();
		windowPosition.y = windowPositionObject["y"].GetInt();
	}

	if (doc.HasMember("windowResolution"))
	{
		auto windowResolutionObject = doc["windowResolution"].GetObject();
		windowResolution.x = windowResolutionObject["x"].GetInt();
		windowResolution.y = windowResolutionObject["y"].GetInt();
	}

	displayIndex = doc["displayIndex"].GetUint();
	displayMode = stringToDisplayMode(doc["displayMode"].GetString());
}