#pragma once

#include <cassert>

namespace gem
{
	enum class StatusCode
	{
		NONE = 0,
		INPUT = 10,
		SENSITIVE_INPUT = 11,
		SUCCESS = 20,
		REDIRECT_TEMPORARY = 30,
		REDIRECT_PERMANENT = 31,
		TEMPORARY_FAILURE = 40,
		SERVER_UNAVAILABLE = 41,
		CGI_ERROR = 42,
		PROXY_ERROR = 43,
		SLOW_DOWN = 44,
		PERMANENT_FAILURE = 50,
		NOT_FOUND = 51,
		GONE = 52,
		PROXY_REQUEST_REFUSED = 53,
		BAD_REQUEST = 59,
		CLIENT_CERTIFICATE_REQUIRED = 60,
		CERTIFICATE_NOT_AUTHORISED = 61,
		CERTIFICATE_NOT_VALID = 62
	};

	constexpr const char *statusCodeToString(StatusCode code)
	{
		switch (code)
		{
			case StatusCode::NONE:
				return "NONE";
			case StatusCode::INPUT:
				return "INPUT";
			case StatusCode::SENSITIVE_INPUT:
				return "SENSITIVE INPUT";
			case StatusCode::SUCCESS:
				return "SUCCESS";
			case StatusCode::REDIRECT_TEMPORARY:
				return "REDIRECT TEMPORARY";
			case StatusCode::REDIRECT_PERMANENT:
				return "REDIRECT PERMANENT";
			case StatusCode::TEMPORARY_FAILURE:
				return "TEMPORARY FAILURE";
			case StatusCode::SERVER_UNAVAILABLE:
				return "SERVER UNAVAILABLE";
			case StatusCode::CGI_ERROR:
				return "CGI ERROR";
			case StatusCode::PROXY_ERROR:
				return "PROXY ERROR";
			case StatusCode::SLOW_DOWN:
				return "SLOW DOWN";
			case StatusCode::PERMANENT_FAILURE:
				return "PERMANENT FAILURE";
			case StatusCode::NOT_FOUND:
				return "NOT FOUND";
			case StatusCode::GONE:
				return "GONE";
			case StatusCode::PROXY_REQUEST_REFUSED:
				return "PROXY REQUEST REFUSED";
			case StatusCode::BAD_REQUEST:
				return "BAD REQUEST";
			case StatusCode::CLIENT_CERTIFICATE_REQUIRED:
				return "CLIENT CERTIFICATE REQUIRED";
			case StatusCode::CERTIFICATE_NOT_AUTHORISED:
				return "CERTIFICATE NOT AUTHORISED";
			case StatusCode::CERTIFICATE_NOT_VALID:
				return "CERTIFICATE NOT VALID";
			default:
				assert(false);
				return nullptr;
		}
	}
}