#ifndef YULIBEVENTSERVER_H_H
#define YULIBEVENTSERVER_H_H

/*
**@author: wite_chen
**@date:   20190830
**@brief:  The server of SimLibeventClient
*/

#include <stdio.h>
#include <string>
#include <errno.h>
#include <iostream>
using namespace std;

#include "event2/event.h"
#include "event2/listener.h"
#include "event2/bufferevent.h"
#include "event2/buffer.h"
#include "event2/thread.h"
#include "event2/util.h"

#include <winsock2.h>
#include <windows.h> 
#include <ws2tcpip.h>
#include <signal.h>

class LibeventHelper
{
public:
	LibeventHelper();
	~LibeventHelper();

	void  Init(int port);
	void  Start();
	void  Stop();

private:
	static LibeventHelper *pThis; // 存储回调函数调用的对象

	static void accept_conn_cb(evconnlistener *listener, evutil_socket_t fd,
					 struct sockaddr *sock, int socklen, void *arg);

	static void echo_read_cb(bufferevent *bev, void *arg);
	static void socket_write_cb(bufferevent *bev, void *arg);
	static void echo_event_cb(bufferevent *bev, short events, void *arg);
	static void signal_cb(evutil_socket_t sig, short events, void *arg);
	static void set_tcp_no_delay(evutil_socket_t fd);

public:
	event_base *m_pBase;
	evconnlistener *m_pListener;
	struct event* m_pEvstop;

};

#endif