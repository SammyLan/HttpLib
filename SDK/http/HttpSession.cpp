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

int CHttpSession::socket_callback(CURL *easy, curl_socket_t s, int what, CHttpSession *pThis, http::SocketInfo * sockInfo)
{
	const char *whatstr[] = { "none", "IN", "OUT", "INOUT", "REMOVE" };
	LogFinal(HTTPLOG,_T("sock_cb: socket=%d, what=%d, sockp=%p"), s, what, sockInfo);
	LogFinal(HTTPLOG,_T("socket callback: socket=%d e=%p what=%S "), s, easy, whatstr[what]);

	auto ret = pThis->pConnMgr_->getSock(s);
	if (ret.get() == nullptr)
	{
		return 0;
	}

	if (what == CURL_POLL_REMOVE)
	{
		LogFinal(HTTPLOG,_T( "CURL_POLL_REMOVE\n"));
		curl_multi_assign(pThis->hMulti_, s, NULL);
	}
	else
	{
		if (sockInfo == nullptr)
		{			
			sockInfo = ret.get();
			curl_multi_assign(pThis->hMulti_, s, sockInfo);
		}
		assert(ret.get() == sockInfo);
		LogFinal(HTTPLOG, _T("\nChanging action from %S to %S"), whatstr[sockInfo->mask], whatstr[what]);
		pThis->setSocket(ret, s, easy, what & (~sockInfo->mask));// only add new instrest
	}
	ret->mask = what;
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
		CURLMcode rc = curl_multi_socket_action(pThis->hMulti_, CURL_SOCKET_TIMEOUT, 0, &pThis->nStillRunning_);

		http::mcode_or_die("timer_cb: curl_multi_socket_action", rc);
		pThis->check_multi_info();
	}
}

void CHttpSession::setSocket(http::SocketInfoPtr & socketInfo, curl_socket_t s, CURL*e, int act)
{
	LogFinal(HTTPLOG,_T( "\nsetsock: socket=%d, act=%d"), s, act);
	if (socketInfo.get() == nullptr)
	{
		LogFinal(HTTPLOG,_T( "\nsocket %d is a c-ares socket, ignoring"), s);
		return;
	}
	assert(socketInfo->tcpSocket.native_handle() == s);
	auto & tcp_socket = socketInfo->tcpSocket;
	if (act == CURL_POLL_IN)
	{
		LogFinal(HTTPLOG,_T( "\nwatching for socket to become readable"));
		tcp_socket.async_read_some(boost::asio::null_buffers(),	std::bind(&event_cb, this, socketInfo,s,e, CURL_POLL_IN,std::placeholders::_1));
	}
	else if (act == CURL_POLL_OUT)
	{
		LogFinal(HTTPLOG,_T( "\nwatching for socket to become writable"));
		tcp_socket.async_write_some(boost::asio::null_buffers(),std::bind(&event_cb, this, socketInfo, s,e, CURL_POLL_OUT, std::placeholders::_1));
	}
	else if (act == CURL_POLL_INOUT)
	{
		LogFinal(HTTPLOG,_T( "\nwatching for socket to become readable & writable"));
		tcp_socket.async_read_some(boost::asio::null_buffers(),std::bind(&event_cb, this, socketInfo, s, e,CURL_POLL_IN, std::placeholders::_1));
		tcp_socket.async_write_some(boost::asio::null_buffers(),std::bind(&event_cb, this, socketInfo, s, e,CURL_POLL_OUT, std::placeholders::_1));
	}
}

/* Check for completed transfers, and remove their easy handles */
void CHttpSession::check_multi_info()
{
	
	CURLMsg *msg;
	int msgs_left;
	CHttpRequest *conn;

	LogFinal(HTTPLOG,_T( "\nREMAINING: %d"), nStillRunning_);

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
void CHttpSession::event_cb(CHttpSession *pThis, http::SocketInfoPtr& tcp_socket, curl_socket_t s, CURL*e, int action, const boost::system::error_code &err)
{
	LogFinal(HTTPLOG,_T( "\nevent_cb: action=%d"), action);
	assert(tcp_socket->tcpSocket.native_handle() == s);
	CURLMcode rc = CURLM_OK;
	if (err)
	{
		LogFinal(HTTPLOG, _T("\nevent_cb: socket=%d action=%d \nERROR=%S"), s, action, err.message().c_str());
		rc = curl_multi_socket_action(pThis->hMulti_, tcp_socket->tcpSocket.native_handle(), CURL_CSELECT_ERR, &pThis->nStillRunning_);
	}
	else
	{
		LogFinal(HTTPLOG, _T("\nevent_cb: socket=%d action=%d"), s, action);
		rc = curl_multi_socket_action(pThis->hMulti_, tcp_socket->tcpSocket.native_handle(), action, &pThis->nStillRunning_);
	}

	http::mcode_or_die("event_cb: curl_multi_socket_action", rc);
	pThis->check_multi_info();

	if (pThis->nStillRunning_ <= 0)
	{
		LogFinal(HTTPLOG,_T( "\nlast transfer done, kill timeout"));
		pThis->timer_.cancel();
	}
	else //живЊ
	{
		int action_continue = (tcp_socket->mask) & action;
		if (action_continue)
		{
			LogFinal(HTTPLOG, _T("\ncontinue read or write: %d"), action_continue);
			pThis->setSocket(tcp_socket, s, e, action_continue); // continue read or write
		}
	}
}

/* CURLOPT_OPENSOCKETFUNCTION */
http::SocketInfoPtr  CHttpSession::openSocket(curlsocktype purpose, struct curl_sockaddr *address)
{
	return pConnMgr_->opensocket(purpose, address);
}


/* CURLOPT_CLOSESOCKETFUNCTION */
int CHttpSession::closeSocket( curl_socket_t item)
{
	return pConnMgr_->close_socket(item);
}