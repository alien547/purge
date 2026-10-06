#ifndef HTTP_H
#define HTTP_H

#include <string>

struct IpLocation{
    std::string country="δ֪", province="δ֪";
};

struct HttpResponse{
    int status_code;
    std::string body;
};

std::string http_get(const std::string& url);
HttpResponse http_post(const std::string& url, const std::string& data, const std::string& content_type="application/x-www-form-urlencoded");
IpLocation query_ip_location();

#endif

