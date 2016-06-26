#include "stdafx.h"
#include "HttpConnMgr.h"
#include "HttpGlobal.h"
#include <Winsock2.h>

TCHAR const LOGFILTER[] = _T("CHttpConnMgr");

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
			LogErrorEx(LOGFILTER,_T("ERROR: Returning CURL_SOCKET_BAD to signal error,ec=%S"),ec.message());
		}
		else
		{
			newSocket = tcp_socket;
			/* save it for monitoring */
			socketMap_.insert(std::make_pair(newSocket->tcpSocket.native_handle(), tcp_socket));
			int size =(int) socketMap_.size();
			LogFinal(LOGFILTER,_T("socket [%p]: Opened,total size = %d"), (SOCKET)newSocket->tcpSocket.native_handle(),size);
		}
	}
	else
	{
		LogErrorEx(LOGFILTER,_T("unknown  operation :"));
	}

	return newSocket;
}

/* CURLOPT_CLOSESOCKETFUNCTION */
int CHttpConnMgr::close_socket( curl_socket_t item)
{
	LogFinal(LOGFILTER,_T("socket [%p]: close"), item);

	auto it = socketMap_.find(item);

	if (it != socketMap_.end())
	{
		// close or cancel will cancel any asynchronous send, receive or connect operations 
		// Caution: on Windows platform, if connect host timeout, the event_cb will pending forever. Must be canceled manually
#if _WIN32_WINNT >= _WIN32_WINNT_VISTA
	it->second->tcpSocket.cancel();
#endif // _WIN32_WINNT >= _WIN32_WINNT_VISTA
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