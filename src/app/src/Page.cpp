#include "Page.hpp"

#include "Utilities.hpp"

#include <cassert>
#include <fstream>

#include <stb_image.h>
#include <SDL_opengl.h>

using namespace gem;

namespace
{
	[[maybe_unused]] static bool isMimeSupported(std::string_view mime)
	{
		return stringStartsWith(mime, "text") ||
			(stringStartsWith(mime, "image") &&
				(stringStartsWith(&mime[6], "png") || stringStartsWith(&mime[6], "jpeg") || stringStartsWith(&mime[6], "gif")));
	}

	static inline void clearPageData(PageType type, PageData *data)
	{
		if (data != nullptr)
		{
			switch (type)
			{
				case PageType::Image:
					glDeleteTextures(1, &static_cast<ImagePageData *>(data)->textureId);
					break;
				default:
					break;
			}
		}

		delete data;
	}

	static void loadImageFromMemory(unsigned char *data, int size, int &width, int &height, unsigned int &textureId)
	{
		unsigned char *imageData = stbi_load_from_memory(data, size, &width, &height, nullptr, 4);

		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_2D, textureId);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);

		stbi_image_free(imageData);
	}
}

const Page Page::newTabPage = Page(PageType::NewTab, "New Tab");

Page::Page(std::string url) : _url {url}
{
}

Page::Page(PageType type, std::string_view label) : 
	_label {label},
	_pageType {type}
{
}

Page::Page(const Page &page) :
	_url {page._url},
	_label {page._label},
	_pageType {page._pageType},
	_pageData {nullptr},
	_isLoaded {page._isLoaded},
	_isDownloaded {page._isDownloaded},
	_code {page._code},
	_error {page._error},
	_meta {page._meta},
	_binaryData {page._binaryData}
{
}

Page::~Page()
{
	clearPageData(_pageType, _pageData);
}

std::string_view Page::getUrl()
{
	return _url;
}

std::string_view Page::getLabel()
{
	return _label;
}

std::string_view Page::getData()
{
	return std::string_view(_binaryData->data(), _binaryData->size());
}

std::string_view Page::getError()
{
	return _error;
}

PageType Page::getPageType()
{
	return _pageType;
}

bool Page::isLoaded()
{
	return _isLoaded;
}

bool Page::isDownloaded()
{
	return _isDownloaded;
}

void Page::load()
{
	_isLoaded = false;
	_isDownloaded = false;

	std::shared_ptr<GeminiClient> client = std::make_shared<GeminiClient>();
	client->connectAsync(std::bind(&connectAsyncCallback, client, weak_from_this(), std::placeholders::_1), _url, 1965);
}

void Page::download(const char *path)
{
	_isDownloaded = true;

	if (path == nullptr)
	{
		return;
	}

	std::ofstream ofs(path, std::ios::binary);

	if (ofs)
	{
		ofs.write(_binaryData->data(), _binaryData->size());
	}
	else
	{
		fprintf(stderr, "Failed to save file to the directory \"%s\"\n", path);
	}
}

void Page::init(StatusCode code, std::string meta, std::shared_ptr<std::vector<char>> data)
{
	_code = code;
	_meta = meta;
	_isLoaded = true;

	clearPageData(_pageType, _pageData);

	if (_code == StatusCode::SUCCESS)
	{
		_binaryData = data;

		if (*_url.rbegin() == '/')
		{
			_label = _url;
		}
		else
		{
			_label = _url.substr(_url.find_last_of('/') + 1);
		}
	}
	else
	{
		_error = std::to_string(static_cast<int>(code)) + " " + statusCodeToString(code);
		_label = _error;
		return;
	}

	if (!_binaryData)
	{
		return;
	}

	_pageType = PageType::Unsupported;

	if (stringStartsWith(meta, "text"))
	{
		_pageType = PageType::Text;

		if (stringStartsWith(&meta[5], "gemini"))
		{
			_pageType = PageType::Gemtext;

			GemtextPageData *gemtextPageData = new GemtextPageData();
			_pageData = gemtextPageData;
			GemtextParser::parse(gemtextPageData->lines, *_binaryData);

			for (const GemtextLine &line : gemtextPageData->lines)
			{
				if (line.type != GemtextLineType::Block && !line.text.empty())
				{
					_label = line.text;
					break;
				}
			}
		}
	}
	else if (stringStartsWith(meta, "image"))
	{
		if (stringStartsWith(&meta[6], "png") || stringStartsWith(&meta[6], "jpeg") || stringStartsWith(&meta[6], "gif"))
		{
			_pageType = PageType::Image;

			ImagePageData *imagePageData = new ImagePageData();
			_pageData = imagePageData;

			loadImageFromMemory(
				reinterpret_cast<unsigned char *>(_binaryData->data()),
				static_cast<int>(_binaryData->size()),
				imagePageData->imageWidth,
				imagePageData->imageHeight,
				imagePageData->textureId
			);
		}
	}
}

void Page::setError(GeminiClient::ClientCode code)
{
	switch (code)
	{
		case GeminiClient::ClientCode::SUCCESS:
			break;
		case GeminiClient::ClientCode::HOST_NAME_RESOLUTION_ERROR:
		case GeminiClient::ClientCode::CONNECTION_ERROR:
			_error = "Server not found.\nCheck if the address is correct or you have a network connection.";
			break;
		case GeminiClient::ClientCode::TLS_HANDSHAKE_ERROR:
			_error = "TLS handshake with the site failed.";
			break;
		case GeminiClient::ClientCode::REQUEST_ERROR:
			_error = "Sending Gemini request failed.";
			break;
		case GeminiClient::ClientCode::RESPONSE_HEADER_ERROR:
		case GeminiClient::ClientCode::RESPONSE_BODY_ERROR:
			_error = "Receiving Gemini response failed.";
			break;
		case GeminiClient::ClientCode::RESPONSE_HEADER_MALFORMED:
			_error = "The site returned a malformed Gemini response header.";
			break;
		default:
			assert(false);
			break;
	}

	_isLoaded = true;
}
void Page::connectAsyncCallback(std::shared_ptr<GeminiClient> client, std::weak_ptr<Page> pageWeakPtr, GeminiClient::ClientCode clientCode)
{
	if (pageWeakPtr.expired())
	{
		return;
	}

	std::shared_ptr<Page> page = pageWeakPtr.lock();

	if (clientCode == GeminiClient::ClientCode::SUCCESS)
	{
		client->receiveResponseHeaderAsync(std::bind(&receiveResponseHeaderAsyncCallback, client, pageWeakPtr, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	}
	else
	{
		page->setError(clientCode);
	}
}

void Page::receiveResponseHeaderAsyncCallback(std::shared_ptr<GeminiClient> client, std::weak_ptr<Page> pageWeakPtr, GeminiClient::ClientCode clientCode, StatusCode statusCode, std::string meta)
{
	if (pageWeakPtr.expired())
	{
		return;
	}

	std::shared_ptr<Page> page = pageWeakPtr.lock();

	if (clientCode == GeminiClient::ClientCode::SUCCESS)
	{
		// TODO: do not download the whole file directly into memory!

		//if (isMimeSupported(meta))
		//{
		//	client->receiveResponseBodyAsync(std::bind(&receiveResponseBodyAsyncCallback, pageWeakPtr, statusCode, meta, std::placeholders::_1, std::placeholders::_2));
		//}
		//else
		//{
		//	page->init(statusCode, meta, nullptr);
		//}

		client->receiveResponseBodyAsync(std::bind(&receiveResponseBodyAsyncCallback, pageWeakPtr, statusCode, meta, std::placeholders::_1, std::placeholders::_2));
	}
	else
	{
		page->setError(clientCode);
	}
}

void Page::receiveResponseBodyAsyncCallback(std::weak_ptr<Page> pageWeakPtr, StatusCode statusCode, std::string meta, GeminiClient::ClientCode clientCode, std::shared_ptr<std::vector<char>> data)
{
	if (pageWeakPtr.expired())
	{
		return;
	}

	std::shared_ptr<Page> page = pageWeakPtr.lock();

	if (clientCode == GeminiClient::ClientCode::SUCCESS)
	{
		page->init(statusCode, meta, data);
	}
	else
	{
		page->setError(clientCode);
	}
}