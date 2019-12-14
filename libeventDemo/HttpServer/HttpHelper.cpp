#include "HttpHelper.h"
#include <map>
#include "base64.h"
#include <fstream>
#include <vector>

#define MAX_BUFFER 1024*16


std::map<std::string,std::string>mapContentType
{
    { "txt", "text/plain" },
    { "c", "text/plain" },
    { "h", "text/plain" },
    { "html", "text/html" },
    { "htm", "text/htm" },
    { "css", "text/css" },
    { "gif", "image/gif" },
    { "jpg", "image/jpeg" },
    { "jpeg", "image/jpeg" },
    { "png", "image/png" },
    { "pdf", "application/pdf" },
    { "ps", "application/postscript" },
    { "zip", "application/zip" }
};

std::string get_request_method(struct evhttp_request *req, void *arg)
{
    std::string strMethod;
    switch (evhttp_request_get_command(req)) {
        case EVHTTP_REQ_GET: strMethod = "GET"; break;
        case EVHTTP_REQ_POST: strMethod = "POST"; break;
        case EVHTTP_REQ_HEAD: strMethod = "HEAD"; break;
        case EVHTTP_REQ_PUT: strMethod = "PUT"; break;
        case EVHTTP_REQ_DELETE: strMethod = "DELETE"; break;
        case EVHTTP_REQ_OPTIONS: strMethod = "OPTIONS"; break;
        case EVHTTP_REQ_TRACE: strMethod = "TRACE"; break;
        case EVHTTP_REQ_CONNECT: strMethod = "CONNECT"; break;
        case EVHTTP_REQ_PATCH: strMethod = "PATCH"; break;
        default: break;
    }
    printf("Received a %s request for %s\nHeaders:\n",
        strMethod.c_str(), evhttp_request_get_uri(req));
    return strMethod;
}

void get_request_header(struct evhttp_request *req)
{
    struct evkeyvalq * pHeaders = evhttp_request_get_input_headers(req);
    for (struct evkeyval *pHeader = pHeaders->tqh_first; pHeader;
        pHeader = pHeader->next.tqe_next) {
        printf("  %s: %s\n", pHeader->key, pHeader->value);
    }
}

void load_picture(const std::string& pic_path, std::string& pic_data)
{
    std::string data;
    std::fstream fin;
    fin.open(pic_path.c_str(), std::ios::in | std::ios::binary);
    if (fin.is_open()) {
        fin.seekg(0, std::ios::end);
        std::streampos size = fin.tellg();
        fin.seekg(0, std::ios::beg);
        std::vector<char>buffer(size);
        fin.read(buffer.data(), size);
        fin.close();
        data.assign((char*)buffer.data(), size);
    }
    pic_data = data;
    //pic_data = base64_encode(data);
}

char* get_request_uri(struct evhttp_request *pRequest)
{
    //获取请求uri,端口号之后所有的url
    const char *pUri = evhttp_request_get_uri(pRequest);
    return (char*)pUri;
}

char* get_request_path(struct evhttp_uri* pUri)
{
    //获取uri中的path部分,去掉uri中的参数部分
    const char* pPath = evhttp_uri_get_path(pUri);
    if (nullptr == pPath) {
        pPath = "/";
    }
    else {
        printf("====line:%d,path is:%s\n", __LINE__, pPath);
    }
    return (char*)pPath;
}

void get_request_para(struct evhttp_uri *pDecoded)
{
    if (nullptr == pDecoded) {
        return;
    }
    //获取uri中的参数部分
    char* pQueryPara = (char*)evhttp_uri_get_query(pDecoded);
    if (nullptr == pQueryPara) {
        printf("====line:%d,evhttp_uri_get_query return null\n", __LINE__);
        return;
    }
    char* pQueryData = "data";
    //查询指定参数的值
    struct evkeyvalq *pParams = { 0 };
    evhttp_parse_query_str(pQueryPara, pParams);
    char* pQueryReultData = (char*)evhttp_find_header(pParams, pQueryData);

}
//解析post请求数据
std::string get_post_message(struct evhttp_request *req)
{
    std::string strData;
    size_t post_size = 0;
    post_size = evbuffer_get_length(req->input_buffer);//获取数据长度
    printf("====line:%d,post len:%d\n", __LINE__, post_size);
    if (post_size <= 0) {
        printf("====line:%d,post msg is empty!\n", __LINE__);
        return strData;
    }
    do {
        char szBuffer[MAX_BUFFER] = { 0 };
        evbuffer_remove(req->input_buffer, szBuffer, MAX_BUFFER);
        strData += szBuffer;
    } while (post_size - MAX_BUFFER > 0);

    return strData;
}

//处理get请求
void http_handler_testget_msg(struct evhttp_request *pRequest, void *arg)
{
    //127.0.0.1:20096/testget?sign=test&data=100
    if (nullptr == pRequest) {
        printf("====line:%d,%s\n", __LINE__, "input param req is null.");
        return;
    }

    //回响应
    struct evbuffer *pBuffer = NULL;
    pBuffer = evbuffer_new();
    if (nullptr == pBuffer) {
        printf("====line:%d,%s\n", __LINE__, "retbuff is null.");
        return;
    }
    evbuffer_add_printf(pBuffer, "Receive get request,Thamks for the request!");
    evhttp_send_reply(pRequest, HTTP_OK, "Client", pBuffer);
    evbuffer_free(pBuffer);
}

//处理post请求
void http_handler_testpost_msg(struct evhttp_request *pRequest, void *arg)
{
    if (nullptr == pRequest) {
        printf("====line:%d,%s\n", __LINE__, "input param req is null.");
        return;
    }
    //获取请求数据，一般是json格式的数据
    std::string strPostData = get_post_message(pRequest);
    if (strPostData.empty()) {
        printf("====line:%d,%s\n", __LINE__, "get_post_message return null.");
        return;
    }
    else {
        //可以使用json库解析需要的数据
        printf("====line:%d,request data:%s", __LINE__, strPostData.c_str());
    }

    //回响应
    struct evbuffer *pBuffer = nullptr;
    pBuffer = evbuffer_new();
    if (nullptr == pBuffer) {
        printf("====line:%d,%s\n", __LINE__, "retbuff is null.");
        return;
    }
    evbuffer_add_printf(pBuffer, "Receive post request,Thamks for the request!");
    evhttp_send_reply(pRequest, HTTP_OK, "Client", pBuffer);
    evbuffer_free(pBuffer);
}

//处理jpg
void http_handler_photo_msg(struct evhttp_request *pRequest, void *arg)
{
    if (nullptr == pRequest) {
        printf("====line:%d,%s\n", __LINE__, "input param req is null.");
        return;
    }
    //回响应
    struct evbuffer *retbuff = evbuffer_new();
    if (nullptr == retbuff) {
        printf("====line:%d,%s\n", __LINE__, "retbuff is null.");
        return;
    }
    const char *pUri = evhttp_request_get_uri(pRequest);//获取请求uri
    if (nullptr == pUri) {
        evbuffer_add_printf(retbuff, "url is null");
    }
    else {
        printf("====line:%d,Got a GET request for <%s>\n", __LINE__, pUri);
        std::string strPicPath = "test.jpg";
        std::string strPicData;
        load_picture(strPicPath, strPicData);
        if (strPicData.empty()) {
            evbuffer_add_printf(retbuff, "Invalid file data");
        }
        else {
            evhttp_add_header(evhttp_request_get_output_headers(pRequest),
                "Content-Type", "image/jpeg");
            evbuffer_add(retbuff, strPicData.c_str(), strPicData.length());
        }
    }
    evhttp_send_reply(pRequest, 200, "OK", retbuff);
    evbuffer_free(retbuff);
}

//处理未设定的请求
void http_handler_all(struct evhttp_request *pRequest, void *arg)
{
    //http://127.0.0.1:20096/photo/20191022/test.jpg
    if (nullptr == pRequest) {
        printf("====line:%d,%s\n", __LINE__, "input param req is null.");
        return;
    }
    //回响应
    struct evbuffer *pBuffer = evbuffer_new();
    if (nullptr == pBuffer) {
        printf("====line:%d,%s\n", __LINE__, "retbuff is null.");
        return;
    }
    const char *pUri = get_request_uri(pRequest);
    if (nullptr == pUri) {
        evbuffer_add_printf(pBuffer, "url is null");
    }
    else {
        printf("====line:%d,Got a GET request for <%s>\n", __LINE__, pUri);

        struct evhttp_uri* pDecoded = evhttp_uri_parse(pUri);
        if (nullptr == pDecoded) {
            printf("====line:%d,It's not a good URI. Sending BADREQUEST\n", __LINE__);
            evhttp_send_error(pRequest, HTTP_BADREQUEST, 0);
            return;
        }
        //获取请求uri,不包括参数
        char* pPath = get_request_path(pDecoded);
        //获取请求参数
        get_request_para(pDecoded);

        std::string strPicPath = "test.jpg";
        std::string strPicData;
        load_picture(strPicPath, strPicData);
        if (strPicData.empty()) {
            evbuffer_add_printf(pBuffer, "Invalid file data");
        }
        else {
            evhttp_add_header(evhttp_request_get_output_headers(pRequest),
                "Content-Type", "image/jpeg");
            evbuffer_add(pBuffer, strPicData.c_str(), strPicData.length());
        }
    }

    evhttp_send_reply(pRequest, 200, "OK", pBuffer);
    evbuffer_free(pBuffer);
}

HttpHelper::HttpHelper()
    : m_pHttpServer(nullptr)
{
}


HttpHelper::~HttpHelper()
{
}

bool HttpHelper::Init()
{
#ifdef _WIN32
    WSADATA wsaData;
    DWORD Ret;
    if ((Ret = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0) {
        printf("WSAStartup failed with error %d\n", Ret);
        exit(1);
    }
#endif // Win32
    short http_port = 20096;
    char* http_addr = "0.0.0.0";
    //初始化
    event_init();
    //启动http服务端
    m_pHttpServer = evhttp_start(http_addr, http_port);
    if (m_pHttpServer == NULL)
    {
        printf("====line:%d,%s\n", __LINE__, "http server start failed.");
        return false;
    }
    //设置请求超时时间(s)
    evhttp_set_timeout(m_pHttpServer, 5);
    //设置事件处理函数，evhttp_set_cb针对每一个事件(请求)注册一个处理函数，
    //区别于evhttp_set_gencb函数，是对所有请求设置一个统一的处理函数
    evhttp_set_cb(m_pHttpServer, "/testpost", http_handler_testpost_msg, NULL);
    evhttp_set_cb(m_pHttpServer, "/testget", http_handler_testget_msg, NULL);
    evhttp_set_cb(m_pHttpServer, "/photo", http_handler_photo_msg, NULL);

    //Set a callback for all requests that are not caught by specific callbacks
    evhttp_set_gencb(m_pHttpServer, http_handler_all, NULL);

    return true;
}

void HttpHelper::Start()
{
    event_dispatch();
}

void HttpHelper::Stop()
{
    evhttp_free(m_pHttpServer);
}
