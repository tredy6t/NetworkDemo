#pragma once
#include <string>
#include <map>
#include <set>

struct DeviceInfo
{
    std::string uri;
    std::string ip;
    std::string name;
    std::string location;
    std::string hardware;

    bool operator< (const DeviceInfo& that) const
    {
        return ip < that.ip;
    }
};

class OnvifHelper
{
public:
    
    int DiscoveryDevice();
    int GetCameraRtsp(const std::string& ip, const std::string& user, const std::string& password, std::string& rtsp);
    int GetLocalIps(std::map<std::string, std::set<std::string> >& ips);
private:
    struct soap* init_soap(struct SOAP_ENV__Header *header, const char *was_To,
        const char *was_Action, int timeout);
    void get_profiles(struct soap *soap, struct _tds__GetCapabilitiesResponse *capa_resp);
    void get_uri(struct soap *soap, struct _trt__GetProfilesResponse *trt__GetProfilesResponse,
        struct _tds__GetCapabilitiesResponse *capa_resp);
    int get_capabilities(struct __wsdd__ProbeMatches *resp);
    void split_str(const std::string& prefix, std::string& str, std::map<std::string, std::string>& dev_info);
    std::string get_ip_from_str(const std::string& src_str);

private:
    std::string m_strLocalIP;
    int m_nTimeOut;
    std::string m_strUsername;
    std::string m_strPassword;

};
