#pragma once
#include <curl\curl.h>
#include <string>
#include "HttpGlobal.h"
class CHttpSession;
class CHttpRequest
{
	friend class CHttpSession;
public:
	static void setDebugMode(bool debugMode);
	static void setProxy(std::string const & strIP, std::string const & strPort, std::string const &strUserName, std::string const & strPassword);

	CHttpRequest(CHttpSession *pSession);
	~CHttpRequest();

	void setUrl(const std::string&url, const http::Parameters & para);	
	void setHeader(const http::Header & header);
	void setFormContent(const std::vector<std::pair<std::string, std::string>> & vecName2Content);
	void setCookie(std::string const & cookie);
	void setRange(int64_t beg,int64_t end);

	void get(std::string const & url, const http::Parameters & para = http::Parameters{});

	bool close();
	void onDone(CURLcode res);

	CURL * getHandle() const { return handle_; }
	std::string const & getUrl() const { return url_; }

#pragma region option
	void setTimeout(long ms);
	void setConnTimeout(long ms);
	void setMaxUploadSpeed(int64_t maxSpeed);
	void setMaxDowloadSpeed(int64_t maxSpeed);
#pragma endregion option

#pragma region delegate
	void setReadDelegate();
	void setWriteDelegate();
	void setHeaderDelegate();
#pragma endregion delegate

	

	
private:
#pragma region callback
	//static int prog_cb(CHttpRequest *pThis, double dltotal, double dlnow, double ult, double uln);

	static size_t write_callback(void *ptr, size_t size, size_t nmemb, CHttpRequest *pThis);
	static size_t read_callback(char *buffer, size_t size, size_t nitems, CHttpRequest *pThis);

	static curl_socket_t opensocket(CHttpSession *pThis, curlsocktype purpose, struct curl_sockaddr *address);
	static int close_socket(CHttpSession *pThis, curl_socket_t item);
	static int sockopt_callback(CHttpSession * pThis, curl_socket_t curlfd, curlsocktype purpose);
	static int debug_callback(CURL *handle,curl_infotype type,	char *data,	size_t size, CHttpRequest * pThis);

	static size_t header_callback(char *buffer, size_t size, size_t nitems, CHttpRequest * pThis);
#pragma endregion callback
private:
#pragma region data member
	CURL *	handle_;
	std::string	url_;
	CHttpSession *	pSession_;
	char error[CURL_ERROR_SIZE];
	static std::string strProxyHost;
	static std::string strProxyUsrPwd;
	static bool	s_debugMode;
#pragma endregion data member
};

