#pragma once
#include <thread>
#include <boost/asio.hpp>
#include <memory>
#include <vector>
class ThreadPool
{
public:
	typedef std::shared_ptr<std::thread> ThreadPtr;
	typedef std::vector<ThreadPtr> ThreadList;
	typedef boost::asio::io_service io_serviceT;
public:
	ThreadPool(size_t threads,std::string const & threadName = std::string(""));
	~ThreadPool();
	void stop();

	template<typename Task>	void postTask(Task && task);

	ThreadPool::io_serviceT &  io_service() { return io_service_; };
private:
	io_serviceT  io_service_;
	io_serviceT::work work_;
	ThreadList threadPool_;
};


template<typename Task>	
inline void ThreadPool::postTask(Task && task)
{
	io_service_.post(std::forward<Task&&>(task));
}