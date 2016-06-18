#include "stdafx.h"
#include "HttpConnMgr.h"
#include "HttpGlobal.h"

CHttpConnMgr::CHttpConnMgr(boost::asio::io_service & io_service)
	:io_service_(io_service)
{

}


CHttpConnMgr::~CHttpConnMgr()
{
}

/* CURLOPT_OPENSOCKETFUNCTION */
http::TCPSocketPtr CHttpConnMgr::opensocket(curlsocktype purpose, struct curl_sockaddr *address)
{
	LogFinal(HTTPLOG,_T("\nopensocket :"));

	http::TCPSocketPtr newSocket;

	/* restrict to IPv4 */
	if (purpose == CURLSOCKTYPE_IPCXN && address->family == AF_INET)
	{
		/* create a tcp socket object */
		http::TCPSocketPtr tcp_socket(new boost::asio::ip::tcp::socket(io_service_));

		/* open it and get the native handle*/
		boost::system::error_code ec;
		tcp_socket->open(boost::asio::ip::tcp::v4(), ec);

		if (ec)
		{
			/* An error occurred */
			LogFinal(HTTPLOG,_T("\nERROR: Returning CURL_SOCKET_BAD to signal error,ec=%s"),ec.message());
		}
		else
		{
			newSocket = tcp_socket;
			LogFinal(HTTPLOG,_T("\nOpened socket %d"), newSocket->native_handle());

			/* save it for monitoring */
			socketMap_.insert(std::make_pair(newSocket->native_handle(), tcp_socket));
		}
	}

	return newSocket;
}

/* CURLOPT_CLOSESOCKETFUNCTION */
int CHttpConnMgr::close_socket( curl_socket_t item)
{
	LogFinal(HTTPLOG,_T("\nclose_socket : %d"), item);

	auto it = socketMap_.find(item);

	if (it != socketMap_.end())
	{
		socketMap_.erase(it);
	}
	return 0;
}

http::TCPSocketPtr CHttpConnMgr::getSock(curl_socket_t s)
{
	http::TCPSocketPtr pItem;
	auto it = socketMap_.find(s);
	if (it != socketMap_.end())
	{
		pItem = it->second;
	}
	return pItem;
}