#pragma once
#include <map>
#include <string>
#include <string>
#include <memory>
#include <tuple>
#include <boost/asio.hpp>
#define HTTPLOG _T("http")

namespace http
{
	struct SocketInfo
	{
		SocketInfo(boost::asio::io_service & io_service)
			:tcpSocket(io_service)	{}
		boost::asio::ip::tcp::socket tcpSocket;
		int mask = 0;
	};
	typedef std::shared_ptr<SocketInfo> SocketInfoPtr;

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