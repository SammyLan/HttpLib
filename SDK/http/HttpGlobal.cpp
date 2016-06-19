#include "stdafx.h"
#include "HttpGlobal.h"
#include <curl\curl.h>
#ifdef _DEBUG
#pragma comment(lib,"libcurl_a_debug.lib")
#else
#pragma comment(lib,"libcurl_a.lib")
#endif // _DEBUG

namespace http
{
	CurGlobalInit::CurGlobalInit()
	{
		curl_global_init(CURL_GLOBAL_ALL);
	}

	CurGlobalInit::~CurGlobalInit()
	{
		curl_global_cleanup();
	}
}


void http::mcode_or_die(const char *where, int code)
{
	if (CURLM_OK != code)
	{
		const char *s;
		switch (code)
		{
		case CURLM_CALL_MULTI_PERFORM:
			s = "CURLM_CALL_MULTI_PERFORM";
			break;
		case CURLM_BAD_HANDLE:
			s = "CURLM_BAD_HANDLE";
			break;
		case CURLM_BAD_EASY_HANDLE:
			s = "CURLM_BAD_EASY_HANDLE";
			break;
		case CURLM_OUT_OF_MEMORY:
			s = "CURLM_OUT_OF_MEMORY";
			break;
		case CURLM_INTERNAL_ERROR:
			s = "CURLM_INTERNAL_ERROR";
			break;
		case CURLM_UNKNOWN_OPTION:
			s = "CURLM_UNKNOWN_OPTION";
			break;
		case CURLM_LAST:
			s = "CURLM_LAST";
			break;
		default:
			s = "CURLM_unknown";
			break;
		case CURLM_BAD_SOCKET:
			s = "CURLM_BAD_SOCKET";
			LogFinal(HTTPLOG,_T( "ERROR: %S returns %S"), where, s);
			/* ignore this error */
			return;
		}
		assert(false);
		LogFinal(HTTPLOG,_T( "ERROR: %S returns %S"), where, s);
		exit(code);
	}
}

