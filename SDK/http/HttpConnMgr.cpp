#include "stdafx.h"
#include "HttpConnMgr.h"
#include <iostream>

CHttpConnMgr::CHttpConnMgr(boost::asio::io_service & io_service)
	:io_service_(io_service)
{

}


CHttpConnMgr::~CHttpConnMgr()
{
}

/* CURLOPT_OPENSOCKETFUNCTION */
curl_socket_t CHttpConnMgr::opensocket(curlsocktype purpose, struct curl_sockaddr *address)
{
	fprintf(MSG_OUT, "\nopensocket :");

	curl_socket_t sockfd = CURL_SOCKET_BAD;

	/* restrict to IPv4 */
	if (purpose == CURLSOCKTYPE_IPCXN && address->family == AF_INET)
	{
		/* create a tcp socket object */
		SocketPtr tcp_socket(new boost::asio::ip::tcp::socket(io_service_));

		/* open it and get the native handle*/
		boost::system::error_code ec;
		tcp_socket->open(boost::asio::ip::tcp::v4(), ec);

		if (ec)
		{
			/* An error occurred */
			std::cout << std::endl << "Couldn't open socket [" << ec << "][" << ec.message() << "]";
			fprintf(MSG_OUT, "\nERROR: Returning CURL_SOCKET_BAD to signal error");
		}
		else
		{
			sockfd = tcp_socket->native_handle();
			fprintf(MSG_OUT, "\nOpened socket %d", sockfd);

			/* save it for monitoring */
			socketMap_.insert(std::make_pair(sockfd, tcp_socket));
		}
	}

	return sockfd;
}

/* CURLOPT_CLOSESOCKETFUNCTION */
int CHttpConnMgr::close_socket( curl_socket_t item)
{
	fprintf(MSG_OUT, "\nclose_socket : %d", item);

	auto it = socketMap_.find(item);

	if (it != socketMap_.end())
	{
		socketMap_.erase(it);
	}
	return 0;
}