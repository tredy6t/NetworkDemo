#include "LibeventHelper.h"

/*
**@author: wite_chen
**@date:   20190830
**@brief:  The client of SimLibeventClient
*/

LibeventHelper *LibeventHelper::pThis = NULL;
const static char* s_serverIpAddr = "127.0.0.1";
const static int s_iBlockSize = 10;
const static long s_iTimeOut = 10;  //超时时间
const static int s_iSessionCnt = 1;
int LibeventHelper::m_siLtotal_bytes_read = 0;
int LibeventHelper::m_siLtotal_messages_read = 0;


LibeventHelper::LibeventHelper()
{
	pThis   = this; //将this指针赋给pThis，使得回调函数能通过pThis指针访问本对象
	m_pBase = NULL;
	m_pszMsg = NULL;
	m_evtimeout = NULL;
	m_bevs = NULL;
}


LibeventHelper::~LibeventHelper()
{

}

/*
**@author: wite_chen
**@date:   20190830
**@param:  evutil_socket_t fd
**@brief:  设置非阻塞，禁止Nagle算法。
*/
void LibeventHelper::set_tcp_no_delay(evutil_socket_t fd)
{
	int iOne = 1;
	setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char*)&iOne, sizeof iOne);
}

/*
**@author: wite_chen
**@date:   20190830
**@param:  evutil_socket_t fd, short events, void *arg
**@brief:  超时回调函数。
*/
void LibeventHelper::timeoutcb(evutil_socket_t fd, short events, void *arg)
{
	struct event_base *base = (event_base*)arg;
	printf("timeout...\n");

	event_base_loopexit(base, NULL);
}

void LibeventHelper::signal_cb(evutil_socket_t sig, short events, void *arg)
{
    struct event_base *base = (event_base *)arg;
    printf("exception: interrupt, stop now!\n");

    event_base_loopexit(base, NULL);
}

/*
**@author: wite_chen
**@date:   20190830
**@param:  int fd, short events, void* arg
**@brief:  暂时未使用
*/
void LibeventHelper::cmd_msg_cb(int fd, short events, void* arg)
{
	printf("server_msg_cb ing....\n");
	struct bufferevent* bev = (struct bufferevent*)arg;

	char msg[1024] = "testlaoyang20161210";
	int iLen = 1 + strlen(msg);
	/*int iLen = bufferevent_read(bev, msg, sizeof(msg));
	if (0 == iLen)
	{
		printf("recv message empty.\n");
		exit(1);
	}*/
	
	//把终端的消息发送给服务器端
	bufferevent_write(bev, msg, iLen);
}

/*
**@author: wite_chen
**@date:   20190830
**@param:  struct bufferevent* bev, void* arg
**@brief:  writecb回调函数，暂时未使用
*/
void LibeventHelper::writecb(struct bufferevent* bev, void* arg)
{
	/*
	printf("send_server_cb running....\n");

	char szSendMsg[1024] = "[writecb: i'am client]";
	int iLen = 1 + strlen(szSendMsg);
	printf("iLen = %d\n", iLen);
	//把终端的消息发送给服务器端
	bufferevent_write(bev, szSendMsg, iLen);
	*/
}

/*
**@author: wite_chen
**@date:   20190830
**@param:  struct bufferevent* bev, void* arg
**@brief:  readcb回调函数，接收处理回调接口。
*/
void LibeventHelper::readcb(struct bufferevent* bev, void* arg)
{
	char szRecvMsg[1024] = {0};
	int len = bufferevent_read(bev, szRecvMsg, sizeof(szRecvMsg));
	szRecvMsg[len] = '\0';
    printf("recv from server: cnt = %d, len = %d, msg = %s\n", m_siLtotal_messages_read, len, szRecvMsg);

	++m_siLtotal_messages_read;
	m_siLtotal_bytes_read += len;

	//把终端的消息发送给服务器端
    std::string strMsg = "this is a test";
    bufferevent_write(bev, strMsg.c_str(), strMsg.length());

	//以下是chenshuo的使用方法
	/*This callback is invoked when there is data to read on bev @by chenshuo below */
	//struct evbuffer *input = bufferevent_get_input(bev);
	//struct evbuffer *output = bufferevent_get_output(bev);
	//++m_siLtotal_messages_read;
	//m_siLtotal_bytes_read += evbuffer_get_length(input);
	//evbuffer_add_buffer(output, input);

}

/*
**@author: wite_chen
**@date:   20190830
**@param:  struct bufferevent *bev, short event, void *arg
**@brief:  eventcb回调函数，事件或出错处理回调接口。
*/
void LibeventHelper::eventcb(struct bufferevent *bev, short event, void *arg)
{

	if (event & BEV_EVENT_EOF)
	{
		printf("connection closed\n");
	}
	else if (event & BEV_EVENT_ERROR)
	{
		printf("some other error\n");
	}
	else if( event & BEV_EVENT_CONNECTED)
	{
		printf("the client has connected to server\n");
		evutil_socket_t fd = bufferevent_getfd(bev);
		set_tcp_no_delay(fd);
	}
}


/*
**@author: wite_chen
**@date:   20190830
**@param:  int iPort, 传入端口。
**@brief:  libevent，socket初始化等
*/
void LibeventHelper::Init(int iPort)
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

	m_timeout.tv_sec = s_iTimeOut;  //60s超时
	m_timeout.tv_usec = 0;

	m_pszMsg = (char*)malloc(1 + s_iBlockSize);
	memset(m_pszMsg, 0, s_iBlockSize);
	for (int i = 0; i < s_iBlockSize; ++i)
	{
		m_pszMsg[i] = 't'; /*i%128;*/
	}
	m_pszMsg[s_iBlockSize] = '\0';
	//printf("m_pszMsg = %s\n", m_pszMsg);

	m_pBase = event_base_new();
	if (!m_pBase)
	{
		printf("Couldn't open event base!\n");
		exit(1);
	}

	//设定超时
	m_evtimeout = evtimer_new(m_pBase, timeoutcb, m_pBase);
	evtimer_add(m_evtimeout, &m_timeout);

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr) );
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(iPort);
	server_addr.sin_addr.s_addr = inet_addr(s_serverIpAddr);

	m_bevs = (bufferevent**)malloc(s_iSessionCnt * sizeof(struct bufferevent *));
	for (int i=0; i < s_iSessionCnt; ++i)
	{
		struct bufferevent* bev = bufferevent_socket_new(m_pBase, -1, BEV_OPT_CLOSE_ON_FREE);
		bufferevent_setcb(bev, readcb, NULL, eventcb, NULL);
		bufferevent_enable(bev, EV_READ | EV_WRITE);

		evbuffer_add(bufferevent_get_output(bev), m_pszMsg, s_iBlockSize);

		if (bufferevent_socket_connect(bev, (struct sockaddr *)&server_addr,
			sizeof(server_addr)) < 0)
		{
			printf("Error starting connection!\n");
			bufferevent_free(bev);
			exit(1);
		}	
		m_bevs[i] = bev;
	}
	
	
}

/*
**@author: wite_chen
**@date:   20190830
**@param:  无
**@brief:  启动，循环执行
*/
void  LibeventHelper::Start()
{
	event_base_dispatch(m_pBase);
}

/*
**@author: wite_chen
**@date:   20190830
**@param:  无
**@brief:  停止，内存等释放&结果统计
*/
void  LibeventHelper::Stop()
{
    event_base_loopexit(m_pBase, nullptr);
    for (int i = 0; i < s_iSessionCnt; ++i)
    {
        if (NULL != m_bevs[i])
        {
            bufferevent_free(m_bevs[i]);
        }
    }
	if (NULL != m_pBase)
	{
		event_base_free(m_pBase);
	}

	if (NULL != m_bevs)
	{
		free(m_bevs);
	}

	if (NULL != m_pszMsg)
	{
		free(m_pszMsg);
	}

	printf("%d total bytes read\n", m_siLtotal_bytes_read);
	printf("%d total messages read\n", m_siLtotal_messages_read);
	printf("%.3f average messages size read\n", (double)m_siLtotal_bytes_read/m_siLtotal_messages_read);
	printf("%.3f MiB/s throughtput\n", (double)m_siLtotal_bytes_read/(m_timeout.tv_sec * 1024 * 1024));
	
}

