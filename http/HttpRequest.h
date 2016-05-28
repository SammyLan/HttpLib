#pragma once
#include <curl\curl.h>
#include <string>

class CHttpSession;
class CHttpRequest
{
public:
	CHttpRequest(CHttpSession *pSession);
	~CHttpRequest();
	void Get(std::string const & url);
	CURL * Handle();
	bool Close();
	void OnDone(CURLcode res);
private:
	static size_t write_cb(void *ptr, size_t size, size_t nmemb, CHttpRequest *pThis);
	static int prog_cb(CHttpRequest *pThis, double dltotal, double dlnow, double ult, double uln);
public:
	CURL *	easy;
	std::string	url_;
	CHttpSession *	pSession_;
	char error[CURL_ERROR_SIZE];
};

