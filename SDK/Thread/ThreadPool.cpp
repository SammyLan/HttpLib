#include "stdafx.h"
#include "ThreadPool.h"
#include <functional>

void SetThreadNameInternal(void * hThread, std::string const & threadName)
{
#ifdef WIN32
	//ֻ�ڵ��Ե�ʱ����Ч 
	if (!::IsDebuggerPresent())
		return;
	typedef struct tagTHREADNAME_INFO
	{
		DWORD dwType; // must be 0x1000
		LPCSTR szName; // pointer to name (in user addr space)
		DWORD dwThreadID; // thread ID (-1=caller thread)
		DWORD dwFlags; // reserved for future use, must be zero
	} THREADNAME_INFO;
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = threadName.c_str();
	info.dwThreadID = ::GetThreadId(hThread);
	info.dwFlags = 0;

	__try
	{
		const DWORD kVCThreadNameException = 0x406D1388;
		RaiseException(kVCThreadNameException, 0, sizeof(info) / sizeof(DWORD), reinterpret_cast<DWORD_PTR*>(&info));
	}
	__except (EXCEPTION_CONTINUE_EXECUTION)
	{
	}
#endif
}

ThreadPool::ThreadPool(size_t threads, std::string const & threadName)
	:work_(io_service_)
{
	for (size_t i = 0; i < threads; i++)
	{
		auto && thread = std::make_shared<std::thread>(std::bind<size_t(io_serviceT::*)()>(&io_serviceT::run, &io_service_));
		if (IsDebuggerPresent())
		{

		}
		SetThreadNameInternal(thread->native_handle(), threadName.empty()?"iothread":threadName);
		threadPool_.push_back(thread);
	}
}


ThreadPool::~ThreadPool()
{
	stop();
}
void ThreadPool::stop()
{
	io_service_.stop();
	for (auto & thread : threadPool_)
	{
		thread->join();
	}
	threadPool_.clear();
}
