#include "stdafx.h"
#include "HttpConnMgr.h"
#include "HttpGlobal.h"
#include <Winsock2.h>
CHttpConnMgr::CHttpConnMgr(boost::asio::io_service & io_service)
	:io_service_(io_service)
{

}


CHttpConnMgr::~CHttpConnMgr()
{
}

/* CURLOPT_OPENSOCKETFUNCTION */
http::SocketInfoPtr CHttpConnMgr::opensocket(curlsocktype purpose, struct curl_sockaddr *address)
{
	LogFinal(HTTPLOG,_T("\nopensocket :"));

	http::SocketInfoPtr newSocket;

	/* restrict to IPv4 */
	if (purpose == CURLSOCKTYPE_IPCXN && address->family == AF_INET)
	{
		/* create a tcp socket object */
		http::SocketInfoPtr tcp_socket(new http::SocketInfo(io_service_));

		/* open it and get the native handle*/
		boost::system::error_code ec;
		tcp_socket->tcpSocket.open(boost::asio::ip::tcp::v4(), ec);

		if (ec)
		{
			/* An error occurred */
			LogFinal(HTTPLOG,_T("\nERROR: Returning CURL_SOCKET_BAD to signal error,ec=%S"),ec.message());
		}
		else
		{
			newSocket = tcp_socket;
			/* save it for monitoring */
			socketMap_.insert(std::make_pair(newSocket->tcpSocket.native_handle(), tcp_socket));
			int size =(int) socketMap_.size();
			LogFinal(HTTPLOG,_T("Opened socket %d,total size = %i"), newSocket->tcpSocket.native_handle(),size);
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
		// close or cancel will cancel any asynchronous send, receive or connect operations 
		// Caution: on Windows platform, if connect host timeout, the event_cb will pending forever. Must be canceled manually
		it->second->tcpSocket.cancel();
		socketMap_.erase(it);
	}
	return 0;
}

http::SocketInfoPtr CHttpConnMgr::getSock(curl_socket_t s)
{
	http::SocketInfoPtr pItem;
	auto it = socketMap_.find(s);
	if (it != socketMap_.end())
	{
		pItem = it->second;
	}
	return pItem;
}