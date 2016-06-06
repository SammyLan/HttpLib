#pragma once
#include <curl/curl.h>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <map>
#include <memory>

class CHttpConnMgr
{
	typedef std::shared_ptr<boost::asio::ip::tcp::socket> SocketPtr;
	typedef std::map<curl_socket_t, SocketPtr>	SocketPool;
public:
	CHttpConnMgr(boost::asio::io_service & io_service);
	~CHttpConnMgr();
	curl_socket_t opensocket(curlsocktype purpose, struct curl_sockaddr *address);
	int close_socket(curl_socket_t item);
private:
	boost::asio::io_service & io_service_;
	SocketPool socketMap_;
};

