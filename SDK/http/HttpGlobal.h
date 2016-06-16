#pragma once
#include <map>
#include <string>
#include <curl\curl.h>
#include <vector>
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
	typedef unsigned char byte;
	typedef std::vector<byte> Buffer;
	typedef std::shared_ptr<Buffer> BufferPtr;	
	typedef std::pair<std::string, BufferPtr> FormItem;
	typedef std::vector<FormItem> FormList;

}