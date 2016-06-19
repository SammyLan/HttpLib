#pragma once
#include <curl\curl.h>
#include <cpr\cpr.h>
#include <string>
#include <functional>
#include "HttpGlobal.h"
class CHttpSession;
class CHttpRequest
{
	friend class CHttpSession;
public:
	enum RecvDataFlag
	{
		RecvData_None,
		RecvData_Header =0x00000001,
		RecvData_Body = 0x00000002,
		RecvData_All= RecvData_Header| RecvData_Body
	};
	typedef std::function<void(cpr::Response const & response, data::BufferPtr const & body)> OnRespond;
	typedef std::function<void(data::byte const * data, size_t size)> OnDataRecv;

public:
	static void setDebugMode(bool debugMode);
	static void setProxy(std::string const & strIP, std::string const & strPort, std::string const &strUserName, std::string const & strPassword);

	CHttpRequest(CHttpSession *pSession);
	~CHttpRequest();

	void setCookie(std::string const & cookie);
	void setRange(int64_t beg,int64_t end);
	int headRequest(std::string const & url,
		const cpr::Parameters & para = cpr::Parameters{},
		OnRespond const &  onRespond = OnRespond());
	int get(std::string const & url,
		const cpr::Parameters & para = cpr::Parameters{},
		DWORD const recvDataFlag = CHttpRequest::RecvData_None,
		OnRespond const &  onRespond = OnRespond(),
		OnDataRecv const & onBodyRecv = OnDataRecv(),
		OnDataRecv const & onHeaderRecv = OnDataRecv()
		);

	int postMultiForm(std::string const & url,
		cpr::Header const & header,
		cpr::Parameters const & para,
		data::FormList const & formList,
		DWORD const recvDataFlag = CHttpRequest::RecvData_None,
		OnRespond const &  onRespond = OnRespond(),
		OnDataRecv const & onBodyRecv = OnDataRecv(),
		OnDataRecv const & onHeaderRecv = OnDataRecv()
		);

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

private:
#pragma region delegate
	void setDelegate(DWORD const recvDataFlag,	OnRespond const & onRespond,OnDataRecv const & onBodyRecv,OnDataRecv const & onHeaderRecv);
	void setReadDelegate();
#pragma endregion delegate
	
private:
	void setUrl(const std::string&url, const cpr::Parameters & para);
	void setHeader(const cpr::Header & header);
	void setFormContent(data::FormList const & formList);
	void clearData();
#pragma region callback
	//static int prog_cb(CHttpRequest *pThis, double dltotal, double dlnow, double ult, double uln);

	static size_t header_callback(data::byte * data, size_t size, size_t nitems, CHttpRequest * pThis);
	static size_t header_callbackEx(data::byte * data, size_t size, size_t nitems, data::Buffer * pData);

	static size_t write_callback(data::byte * data, size_t size, size_t nitems, CHttpRequest *pThis);
	static size_t write_callbackEx(data::byte * data, size_t size, size_t nitems, data::Buffer *pData);

	static size_t read_callback(data::byte * data, size_t size, size_t nitems, CHttpRequest *pThis);

	static curl_socket_t opensocket(CHttpSession *pThis, curlsocktype purpose, struct curl_sockaddr *address);
	static int close_socket(CHttpSession *pThis, curl_socket_t item);
	static int sockopt_callback(CHttpSession * pThis, curl_socket_t curlfd, curlsocktype purpose);
	static int debug_callback(CURL *handle,curl_infotype type,	char *data,	size_t size, CHttpRequest * pThis);
#pragma endregion callback
private:
#pragma region data member
	CURL *	handle_ ;
	curl_httppost * formpost_ = nullptr;
	curl_slist * headerList_ = nullptr;
	std::string	url_;
	data::FormList formList_;
	data::Buffer header_;
	data::BufferPtr	body_;
	CHttpSession *	pSession_;
	char error_[CURL_ERROR_SIZE];
	static std::string strProxyHost;
	static std::string strProxyUsrPwd;
	static bool	s_debugMode;

	OnRespond	onRespond_;
	OnDataRecv	onHeaderRecv_;
	OnDataRecv	onBodyRecv_;

#pragma endregion data member
};

