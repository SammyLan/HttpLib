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
	typedef std::map<curl_socket_t, http::SocketInfoPtr>	SocketPool;
	friend class CHttpSession;
public:
	CHttpConnMgr(boost::asio::io_service & io_service);
	~CHttpConnMgr();
	http::SocketInfoPtr opensocket(curlsocktype purpose, struct curl_sockaddr *address);
	int close_socket(curl_socket_t item);
	http::SocketInfoPtr getSock(curl_socket_t s);
	size_t getSockCount() { return socketMap_.size(); }
private:
	boost::asio::io_service & io_service_;
	SocketPool socketMap_;
};

