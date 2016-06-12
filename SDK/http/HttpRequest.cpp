#include "stdafx.h"
#include "HttpRequest.h"
#include "HttpSession.h"
#include <iostream>
#include <fstream>

std::string CHttpRequest::strProxyHost;
std::string CHttpRequest::strProxyUsrPwd;

TCHAR LOGCURLDEBUG[] = _T("CURLDEBUG");
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
	handle_ = curl_easy_init();
	if (!handle_)
	{
		LogFinal(HTTPLOG,_T("\ncurl_easy_init() failed, exiting!"));
		exit(2);
	}
#ifdef DEBUG
	curl_easy_setopt(this->handle_, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt(this->handle_, CURLOPT_DEBUGFUNCTION, &CHttpRequest::debug_callback);
	curl_easy_setopt(this->handle_, CURLOPT_DEBUGDATA, this);
#endif
	curl_easy_setopt(this->handle_, CURLOPT_WRITEFUNCTION, write_cb);
	curl_easy_setopt(this->handle_, CURLOPT_WRITEDATA, this);
	
	curl_easy_setopt(this->handle_, CURLOPT_ERRORBUFFER, this->error);
	curl_easy_setopt(this->handle_, CURLOPT_PRIVATE, this);
	//curl_easy_setopt(this->handle_, CURLOPT_NOPROGRESS, 1L);
	//curl_easy_setopt(this->handle_, CURLOPT_PROGRESSFUNCTION, prog_cb);
	//curl_easy_setopt(this->handle_, CURLOPT_PROGRESSDATA, this);
	curl_easy_setopt(this->handle_, CURLOPT_LOW_SPEED_TIME, 3L);
	curl_easy_setopt(this->handle_, CURLOPT_LOW_SPEED_LIMIT, 10L);

	/* call this function to get a socket */
	curl_easy_setopt(handle_, CURLOPT_OPENSOCKETFUNCTION, &CHttpRequest::opensocket);
	curl_easy_setopt(handle_, CURLOPT_OPENSOCKETDATA, this->pSession_);

	/* call this function to close a socket */
	curl_easy_setopt(this->handle_, CURLOPT_CLOSESOCKETFUNCTION, &CHttpRequest::close_socket);
	curl_easy_setopt(this->handle_, CURLOPT_CLOSESOCKETDATA, this->pSession_);

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
	
}

CHttpRequest::~CHttpRequest()
{
}

void CHttpRequest::setUrl(const std::string&url, const http::Parameters & para)
{
	if (!para.content.empty())
	{
		url_ = url + "?" + para.content;
	}
	else
	{
		url_ = url;
	}
	curl_easy_setopt(handle_, CURLOPT_URL, url.data());
}

void CHttpRequest::setHeader(const http::Header & header)
{
	curl_slist* chunk = nullptr;
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
		chunk = curl_slist_append(chunk, header_string.data());
		curl_easy_setopt(handle_, CURLOPT_HTTPHEADER, chunk);
	}
}

void CHttpRequest::setFormContent(const std::vector<std::pair<std::string, std::string>> & vecName2Content)
{
	curl_httppost *lastptr = nullptr;
	curl_httppost *formpost = nullptr;
	for (size_t i = 0; i < vecName2Content.size(); ++i)
	{
		curl_formadd(&formpost, &lastptr,
			CURLFORM_COPYNAME, vecName2Content[i].first.c_str(),
			CURLFORM_COPYCONTENTS, vecName2Content[i].second.c_str(),
			CURLFORM_CONTENTSLENGTH, vecName2Content[i].second.length(),
			CURLFORM_END);
	}
	curl_easy_setopt(handle_, CURLOPT_HTTPPOST, formpost);
}

void CHttpRequest::get(std::string const & url)
{
	getWithParam(url, http::Parameters{});
}

void CHttpRequest::getWithParam(std::string const & url, const http::Parameters & para)
{
	setUrl(url, para);
	auto rc = pSession_->addHandle(this);
}

bool CHttpRequest::close()
{
	return true;
}

void CHttpRequest::onDone(CURLcode res)
{
	char *eff_url;
	curl_easy_getinfo(handle_, CURLINFO_EFFECTIVE_URL, &eff_url);
	LogFinal(HTTPLOG,_T("\nDONE: %S => (%d) %S"), eff_url, res, error);
	pSession_->removeHandle(this);
	curl_easy_cleanup(handle_);	
	//TODO::delete
	delete this;
}

#pragma region delegate
/* CURLOPT_WRITEFUNCTION */
size_t CHttpRequest::write_cb(void *ptr, size_t size, size_t nmemb, CHttpRequest *pThis)
{
	std::ofstream ofs("d:\\qq.html", ios_base::app);
	size_t written = size * nmemb;
	char* pBuffer = (char *)malloc(written + 1);

	strncpy(pBuffer, (const char *)ptr, written);
	pBuffer[written] = '\0';

	ofs.write(pBuffer, written);
	free(pBuffer);

	return written;
}

/* CURLOPT_PROGRESSFUNCTION */
/*int CHttpRequest::prog_cb(CHttpRequest *pThis, double dltotal, double dlnow, double ult, double uln)
{
	(void)ult;
	(void)uln;

	LogFinal(HTTPLOG,_T("\nProgress: %S (%g/%g)"), pThis->url_.c_str(), dlnow, dltotal);
	LogFinal(HTTPLOG,_T("\nProgress: %S (%g)"), pThis->url_.c_str(), ult);

	return 0;
}*/

int CHttpRequest::debug_callback(CURL *handle, curl_infotype type, char *data, size_t size, CHttpRequest * pThis)
{
	const char *text;
	(void)handle; /* prevent compiler warning */

	switch (type){
	case CURLINFO_TEXT:
		LogFinal(LOGCURLDEBUG, _T("%p:== Info:\r\n%S"), handle,data);
		break;	
	case CURLINFO_HEADER_IN:
		LogFinal(LOGCURLDEBUG, _T("%p:<= Recv header:\r\n%S"), handle,data);
		break;
	case CURLINFO_HEADER_OUT:
		LogFinal(LOGCURLDEBUG, _T("%p:=> Send header:\r\n%S"), handle,data);
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
	return pThis->opensocket(purpose,address);
}

int CHttpRequest::close_socket(CHttpSession *pThis, curl_socket_t item)
{
	return pThis->close_socket(item);
}
#pragma endregion delegate