#include "stdafx.h"
#include "HttpRequest.h"
#include "HttpSession.h"
#include <sstream>
#include <cpr/error.h>
#include <cpr/util.h>


std::string CHttpRequest::strProxyHost;
std::string CHttpRequest::strProxyUsrPwd;
#ifdef _DEBUG
bool	CHttpRequest::s_debugMode = true;
#else
bool	CHttpRequest::s_debugMode = false;
#endif
TCHAR const LOGFILTER[] = _T("CHttpRequest");

void CHttpRequest::setDebugMode(bool debugMode)
{
	s_debugMode = debugMode;
}

void CHttpRequest::setProxy(std::string const & strIP, std::string const & strPort, std::string const &strUserName, std::string const & strPassword)
{
	if (!strIP.empty())
	{
		strProxyHost = strIP + ":" + strPort;
		if (!strUserName.empty())
		{
			strProxyUsrPwd = strUserName + ":" + strPassword;
		}
	}
}

CHttpRequest::CHttpRequest(CHttpSession *pSession)
	:pSession_(pSession)
{
	error_[0] = '\0';
	handle_ = curl_easy_init();
	if (!handle_)
	{
		LogErrorEx(LOGFILTER,_T("curl_easy_init() failed, exiting!"));
		exit(2);
	}

	if (s_debugMode)
	{
		curl_easy_setopt(this->handle_, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(this->handle_, CURLOPT_DEBUGFUNCTION, &CHttpRequest::debug_callback);
		curl_easy_setopt(this->handle_, CURLOPT_DEBUGDATA, this);
	}
	curl_easy_setopt(this->handle_, CURLOPT_COOKIEFILE, ""); /* just to start the cookie engine */
	curl_easy_setopt(this->handle_, CURLOPT_NOSIGNAL, 1L);
	//curl_easy_setopt(this->handle_, CURLOPT_NOPROGRESS, 1L);
	//curl_easy_setopt(this->handle_, CURLOPT_PROGRESSFUNCTION, prog_cb);
	//curl_easy_setopt(this->handle_, CURLOPT_PROGRESSDATA, this);
	
	curl_easy_setopt(this->handle_, CURLOPT_ERRORBUFFER, this->error_);
	curl_easy_setopt(this->handle_, CURLOPT_PRIVATE, this);
	
	//curl_easy_setopt(this->handle_, CURLOPT_LOW_SPEED_TIME, 3L);
	//curl_easy_setopt(this->handle_, CURLOPT_LOW_SPEED_LIMIT, 10L);

	curl_easy_setopt(this->handle_, CURLOPT_TCP_KEEPALIVE, 1L);
	curl_easy_setopt(this->handle_, CURLOPT_TCP_KEEPIDLE, 120L);
	curl_easy_setopt(this->handle_, CURLOPT_TCP_KEEPINTVL, 60L);
	curl_easy_setopt(this->handle_, CURLOPT_NOSIGNAL, 1);
	if (!strProxyHost.empty())
	{
		curl_easy_setopt(handle_, CURLOPT_PROXY, strProxyHost.c_str());
		if (!strProxyUsrPwd.empty())
		{
			curl_easy_setopt(handle_, CURLOPT_PROXYUSERPWD, strProxyUsrPwd.c_str());
		}
	}
	//setTimeout(1000000);
	setConnTimeout(1000*60);
	/* call this function to get a socket */
	curl_easy_setopt(handle_, CURLOPT_OPENSOCKETFUNCTION, &CHttpRequest::opensocket);
	curl_easy_setopt(handle_, CURLOPT_OPENSOCKETDATA, this->pSession_);

	/* call this function to close a socket */
	curl_easy_setopt(this->handle_, CURLOPT_CLOSESOCKETFUNCTION, &CHttpRequest::close_socket);
	curl_easy_setopt(this->handle_, CURLOPT_CLOSESOCKETDATA, this->pSession_);

	curl_easy_setopt(this->handle_, CURLOPT_SOCKOPTFUNCTION, &CHttpRequest::sockopt_callback);
	curl_easy_setopt(this->handle_, CURLOPT_SOCKOPTDATA, this);
	setReadDelegate();
}

CHttpRequest::~CHttpRequest()
{
	clearData();
	curl_easy_cleanup(handle_);
}

void CHttpRequest::setUrl(const std::string&url, const cpr::Parameters & para)
{
	if (!para.content.empty())
	{
		url_ = url + "?" + para.content;
	}
	else
	{
		url_ = url;
	}
	curl_easy_setopt(handle_, CURLOPT_URL, url_.data());
}

void CHttpRequest::setHeader(const cpr::Header & header)
{
	for (auto item = header.cbegin(); item != header.cend(); ++item)
	{
		auto header_string = item->first;
		if (item->second.empty())
		{
			header_string += ";";
		}
		else
		{
			header_string += ": " + item->second;
		}
		headerList_ = curl_slist_append(headerList_, header_string.data());
		curl_easy_setopt(handle_, CURLOPT_HTTPHEADER, headerList_);
	}
}

void CHttpRequest::setFormContent(data::FormList const & formList)
{
	formpost_ = nullptr;
	curl_httppost *lastptr = nullptr;	
	for (auto const & item: formList)
	{
		curl_formadd(&formpost_, &lastptr,
			CURLFORM_COPYNAME, item.first.c_str(),
			CURLFORM_PTRCONTENTS, item.second->data(),
			CURLFORM_CONTENTSLENGTH, item.second->size(),
			CURLFORM_END);
	}
	curl_easy_setopt(handle_, CURLOPT_HTTPPOST, formpost_);
	formList_ = formList;
}

void CHttpRequest::setCookie(std::string const & cookie)
{
	curl_easy_setopt(handle_, CURLOPT_COOKIE, cookie.c_str());
}

void CHttpRequest::setRange(int64_t beg, int64_t end)
{
	std::ostringstream ofs;
	if (end != 0)
	{
		ofs << beg << '-' << end;
	}
	else
	{
		ofs << beg << '-';
	}	
	curl_easy_setopt(handle_, CURLOPT_RANGE, ofs.str().c_str());
}

int CHttpRequest::headRequest(std::string const & url,
	const cpr::Parameters & para,
	OnRespond const &  onRespond)
{
	setDelegate(CHttpRequest::RecvData_Header, onRespond, OnDataRecv(), OnDataRecv());
	curl_easy_setopt(handle_, CURLOPT_NOBODY, 1);
	setUrl(url, para);
	auto rc = pSession_->addHandle(this);
	return rc;
}

int CHttpRequest::get(std::string const & url,
	const cpr::Parameters & para,
	DWORD const recvDataFlag,
	OnRespond const &  onRespond,
	OnDataRecv const & onBodyRecv,
	OnDataRecv const & onHeaderRecv
	)
{
	setDelegate(recvDataFlag, onRespond, onBodyRecv,onHeaderRecv);
	setUrl(url, para);
	auto rc = pSession_->addHandle(this);
	return rc;
}

int CHttpRequest::postMultiForm(std::string const & url,
	cpr::Header const & header,
	cpr::Parameters const & para,
	data::FormList const & formList,
	DWORD const recvDataFlag,
	OnRespond const & onRespond,
	OnDataRecv const & onBodyRecv,
	OnDataRecv const & onHeaderRecv)
{
	setDelegate(recvDataFlag, onRespond, onBodyRecv, onHeaderRecv);
	setUrl(url, para);
	setFormContent(formList);
	auto rc = pSession_->addHandle(this);
	return rc;
}

bool CHttpRequest::cancel()
{
	LogFinal(LOGFILTER, _T("cancle\r\nurl=%S"),url_.c_str());
	pSession_->removeHandle(this);
	//TODO::是否需要调用onDone?
	return true;
}

void CHttpRequest::onDone(CURLcode res)
{
	char* raw_url;
	long response_code;
	double elapsed;
	curl_easy_getinfo(handle_, CURLINFO_RESPONSE_CODE, &response_code);
	curl_easy_getinfo(handle_, CURLINFO_TOTAL_TIME, &elapsed);
	curl_easy_getinfo(handle_, CURLINFO_EFFECTIVE_URL, &raw_url);

	cpr::Error error(res, error_);

	cpr::Cookies cookies;
	struct curl_slist* raw_cookies;
	curl_easy_getinfo(handle_, CURLINFO_COOKIELIST, &raw_cookies);
	for (struct curl_slist* nc = raw_cookies; nc; nc = nc->next) {
		auto tokens = cpr::util::split(nc->data, '\t');
		auto value = tokens.back();
		tokens.pop_back();
		cookies[tokens.back()] = value;
	}
	curl_slist_free_all(raw_cookies);

	auto header = cpr::util::parseHeader(header_);
	//auto && response_string = cpr::util::parseResponse(*body_);
	cpr::Response response{ response_code, "", header, raw_url, elapsed, cookies, error };
	LogFinal(LOGFILTER, _T("DONE: %S => (%d) %S"), raw_url, res, error_);
	onRespond_(response, body_);
}

#pragma region option
void CHttpRequest::setTimeout(long ms)
{
	curl_easy_setopt(handle_, CURLOPT_TIMEOUT_MS, ms);
}

void CHttpRequest::setConnTimeout(long ms)
{
	curl_easy_setopt(handle_, CURLOPT_CONNECTTIMEOUT_MS, ms);
}


void CHttpRequest::setMaxUploadSpeed(int64_t maxSpeed)
{
	curl_easy_setopt(handle_, CURLOPT_MAX_SEND_SPEED_LARGE, maxSpeed);
}

void CHttpRequest::setMaxDowloadSpeed(int64_t maxSpeed)
{
	curl_easy_setopt(handle_, CURLOPT_MAX_RECV_SPEED_LARGE, maxSpeed);
}
#pragma endregion option

#pragma region delegate
void CHttpRequest::setDelegate(DWORD const recvDataFlag, OnRespond const & onRespond, OnDataRecv const & onBodyRecv, OnDataRecv const & onHeaderRecv)
{
	onRespond_ = onRespond;
	onBodyRecv_ = onBodyRecv;
	onHeaderRecv_ = onHeaderRecv;
	header_.clear();
	body_.reset();
	if (recvDataFlag & RecvData_Header)
	{
		if (onHeaderRecv)
		{
			curl_easy_setopt(this->handle_, CURLOPT_HEADERFUNCTION, header_callback);
			curl_easy_setopt(this->handle_, CURLOPT_HEADERDATA, this);
		}
		else
		{
			curl_easy_setopt(this->handle_, CURLOPT_HEADERFUNCTION, header_callbackEx);
			curl_easy_setopt(this->handle_, CURLOPT_HEADERDATA, &header_);
		}		
	}

	if (recvDataFlag & RecvData_Body)
	{
		if (onBodyRecv)
		{
			curl_easy_setopt(this->handle_, CURLOPT_WRITEFUNCTION, write_callback);
			curl_easy_setopt(this->handle_, CURLOPT_WRITEDATA, this);
		}
		else
		{
			body_ = std::make_shared<data::Buffer>();			
			curl_easy_setopt(this->handle_, CURLOPT_WRITEFUNCTION, write_callbackEx);
			curl_easy_setopt(this->handle_, CURLOPT_WRITEDATA, body_.get());
		}
	}
}

void CHttpRequest::setReadDelegate()
{
	curl_easy_setopt(this->handle_, CURLOPT_READFUNCTION, read_callback);
	curl_easy_setopt(this->handle_, CURLOPT_READDATA, this);
}

#pragma endregion delegate

#pragma region private

void CHttpRequest::clearData()
{
	if (formpost_ != nullptr)
	{
		curl_formfree(formpost_);
		formpost_ = nullptr;
	}
	if (headerList_ != nullptr)
	{
		curl_slist_free_all(headerList_);
		headerList_ = nullptr;
	}
	formList_.clear();
}

#pragma region private
#pragma region callback
size_t CHttpRequest::header_callback(data::byte *data, size_t size, size_t nitems, CHttpRequest * pThis)
{
	pThis->onHeaderRecv_(data, size * nitems);
	return size * nitems;
}

size_t CHttpRequest::header_callbackEx(data::byte *data, size_t size, size_t nitems, data::Buffer * pData)
{
	pData->append(data, size * nitems);
	return size * nitems;
}

/* CURLOPT_WRITEFUNCTION */
size_t CHttpRequest::write_callback(data::byte *data, size_t size, size_t nitems, CHttpRequest *pThis)
{
	pThis->onBodyRecv_(data, size * nitems);
	return size * nitems;
}

size_t CHttpRequest::write_callbackEx(data::byte * data, size_t size, size_t nitems, data::Buffer *pData)
{
	pData->append(data, size * nitems);
	return size * nitems;
}

size_t CHttpRequest::read_callback(data::byte *buffer, size_t size, size_t nitems, CHttpRequest *pThis)
{
	return size * nitems;
}

/* CURLOPT_PROGRESSFUNCTION */
/*int CHttpRequest::prog_cb(CHttpRequest *pThis, double dltotal, double dlnow, double ult, double uln)
{
	(void)ult;
	(void)uln;

	LogDev(HTTPLOG,_T("Progress: %S (%g/%g)"), pThis->url_.c_str(), dlnow, dltotal);
	LogDev(HTTPLOG,_T("Progress: %S (%g)"), pThis->url_.c_str(), ult);

	return 0;
}*/

int CHttpRequest::debug_callback(CURL *handle, curl_infotype type, char *data, size_t size, CHttpRequest * pThis)
{
	static TCHAR const LOGCURLDEBUG[] = _T("CURLDEBUG");
	const char *text;
	(void)handle; /* prevent compiler warning */

	switch (type){
	case CURLINFO_TEXT:
		LogDev(LOGCURLDEBUG, _T("%p:== Info:\r\n%S"), handle,data);
		break;	
	case CURLINFO_HEADER_IN:
		//LogDev(LOGCURLDEBUG, _T("%p:<= Recv header:\r\n%S"), handle,data);
		break;
	case CURLINFO_HEADER_OUT:
		//LogDev(LOGCURLDEBUG, _T("%p:=> Send header:\r\n%S"), handle,data);
		break;
	case CURLINFO_DATA_OUT:
		text = "=> Send data";
		break;
	case CURLINFO_SSL_DATA_OUT:
		text = "=> Send SSL data";
		break;
	case CURLINFO_DATA_IN:
		text = "<= Recv data";
		break;
	case CURLINFO_SSL_DATA_IN:
		text = "<= Recv SSL data";
		break;
	}
	return 0;
}

curl_socket_t CHttpRequest::opensocket(CHttpSession *pThis, curlsocktype purpose, struct curl_sockaddr *address)
{
	auto socket = pThis->openSocket(purpose, address);
	return socket.get() != nullptr? socket->tcpSocket.native_handle(): CURL_SOCKET_BAD;
}

int CHttpRequest::close_socket(CHttpSession *pThis, curl_socket_t item)
{
	return pThis->closeSocket(item);
}

int CHttpRequest::sockopt_callback(CHttpSession * pThis, curl_socket_t curlfd, curlsocktype purpose)
{
	int bufLen = 0;
	int len = sizeof(bufLen);
	::getsockopt((SOCKET)curlfd, SOL_SOCKET, SO_RCVBUF, (char *)&bufLen, &len);
	bufLen = 1024 * 8;
	//::setsockopt((SOCKET)curlfd, SOL_SOCKET, SO_RCVBUF, (char const*)&bufLen, sizeof(bufLen));
	unsigned long ul = true;
	if (ioctlsocket(curlfd, FIONBIO, &ul) == SOCKET_ERROR)
	{
		LogErrorEx(LOGFILTER, _T("ioctlsocket error"));
	}
	return CURL_SOCKOPT_OK;
}

#pragma endregion callback