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
	void enablePipeline(size_t maxRequestPerConn = 5);
	CURLMcode addHandle(CHttpRequest * pHandle);
	CURLMcode removeHandle(CHttpRequest * pHandle);
	CURLM * getHandle() const { return hMulti_; }
	http::SocketInfoPtr  openSocket(curlsocktype purpose, struct curl_sockaddr *address);
	int closeSocket(curl_socket_t item);
	CHttpConnMgr * getConnMgr() { return pConnMgr_; }
private:
	static int socket_callback(CURL *easy, curl_socket_t s, int what, CHttpSession *pThis, http::SocketInfo * sockInfo);
	static int timer_callback(CURLM *multi, long timeout_ms, CHttpSession *pThis);
	static void timer_cb(const boost::system::error_code & error, CHttpSession *pThis, long timeout_ms);
	static void event_cb(CHttpSession *pThis, http::SocketInfoPtr& tcp_socket, curl_socket_t s,CURL*e, int action, const boost::system::error_code &err);
private:
	void setSocket(http::SocketInfoPtr & sockInfo, curl_socket_t s, CURL*e, int act);
	void check_multi_info();
private:
	CURLM *	hMulti_;
	int		nStillRunning_ = 0;	
	boost::asio::io_service & io_service_;
	CHttpConnMgr * pConnMgr_;
	boost::asio::deadline_timer timer_;	
};