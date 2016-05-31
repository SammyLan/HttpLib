#include "stdafx.h"
#include "ThreadPool.h"
#include <functional>


ThreadPool::ThreadPool(size_t threads)
	:work_(io_service_)
{
	for (size_t i = 0; i < threads; i++)
	{
		threadPool_.push_back(std::make_shared<std::thread>(std::bind<size_t (io_serviceT::*)()>(&io_serviceT::run, &io_service_)));
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
