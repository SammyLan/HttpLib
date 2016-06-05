#pragma once
#include <curl\curl.h>
#include <string>

class CHttpSession;
class CHttpRequest
{
public:
	CHttpRequest(CHttpSession *pSession);
	~CHttpRequest();
	void get(std::string const & url);
	CURL * handle();
	bool close();
	void onDone(CURLcode res);
private:
	static size_t write_cb(void *ptr, size_t size, size_t nmemb, CHttpRequest *pThis);
	static int prog_cb(CHttpRequest *pThis, double dltotal, double dlnow, double ult, double uln);
	static curl_socket_t opensocket(CHttpSession *pThis, curlsocktype purpose, struct curl_sockaddr *address);
	static int close_socket(CHttpSession *pThis, curl_socket_t item);
public:
	CURL *	easy_;
	std::string	url_;
	CHttpSession *	pSession_;
	char error[CURL_ERROR_SIZE];
};

