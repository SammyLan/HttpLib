#pragma once
#include <thread>
#include <boost/asio.hpp>
#include <memory>
#include <vector>
class ThreadPool
{
	typedef std::shared_ptr<std::thread> ThreadPtr;
	typedef std::vector<ThreadPtr> ThreadList;
public:
	ThreadPool(size_t threads);
	~ThreadPool();
	boost::asio::io_service &  io_service() { return io_service_; };
private:
	boost::asio::io_service  io_service_;
	boost::asio::io_service::work work_;
	ThreadList threadPool_;
};

