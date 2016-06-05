#include "stdafx.h"
#include "HttpSession.h"
#include <iostream>
#include "HttpRequest.h"

#ifdef DEBUG
#pragma comment(lib,"libcurl_a_debug.lib")
#else
#pragma comment(lib,"libcurl_a.lib")
#endif // DEBUG


namespace NSCURL
{
	bool CurlInit()
	{
		return true;
	}

	bool CurlUninit()
	{
		return true;
	}

	void mcode_or_die(const char *where, CURLMcode code)
	{
		if (CURLM_OK != code)
		{
			const char *s;
			switch (code)
			{
			case CURLM_CALL_MULTI_PERFORM:
				s = "CURLM_CALL_MULTI_PERFORM";
				break;
			case CURLM_BAD_HANDLE:
				s = "CURLM_BAD_HANDLE";
				break;
			case CURLM_BAD_EASY_HANDLE:
				s = "CURLM_BAD_EASY_HANDLE";
				break;
			case CURLM_OUT_OF_MEMORY:
				s = "CURLM_OUT_OF_MEMORY";
				break;
			case CURLM_INTERNAL_ERROR:
				s = "CURLM_INTERNAL_ERROR";
				break;
			case CURLM_UNKNOWN_OPTION:
				s = "CURLM_UNKNOWN_OPTION";
				break;
			case CURLM_LAST:
				s = "CURLM_LAST";
				break;
			default:
				s = "CURLM_unknown";
				break;
			case CURLM_BAD_SOCKET:
				s = "CURLM_BAD_SOCKET";
				fprintf(MSG_OUT, "\nERROR: %s returns %s", where, s);
				/* ignore this error */
				return;
			}
			assert(false);
			fprintf(MSG_OUT, "\nERROR: %s returns %s", where, s);
			exit(code);
		}
	}
}


CHttpSession::CHttpSession(boost::asio::io_service & io_service)
	:io_service_(io_service)
	, timer_(io_service)
{
	hMulti_ = curl_multi_init();

	curl_multi_setopt(hMulti_, CURLMOPT_SOCKETFUNCTION, sock_cb);
	curl_multi_setopt(hMulti_, CURLMOPT_SOCKETDATA, this);
	curl_multi_setopt(hMulti_, CURLMOPT_TIMERFUNCTION, multi_timer_cb);
	curl_multi_setopt(hMulti_, CURLMOPT_TIMERDATA, this);
}


CHttpSession::~CHttpSession()
{
	curl_multi_cleanup(hMulti_);
}


CURLMcode CHttpSession::addHandle(CHttpRequest * pHandle)
{
	fprintf(MSG_OUT,"\nAdding easy %p to multi %p (%s)", pHandle->easy_, hMulti_, pHandle->url_.c_str());
	auto rc = curl_multi_add_handle(hMulti_, pHandle->handle());
	NSCURL::mcode_or_die("new_conn: curl_multi_add_handle", rc);
	return rc;
}

CURLMcode CHttpSession::removeHandle(CHttpRequest * pHandle)
{
	return curl_multi_remove_handle(hMulti_, pHandle->handle());
}

CURLM * CHttpSession::handle()
{
	return hMulti_;
}

int CHttpSession::sock_cb(CURL *e, curl_socket_t s, int what, CHttpSession *pThis, void *sockp)
{
	fprintf(MSG_OUT, "\nsock_cb: socket=%d, what=%d, sockp=%p", s, what, sockp);

	int *actionp = (int *)sockp;
	const char *whatstr[] = { "none", "IN", "OUT", "INOUT", "REMOVE" };

	fprintf(MSG_OUT,
		"\nsocket callback: s=%d e=%p what=%s ", s, e, whatstr[what]);

	if (what == CURL_POLL_REMOVE)
	{
		fprintf(MSG_OUT, "\n");
		pThis->remsock(actionp);
	}
	else
	{
		if (!actionp)
		{
			fprintf(MSG_OUT, "\nAdding data: %s", whatstr[what]);
			pThis->addsock(s, e, what);
		}
		else
		{
			fprintf(MSG_OUT,
				"\nChanging action from %s to %s",
				whatstr[*actionp], whatstr[what]);
			pThis->setsock(actionp, s, e, what);
		}
	}

	return 0;
}

/* Update the event timer after curl_multi library calls */
int CHttpSession::multi_timer_cb(CURLM *multi, long timeout_ms, CHttpSession *pThis)
{
	fprintf(MSG_OUT, "\nmulti_timer_cb: timeout_ms %ld", timeout_ms);

	/* cancel running timer */
	pThis->timer_.cancel();

	if (timeout_ms > 0)
	{
		/* update timer */
		pThis->timer_.expires_from_now(boost::posix_time::millisec(timeout_ms));
		pThis->timer_.async_wait(boost::bind(&timer_cb, _1, pThis));
	}
	else
	{
		/* call timeout function immediately */
		boost::system::error_code error; /*success*/
		timer_cb(error, pThis);
	}

	return 0;
}

void CHttpSession::timer_cb(const boost::system::error_code & error, CHttpSession *pThis)
{
	if (!error)
	{
		fprintf(MSG_OUT, "\ntimer_cb: ");

		CURLMcode rc;
		rc = curl_multi_socket_action(pThis->hMulti_, CURL_SOCKET_TIMEOUT, 0, &pThis->iStillRunning_);

		NSCURL::mcode_or_die("timer_cb: curl_multi_socket_action", rc);
		pThis->check_multi_info();
	}
}

/* Clean up any data */
void CHttpSession::remsock(int *f)
{
	fprintf(MSG_OUT, "\nremsock: ");

	if (f)
	{
		free(f);
	}
}

void CHttpSession::addsock(curl_socket_t s, CURL *easy, int action)
{
	/* fdp is used to store current action */
	int *fdp = (int *)calloc(sizeof(int), 1);

	setsock(fdp, s, easy, action);
	curl_multi_assign(hMulti_, s, fdp);
}

void CHttpSession::setsock(int *fdp, curl_socket_t s, CURL*e, int act)
{
	fprintf(MSG_OUT, "\nsetsock: socket=%d, act=%d, fdp=%p", s, act, fdp);

	std::map<curl_socket_t, boost::asio::ip::tcp::socket *>::iterator it = socketMap_.find(s);

	if (it == socketMap_.end())
	{
		fprintf(MSG_OUT, "\nsocket %d is a c-ares socket, ignoring", s);

		return;
	}

	boost::asio::ip::tcp::socket * tcp_socket = it->second;

	*fdp = act;

	if (act == CURL_POLL_IN)
	{
		fprintf(MSG_OUT, "\nwatching for socket to become readable");

		tcp_socket->async_read_some(boost::asio::null_buffers(),
			boost::bind(&event_cb, this, tcp_socket, act));
	}
	else if (act == CURL_POLL_OUT)
	{
		fprintf(MSG_OUT, "\nwatching for socket to become writable");

		tcp_socket->async_write_some(boost::asio::null_buffers(),
			boost::bind(&event_cb, this, tcp_socket, act));
	}
	else if (act == CURL_POLL_INOUT)
	{
		fprintf(MSG_OUT, "\nwatching for socket to become readable & writable");

		tcp_socket->async_read_some(boost::asio::null_buffers(),
			boost::bind(&event_cb, this, tcp_socket, act));

		tcp_socket->async_write_some(boost::asio::null_buffers(),
			boost::bind(&event_cb, this, tcp_socket, act));
	}
}

/* Check for completed transfers, and remove their easy handles */
void CHttpSession::check_multi_info()
{
	
	CURLMsg *msg;
	int msgs_left;
	CHttpRequest *conn;

	fprintf(MSG_OUT, "\nREMAINING: %d", iStillRunning_);

	while ((msg = curl_multi_info_read(hMulti_, &msgs_left)))
	{
		if (msg->msg == CURLMSG_DONE)
		{
			auto easy = msg->easy_handle;
			auto res = msg->data.result;
			curl_easy_getinfo(easy, CURLINFO_PRIVATE, &conn);
			conn->onDone(res);
		}
	}
}

/* Called by asio when there is an action on a socket */
void CHttpSession::event_cb(CHttpSession *pThis, boost::asio::ip::tcp::socket *tcp_socket,
	int action)
{
	fprintf(MSG_OUT, "\nevent_cb: action=%d", action);

	CURLMcode rc;
	rc = curl_multi_socket_action(pThis->hMulti_, tcp_socket->native_handle(), action,
		&pThis->iStillRunning_);

	NSCURL::mcode_or_die("event_cb: curl_multi_socket_action", rc);
	pThis->check_multi_info();

	if (pThis->iStillRunning_ <= 0)
	{
		fprintf(MSG_OUT, "\nlast transfer done, kill timeout");
		pThis->timer_.cancel();
	}
}

/* CURLOPT_OPENSOCKETFUNCTION */
curl_socket_t CHttpSession::opensocket(curlsocktype purpose, struct curl_sockaddr *address)
{
	fprintf(MSG_OUT, "\nopensocket :");

	curl_socket_t sockfd = CURL_SOCKET_BAD;

	/* restrict to IPv4 */
	if (purpose == CURLSOCKTYPE_IPCXN && address->family == AF_INET)
	{
		/* create a tcp socket object */
		boost::asio::ip::tcp::socket *tcp_socket = new boost::asio::ip::tcp::socket(io_service_);

		/* open it and get the native handle*/
		boost::system::error_code ec;
		tcp_socket->open(boost::asio::ip::tcp::v4(), ec);

		if (ec)
		{
			/* An error occurred */
			std::cout << std::endl << "Couldn't open socket [" << ec << "][" << ec.message() << "]";
			fprintf(MSG_OUT, "\nERROR: Returning CURL_SOCKET_BAD to signal error");
		}
		else
		{
			sockfd = tcp_socket->native_handle();
			fprintf(MSG_OUT, "\nOpened socket %d", sockfd);

			/* save it for monitoring */
			socketMap_.insert(std::pair<curl_socket_t, boost::asio::ip::tcp::socket *>(sockfd, tcp_socket));
		}
	}

	return sockfd;
}


/* CURLOPT_CLOSESOCKETFUNCTION */
int CHttpSession::close_socket( curl_socket_t item)
{
	fprintf(MSG_OUT, "\nclose_socket : %d", item);

	std::map<curl_socket_t, boost::asio::ip::tcp::socket *>::iterator it = socketMap_.find(item);

	if (it != socketMap_.end())
	{
		delete it->second;
		socketMap_.erase(it);
	}
	return 0;
}