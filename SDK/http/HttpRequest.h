#pragma once
#include <curl\curl.h>
#include <string>
#include "HttpGlobal.h"
class CHttpSession;
class CHttpRequest
{
public:
	CHttpRequest(CHttpSession *pSession);
	~CHttpRequest();
	void setUrl(const std::string&url, const http::Parameters & para);	
	void setHeader(const http::Header & header);
	void setFormContent(const std::vector<std::pair<std::string, std::string>> & vecName2Content);

	void get(std::string const & url);
	void getWithParam(std::string const & url, const http::Parameters & para);

	CURL * getHandle() const { return handle_; }
	std::string const & getUrl() const { return url_; }

	bool close();
	void onDone(CURLcode res);	
private:
	static size_t write_cb(void *ptr, size_t size, size_t nmemb, CHttpRequest *pThis);
	static int prog_cb(CHttpRequest *pThis, double dltotal, double dlnow, double ult, double uln);
	static curl_socket_t opensocket(CHttpSession *pThis, curlsocktype purpose, struct curl_sockaddr *address);
	static int close_socket(CHttpSession *pThis, curl_socket_t item);
public:
	CURL *	handle_;
	std::string	url_;
	CHttpSession *	pSession_;
	char error[CURL_ERROR_SIZE];
};

