#pragma once
#include <curl/curl.h>
#include <boost/asio.hpp>
#include <boost/bind.hpp>



namespace NSCURL
{
	bool CurlInit();
	bool CurlUninit();
	void mcode_or_die(const char *where, CURLMcode code);
}

class CHttpRequest;
class CHttpConnMgr;
class CHttpSession
{
public:
	typedef std::shared_ptr<boost::asio::ip::tcp::socket> SocketPtr;
	typedef std::map<curl_socket_t, SocketPtr>	SocketPool;
public:
	CHttpSession(boost::asio::io_service & io_service, CHttpConnMgr * pConnMgr);
	~CHttpSession();
	CURLMcode addHandle(CHttpRequest * pHandle);
	CURLMcode removeHandle(CHttpRequest * pHandle);
	CURLM * handle();
	curl_socket_t opensocket(curlsocktype purpose, struct curl_sockaddr *address);
	int close_socket(curl_socket_t item);
private:
	static int sock_cb(CURL *e, curl_socket_t s, int what, CHttpSession *pThis, void *sockp);
	static int multi_timer_cb(CURLM *multi, long timeout_ms, CHttpSession *pThis);
	static void timer_cb(const boost::system::error_code & error, CHttpSession *pThis);
	static void event_cb(CHttpSession *pThis, CHttpSession::SocketPtr& tcp_socket,int action);
private:
	void remsock(int *f);
	void addsock(curl_socket_t s, CURL *easy, int action);
	void setsock(int *fdp, curl_socket_t s, CURL*e, int act);
	void check_multi_info();
private:
	CURLM *	hMulti_;
	int		iStillRunning_;	
	boost::asio::io_service & io_service_;
	CHttpConnMgr * pConnMgr_;
	boost::asio::deadline_timer timer_;	
	SocketPool socketMap_;
};

