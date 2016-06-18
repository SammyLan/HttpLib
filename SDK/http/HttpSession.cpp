#include "stdafx.h"
#include "HttpSession.h"
#include <iostream>
#include "HttpRequest.h"
#include "HttpConnMgr.h"
#include "HttpGlobal.h"


CHttpSession::CHttpSession(boost::asio::io_service & io_service, CHttpConnMgr * pConnMgr)
	:io_service_(io_service)
	,timer_(io_service)
	,pConnMgr_(pConnMgr)
{
	hMulti_ = curl_multi_init();

	curl_multi_setopt(hMulti_, CURLMOPT_SOCKETFUNCTION, socket_callback);
	curl_multi_setopt(hMulti_, CURLMOPT_SOCKETDATA, this);
	curl_multi_setopt(hMulti_, CURLMOPT_TIMERFUNCTION, timer_callback);
	curl_multi_setopt(hMulti_, CURLMOPT_TIMERDATA, this);
}


CHttpSession::~CHttpSession()
{
	curl_multi_cleanup(hMulti_);
}


CURLMcode CHttpSession::addHandle(CHttpRequest * pHandle)
{
	LogFinal(HTTPLOG,_T("\nAdding easy %p to multi %p (%S)"), pHandle->handle_, hMulti_, pHandle->url_.c_str());
	auto rc = curl_multi_add_handle(hMulti_, pHandle->getHandle());
	http::mcode_or_die("new_conn: curl_multi_add_handle", rc);
	return rc;
}

CURLMcode CHttpSession::removeHandle(CHttpRequest * pHandle)
{
	return curl_multi_remove_handle(hMulti_, pHandle->getHandle());
}

int CHttpSession::socket_callback(CURL *easy, curl_socket_t s, int what, CHttpSession *pThis, void *socketp)
{
	LogFinal(HTTPLOG,_T( "\nsock_cb: socket=%d, what=%d, sockp=%p"), s, what, socketp);

	int *actionp = (int *)socketp;
	const char *whatstr[] = { "none", "IN", "OUT", "INOUT", "REMOVE" };

	LogFinal(HTTPLOG,_T("\nsocket callback: socket=%d e=%p what=%S "), s, easy, whatstr[what]);

	if (what == CURL_POLL_REMOVE)
	{
		LogFinal(HTTPLOG,_T( "\n"));
		pThis->removeSocket(actionp);
	}
	else
	{
		if (!actionp)
		{
			LogFinal(HTTPLOG,_T( "\nAdding data: %S"), whatstr[what]);
			pThis->addSocket(s, easy, what);
		}
		else
		{
			LogFinal(HTTPLOG,_T("\nChanging action from %S to %S"),	whatstr[*actionp], whatstr[what]);
			pThis->setSocket(actionp, s, easy, what);
		}
	}

	return 0;
}

/* Update the event timer after curl_multi library calls */
int CHttpSession::timer_callback(CURLM *multi, long timeout_ms, CHttpSession *pThis)
{
	LogFinal(HTTPLOG,_T( "\nmulti_timer_cb: timeout_ms %ld"), timeout_ms);

	/* cancel running timer */
	pThis->timer_.cancel();
	if (timeout_ms > 0)
	{
		/* update timer */
		pThis->timer_.expires_from_now(boost::posix_time::millisec(timeout_ms));
		pThis->timer_.async_wait(std::bind(&timer_cb, std::placeholders::_1, pThis));
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
		LogFinal(HTTPLOG,_T( "\ntimer_cb: "));

		CURLMcode rc = curl_multi_socket_action(pThis->hMulti_, CURL_SOCKET_TIMEOUT, 0, &pThis->iStillRunning_);

		http::mcode_or_die("timer_cb: curl_multi_socket_action", rc);
		pThis->check_multi_info();
	}
}

/* Clean up any data */
void CHttpSession::removeSocket(int *f)
{
	LogFinal(HTTPLOG,_T( "\nremsock: "));

	if (f)
	{
		free(f);
	}
}

void CHttpSession::addSocket(curl_socket_t s, CURL *easy, int action)
{
	/* fdp is used to store current action */
	int *fdp = (int *)calloc(sizeof(int), 1);

	setSocket(fdp, s, easy, action);
	curl_multi_assign(hMulti_, s, fdp);
}

void CHttpSession::setSocket(int *fdp, curl_socket_t s, CURL*e, int act)
{
	LogFinal(HTTPLOG,_T( "\nsetsock: socket=%d, act=%d, fdp=%p"), s, act, fdp);

	auto tcp_socket = pConnMgr_->getSock(s);

	if (tcp_socket.get() == nullptr)
	{
		LogFinal(HTTPLOG,_T( "\nsocket %d is a c-ares socket, ignoring"), s);
		return;
	}

	*fdp = act;
	if (act == CURL_POLL_IN)
	{
		LogFinal(HTTPLOG,_T( "\nwatching for socket to become readable"));

		tcp_socket->async_read_some(boost::asio::null_buffers(),
			std::bind(&event_cb, this, tcp_socket, act));
	}
	else if (act == CURL_POLL_OUT)
	{
		LogFinal(HTTPLOG,_T( "\nwatching for socket to become writable"));

		tcp_socket->async_write_some(boost::asio::null_buffers(),
			std::bind(&event_cb, this, tcp_socket, act));
	}
	else if (act == CURL_POLL_INOUT)
	{
		LogFinal(HTTPLOG,_T( "\nwatching for socket to become readable & writable"));

		tcp_socket->async_read_some(boost::asio::null_buffers(),
			std::bind(&event_cb, this, tcp_socket, act));

		tcp_socket->async_write_some(boost::asio::null_buffers(),
			std::bind(&event_cb, this, tcp_socket, act));
	}
}

/* Check for completed transfers, and remove their easy handles */
void CHttpSession::check_multi_info()
{
	
	CURLMsg *msg;
	int msgs_left;
	CHttpRequest *conn;

	LogFinal(HTTPLOG,_T( "\nREMAINING: %d"), iStillRunning_);

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
void CHttpSession::event_cb(CHttpSession *pThis, http::TCPSocketPtr & tcp_socket,
	int action)
{
	LogFinal(HTTPLOG,_T( "\nevent_cb: action=%d"), action);

	CURLMcode rc;
	rc = curl_multi_socket_action(pThis->hMulti_, tcp_socket->native_handle(), action,
		&pThis->iStillRunning_);

	http::mcode_or_die("event_cb: curl_multi_socket_action", rc);
	pThis->check_multi_info();

	if (pThis->iStillRunning_ <= 0)
	{
		LogFinal(HTTPLOG,_T( "\nlast transfer done, kill timeout"));
		pThis->timer_.cancel();
	}
}

/* CURLOPT_OPENSOCKETFUNCTION */
http::TCPSocketPtr  CHttpSession::openSocket(curlsocktype purpose, struct curl_sockaddr *address)
{
	return pConnMgr_->opensocket(purpose, address);
}


/* CURLOPT_CLOSESOCKETFUNCTION */
int CHttpSession::closeSocket( curl_socket_t item)
{
	return pConnMgr_->close_socket(item);
}