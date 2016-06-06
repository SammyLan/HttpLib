#include "stdafx.h"
#include "HttpRequest.h"
#include "HttpSession.h"
#include <iostream>


CHttpRequest::CHttpRequest(CHttpSession *pSession)
	:pSession_(pSession)
{
	easy_ = curl_easy_init();
	if (!easy_)
	{
		fprintf(MSG_OUT, "\ncurl_easy_init() failed, exiting!");
		exit(2);
	}
	curl_easy_setopt(this->easy_, CURLOPT_WRITEFUNCTION, write_cb);
	curl_easy_setopt(this->easy_, CURLOPT_WRITEDATA, this);
	curl_easy_setopt(this->easy_, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt(this->easy_, CURLOPT_ERRORBUFFER, this->error);
	curl_easy_setopt(this->easy_, CURLOPT_PRIVATE, this);
	curl_easy_setopt(this->easy_, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(this->easy_, CURLOPT_PROGRESSFUNCTION, prog_cb);
	curl_easy_setopt(this->easy_, CURLOPT_PROGRESSDATA, this);
	curl_easy_setopt(this->easy_, CURLOPT_LOW_SPEED_TIME, 3L);
	curl_easy_setopt(this->easy_, CURLOPT_LOW_SPEED_LIMIT, 10L);

	/* call this function to get a socket */
	curl_easy_setopt(easy_, CURLOPT_OPENSOCKETFUNCTION, &CHttpRequest::opensocket);
	curl_easy_setopt(easy_, CURLOPT_OPENSOCKETDATA, this->pSession_);

	/* call this function to close a socket */
	curl_easy_setopt(this->easy_, CURLOPT_CLOSESOCKETFUNCTION, &CHttpRequest::close_socket);
	curl_easy_setopt(this->easy_, CURLOPT_CLOSESOCKETDATA, this->pSession_);

	curl_easy_setopt(this->easy_, CURLOPT_TCP_KEEPALIVE, 1L);
	curl_easy_setopt(this->easy_, CURLOPT_TCP_KEEPIDLE, 120L);
	curl_easy_setopt(this->easy_, CURLOPT_TCP_KEEPINTVL, 60L);
	curl_easy_setopt(this->easy_, CURLOPT_NOSIGNAL, 1);
}


CHttpRequest::~CHttpRequest()
{
}

CURL * CHttpRequest::handle()
{
	return easy_;
}

bool CHttpRequest::close()
{
	return true;
}

void CHttpRequest::onDone(CURLcode res)
{
	char *eff_url;
	curl_easy_getinfo(easy_, CURLINFO_EFFECTIVE_URL, &eff_url);
	fprintf(MSG_OUT, "\nDONE: %s => (%d) %s", eff_url, res, error);
	pSession_->removeHandle(this);
	curl_easy_cleanup(easy_);	
	//TODO::delete
	delete this;
}

/* CURLOPT_WRITEFUNCTION */
size_t CHttpRequest::write_cb(void *ptr, size_t size, size_t nmemb, CHttpRequest *pThis)
{
	size_t written = size * nmemb;
	char* pBuffer = (char *)malloc(written + 1);

	strncpy(pBuffer, (const char *)ptr, written);
	pBuffer[written] = '\0';

	fprintf(MSG_OUT, "%s", pBuffer);

	free(pBuffer);

	return written;
}


/* CURLOPT_PROGRESSFUNCTION */
int CHttpRequest::prog_cb(CHttpRequest *pThis, double dltotal, double dlnow, double ult, double uln)
{
	(void)ult;
	(void)uln;

	fprintf(MSG_OUT, "\nProgress: %s (%g/%g)", pThis->url_.c_str(), dlnow, dltotal);
	fprintf(MSG_OUT, "\nProgress: %s (%g)", pThis->url_.c_str(), ult);

	return 0;
}

void CHttpRequest::get(std::string const & url)
{
	url_ = url;
	curl_easy_setopt(this->easy_, CURLOPT_URL, this->url_.c_str());
	auto rc = pSession_->addHandle(this);
}

curl_socket_t CHttpRequest::opensocket(CHttpSession *pThis, curlsocktype purpose, struct curl_sockaddr *address)
{
	return pThis->opensocket(purpose,address);
}

int CHttpRequest::close_socket(CHttpSession *pThis, curl_socket_t item)
{
	return pThis->close_socket(item);
}