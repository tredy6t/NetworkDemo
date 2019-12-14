#include "LibeventHelper.h"

/*
**@author: wite_chen
**@date:   20190830
**@brief:  The server of SimLibeventClient
*/

static int s_iBlockSize = 10;
#define MAX_LINE 1024
LibeventHelper *LibeventHelper::pThis = NULL;

LibeventHelper::LibeventHelper()
{
	pThis   = this; //��thisָ�븳��pThis��ʹ�ûص�������ͨ��pThisָ����ʱ�����
	m_pBase = NULL;
	m_pListener = NULL;
	m_pEvstop = NULL;

}


LibeventHelper::~LibeventHelper()
{

}

/*
**@author: wite_chen
**@date:   20190830
**@param:  evutil_socket_t fd
**@brief:  ���÷���������ֹNagle�㷨��
*/
void LibeventHelper::set_tcp_no_delay(evutil_socket_t fd)
{
	int iOne = 1;
	setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char*)&iOne, sizeof iOne);
}



/*
**@author: wite_chen
**@date:   20190830
**@param:  evutil_socket_t fd
**@brief:  �ȴ����ܿͻ������Ӵ���accept��һ���¿ͻ��������Ϸ�������
*/
void LibeventHelper::accept_conn_cb(evconnlistener *listener, evutil_socket_t fd,
							 struct sockaddr *sock, int socklen, void *arg)
{
	printf("We got a new connection! Set up a bufferevent for it. accept a client %d\n", fd);

	event_base *base = evconnlistener_get_base(listener);

	//Ϊ����ͻ��˷���һ��bufferevent
	bufferevent *bev =  bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);

	set_tcp_no_delay(fd);

	bufferevent_setcb(bev, echo_read_cb, NULL, echo_event_cb, NULL);
	bufferevent_enable(bev, EV_READ | EV_WRITE);

}

/*
**@author: wite_chen
**@date:   20190830
**@param:  bufferevent *bev, void *arg
**@brief:  echo_read_cb�ص��ӿ�
*/
void LibeventHelper::echo_read_cb(bufferevent *bev, void *arg)
{
	
	char msg[MAX_LINE + 1] = {0};
	int iLen = 0;
	evutil_socket_t fd = bufferevent_getfd(bev);
	while (iLen = bufferevent_read(bev, msg, sizeof(msg)-1 ), iLen > 0)
	{
		msg[iLen] = '\0';
		printf("fd=%u, read len = %d\t read msg: %s\n", fd, iLen, msg);
        std::string strMsg = "Welcome to join";
        int iRst = bufferevent_write(bev, strMsg.c_str(), strMsg.length());
		if (-1 == iRst)
		{
			printf("[socket_write_cb]:error occur!\n");
		}
	}

	/*
	char reply[] = "[server: i'm server, send 1111]";
	printf("writecb: len = %d\n", 1 + strlen(reply));
	int iRst = bufferevent_write(bev, reply, 1 + strlen(reply));
	if (-1 == iRst)
	{
		printf("[socket_write_cb]:error occur!\n");
	}
	*/
	/*This callback is invoked when there is data to read on bev */
	//struct evbuffer *input = bufferevent_get_input(bev);
	//struct evbuffer *output = bufferevent_get_output(bev);
	/*��input buffer�е��������� ������ output buffer*/
	//evbuffer_add_buffer(output, input);


}

/*
**@author: wite_chen
**@date:   20190830
**@param:  bufferevent *bev, void *arg
**@brief:  socket_write_cb�ص��ӿڣ���ʱδʹ��
*/
void LibeventHelper::socket_write_cb(bufferevent *bev, void *arg)
{
	/*
	char reply[] = "[server: i'm server, send 1111]";
	printf("writecb: len = %d\n", 1 + strlen(reply));
	int iRst = bufferevent_write(bev, reply, 1 + strlen(reply));
	if (-1 == iRst)
	{
		printf("[socket_write_cb]:error occur!\n");
	}
	*/
}

/*
**@author: wite_chen
**@date:   20190830
**@param:  bufferevent *bev, short events, void *arg
**@brief:  echo_event_cb�¼�������쳣����
*/
void LibeventHelper::echo_event_cb(bufferevent *bev, short events, void *arg)
{
	struct evbuffer *output = bufferevent_get_output(bev);
	size_t remain = evbuffer_get_length(output);

	if (events & BEV_EVENT_TIMEOUT)
	{
		printf("Timed out\n");  //if bufferevent_set_timeouts() called.
	}
	else if (events & BEV_EVENT_EOF)
	{
		printf("connection closed, remain %d\n", remain);
	}
	else if (events & BEV_EVENT_ERROR)
	{
		printf("some other error, remain %d\n", remain);
	}
	//�⽫�Զ�close�׽��ֺ�free��д������
	bufferevent_free(bev);
}

/*
**@author: wite_chen
**@date:   20190830
**@param:  bufferevent *bev, short events, void *arg
**@brief:  signal_cbֹͣ�źŴ���
*/
void LibeventHelper::signal_cb(evutil_socket_t sig, short events, void *arg)
{
	struct event_base *base = (event_base *)arg;
	printf("exception: interrupt, stop now!\n");

	event_base_loopexit(base, NULL);
}

/*
**@author: wite_chen
**@date:   20190830
**@param:  int port, ����˿ڡ�
**@brief:  libevent��socket��ʼ����
*/
void  LibeventHelper::Init(int port)
{
#ifdef WIN32
    WSADATA wsaData;
    DWORD Ret;
    if ((Ret = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0)
    {
        printf("WSAStartup failed with error %d\n", Ret);
        exit(1);
    }
#endif // WIN32

	m_pBase = event_base_new();
	if (NULL == m_pBase)
	{
		printf("couldn't open event base!\n");
		exit(1);
	}

	
	m_pEvstop = evsignal_new(m_pBase, SIGINT, signal_cb, m_pBase);
	evsignal_add(m_pEvstop, NULL);

	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(struct sockaddr_in));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);

	m_pListener = evconnlistener_new_bind(m_pBase, accept_conn_cb, NULL,
								LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE,
								-1, (struct sockaddr*)&sin,
								sizeof(struct sockaddr_in));
	
	if (NULL == m_pListener)
	{
		printf("couldn't create listener!\n");
		exit(1);
	}
}

/*
**@author: wite_chen
**@date:   20190830
**@param:  ��
**@brief:  ������ѭ��ִ��
*/
void  LibeventHelper::Start()
{
	event_base_dispatch(m_pBase);
}

/*
**@author: wite_chen
**@date:   20190830
**@param:  ��
**@brief:  ֹͣ
*/
void  LibeventHelper::Stop()
{
	if (NULL != m_pListener)
	{
		evconnlistener_free(m_pListener);
	}
	if (NULL != m_pEvstop)
	{
		event_free(m_pEvstop);
	}
    
	if (NULL != m_pBase)
	{
		event_base_free(m_pBase);
	}
    event_base_loopexit(m_pBase, nullptr);
}

