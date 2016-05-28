#include "stdafx.h"
#include "HttpRequest.h"
#include "HttpSession.h"
#include <iostream>


CHttpRequest::CHttpRequest(CHttpSession *pSession)
	:pSession_(pSession)
{
	easy = curl_easy_init();
	if (!easy)
	{
		fprintf(MSG_OUT, "\ncurl_easy_init() failed, exiting!");
		exit(2);
	}
	curl_easy_setopt(this->easy, CURLOPT_WRITEFUNCTION, write_cb);
	curl_easy_setopt(this->easy, CURLOPT_WRITEDATA, this);
	curl_easy_setopt(this->easy, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt(this->easy, CURLOPT_ERRORBUFFER, this->error);
	curl_easy_setopt(this->easy, CURLOPT_PRIVATE, this);
	curl_easy_setopt(this->easy, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(this->easy, CURLOPT_PROGRESSFUNCTION, prog_cb);
	curl_easy_setopt(this->easy, CURLOPT_PROGRESSDATA, this);
	curl_easy_setopt(this->easy, CURLOPT_LOW_SPEED_TIME, 3L);
	curl_easy_setopt(this->easy, CURLOPT_LOW_SPEED_LIMIT, 10L);

	/* call this function to get a socket */
	curl_easy_setopt(easy, CURLOPT_OPENSOCKETFUNCTION, &CHttpSession::opensocket);

	/* call this function to close a socket */
	curl_easy_setopt(this->easy, CURLOPT_CLOSESOCKETFUNCTION, &CHttpSession::close_socket);	
}


CHttpRequest::~CHttpRequest()
{
}

CURL * CHttpRequest::Handle()
{
	return easy;
}

bool CHttpRequest::Close()
{
	return true;
}

void CHttpRequest::OnDone(CURLcode res)
{
	char *eff_url;
	curl_easy_getinfo(easy, CURLINFO_EFFECTIVE_URL, &eff_url);
	fprintf(MSG_OUT, "\nDONE: %s => (%d) %s", eff_url, res, error);
	pSession_->RemoveHandle(this);
	curl_easy_cleanup(easy);	
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

void CHttpRequest::Get(std::string const & url)
{
	url_ = url;
	curl_easy_setopt(this->easy, CURLOPT_URL, this->url_.c_str());
	auto rc = pSession_->AddHandle(this);
}