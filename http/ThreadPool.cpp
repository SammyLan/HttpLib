#include "stdafx.h"
#include "ThreadPool.h"

static void ThreadProc(ThreadPool * pThis)
{
	pThis->io_service().run();
}

ThreadPool::ThreadPool(size_t threads)
	:work_(io_service_)
{
	for (size_t i = 0; i < threads; i++)
	{
		threadPool_.push_back(ThreadPtr(new std::thread(&ThreadProc, this)));		
	}
}


ThreadPool::~ThreadPool()
{
	io_service_.stop();
	for (auto & thread: threadPool_)
	{
		thread->join();
	}
	threadPool_.clear();
}
