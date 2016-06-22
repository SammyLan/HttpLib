#pragma once
#include <map>
#include <string>
#include <string>
#include <memory>
#include <tuple>
#include <boost/asio.hpp>
#include <codecvt>
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
	typedef std::pair<int64_t, Buffer> RecvData;	//offset-data
	typedef std::shared_ptr<RecvData> SaveDataPtr;
	typedef std::pair<std::string, BufferPtr> FormItem;
	typedef std::vector<FormItem> FormList;
}

namespace convert
{
	extern std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8ToUnicode;
}

namespace WYTime
{
	class TimeWatch {
	public:
		TimeWatch() { QueryPerformanceFrequency(&m_liPerfFreq); Start(); }

		void Start() { QueryPerformanceCounter(&m_liPerfStart); }

		int64_t Now() const 
		{
			LARGE_INTEGER liPerfNow;
			QueryPerformanceCounter(&liPerfNow);
			return(((liPerfNow.QuadPart - m_liPerfStart.QuadPart) * 1000)/ m_liPerfFreq.QuadPart);
		}

		int64_t NowInMicro() const {   // Returns # of microseconds since Start was called
			LARGE_INTEGER liPerfNow;
			QueryPerformanceCounter(&liPerfNow);
			return(((liPerfNow.QuadPart - m_liPerfStart.QuadPart) * 1000000)/ m_liPerfFreq.QuadPart);
		}
	private:
		LARGE_INTEGER m_liPerfFreq;   // Counts per second
		LARGE_INTEGER m_liPerfStart;  // Starting count
	};
	extern TimeWatch g_watch;
}