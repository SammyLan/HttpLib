#pragma once
#include <map>
#include <string>
#include <curl\curl.h>
#include <string>
#include <memory>
#include <tuple>
#define HTTPLOG _T("http")

namespace http
{
	bool CurlInit();
	bool CurlUninit();
	void mcode_or_die(const char *where, CURLMcode code);
}
namespace data
{
	typedef char byte;
	typedef std::string Buffer;
	typedef std::shared_ptr<Buffer> BufferPtr;	
	typedef std::pair<std::string, BufferPtr> FormItem;
	typedef std::vector<FormItem> FormList;

}