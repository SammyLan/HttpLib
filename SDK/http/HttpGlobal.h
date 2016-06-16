#pragma once
#include <map>
#include <string>
#include <string>
#include <memory>
#include <tuple>
#define HTTPLOG _T("http")

namespace http
{
	struct CurGlobalInit
	{
		CurGlobalInit();
		~CurGlobalInit();
	};

	void mcode_or_die(const char *where, int code);
}
namespace data
{
	typedef char byte;
	typedef std::string Buffer;
	typedef std::shared_ptr<Buffer> BufferPtr;	
	typedef std::pair<std::string, BufferPtr> FormItem;
	typedef std::vector<FormItem> FormList;

}