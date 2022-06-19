#pragma once

#include "StatusCode.hpp"
#include "GeminiClient.hpp"
#include "GemtextParser.hpp"

namespace gem
{
	enum class PageType : uint8_t
	{
		None = 0, // page is loading
		Gemtext,
		Text,
		Image,
		Unsupported, // file format is not supported == dowload file

		NewTab = 100,
		Settings
	};

	struct PageData
	{
	};

	struct GemtextPageData : public PageData
	{
		std::vector<GemtextLine> lines;
	};

	struct ImagePageData : public PageData
	{
		int imageWidth, imageHeight;
		unsigned int textureId;
	};

	class Page : public std::enable_shared_from_this<Page>
	{
	public:
		Page() = default;
		Page(std::string url);
		Page(const Page &page);
		~Page();

		std::string_view getUrl();
		std::string_view getLabel();
		std::string_view getData();
		std::string_view getError();

		PageType getPageType();

		template<typename T>
		T *getPageData()
		{
			static_assert(std::is_base_of_v<PageData, T>);
			return static_cast<T *>(_pageData);
		}

		bool isLoaded();
		bool isDownloaded();

		void load();
		void download(const char *path);

		static const Page newTabPage;

	private:
		Page(PageType type, std::string_view label);

		void init(StatusCode code, std::string meta, std::shared_ptr<std::vector<char>> data);
		void setError(GeminiClient::ClientCode code);

		static void connectAsyncCallback(std::shared_ptr<GeminiClient> client, std::weak_ptr<Page> pageWeakPtr, GeminiClient::ClientCode clientCode);
		static void receiveResponseHeaderAsyncCallback(std::shared_ptr<GeminiClient> client, std::weak_ptr<Page> pageWeakPtr, GeminiClient::ClientCode clientCode, StatusCode statusCode, std::string meta);
		static void receiveResponseBodyAsyncCallback(std::weak_ptr<Page> pageWeakPtr, StatusCode statusCode, std::string meta, GeminiClient::ClientCode clientCode, std::shared_ptr<std::vector<char>> data);

		std::string _url;
		std::string _label;
		PageType _pageType {PageType::None};
		PageData *_pageData {nullptr};

		bool _isLoaded {false};
		bool _isDownloaded {false};

		StatusCode _code {StatusCode::NONE};
		std::string _error;
		std::string _meta;
		std::shared_ptr<std::vector<char>> _binaryData;
	};
}