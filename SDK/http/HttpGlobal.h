#pragma once
#include <map>
#include <string>
#include <curl\curl.h>

#define HTTPLOG _T("http")

namespace http
{
	bool CurlInit();
	bool CurlUninit();
	void mcode_or_die(const char *where, CURLMcode code);
}