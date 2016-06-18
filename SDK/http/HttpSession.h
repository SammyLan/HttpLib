#pragma once
#include <curl/curl.h>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include "HttpGlobal.h"

class CHttpRequest;
class CHttpConnMgr;
class CHttpSession
{
public:
	CHttpSession(boost::asio::io_service & io_service, CHttpConnMgr * pConnMgr);
	~CHttpSession();
	CURLMcode addHandle(CHttpRequest * pHandle);
	CURLMcode removeHandle(CHttpRequest * pHandle);
	CURLM * getHandle() const { return hMulti_; }
	http::TCPSocketPtr  openSocket(curlsocktype purpose, struct curl_sockaddr *address);
	int closeSocket(curl_socket_t item);
private:
	static int socket_callback(CURL *easy, curl_socket_t s, int what, CHttpSession *pThis, void *socketp);
	static int timer_callback(CURLM *multi, long timeout_ms, CHttpSession *pThis);
	static void timer_cb(const boost::system::error_code & error, CHttpSession *pThis);
	static void event_cb(CHttpSession *pThis, http::TCPSocketPtr& tcp_socket,int action);
private:
	void removeSocket(int *f);
	void addSocket(curl_socket_t s, CURL *easy, int action);
	void setSocket(int *fdp, curl_socket_t s, CURL*e, int act);
	void check_multi_info();
private:
	CURLM *	hMulti_;
	int		iStillRunning_;	
	boost::asio::io_service & io_service_;
	CHttpConnMgr * pConnMgr_;
	boost::asio::deadline_timer timer_;	
};