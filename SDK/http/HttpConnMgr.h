#pragma once
#include <curl/curl.h>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <map>
#include <memory>
#include "HttpGlobal.h"
class CHttpConnMgr
{
public:
	typedef std::map<curl_socket_t, http::TCPSocketPtr>	SocketPool;
	friend class CHttpSession;
public:
	CHttpConnMgr(boost::asio::io_service & io_service);
	~CHttpConnMgr();
	http::TCPSocketPtr opensocket(curlsocktype purpose, struct curl_sockaddr *address);
	int close_socket(curl_socket_t item);
	http::TCPSocketPtr getSock(curl_socket_t s);
private:
	boost::asio::io_service & io_service_;
	SocketPool socketMap_;
};

