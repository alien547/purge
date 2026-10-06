#include <mutex>
#include <curl/curl.h>
#include <json.hpp>
#include "Http.h"
using namespace std;
using json=nlohmann::json;

once_flag curl_init_flag;

static size_t write_to_string(void* ptr, size_t size, size_t nmemb, void* userp){
    size_t total=size*nmemb;
    ((string*)(userp))->append((char*)(ptr), total);
    return total;
}

string http_get(const string& url){
    call_once(curl_init_flag, curl_global_init, CURL_GLOBAL_DEFAULT);

    string response;
    CURL* curl=curl_easy_init();

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_string);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_perform(curl);
    long http_code=0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);
    if(http_code!=200)return "";
    return response;
}

HttpResponse http_post(const string& url, const string& data, const string& content_type){
    call_once(curl_init_flag, curl_global_init, CURL_GLOBAL_DEFAULT);

    HttpResponse result{0, ""};
    CURL* curl=curl_easy_init();
    struct curl_slist* headers=NULL;
    headers=curl_slist_append(headers, ("Content-Type: "+content_type).c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_string);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result.body);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.size());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &result.status_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return result;
}

IpLocation query_ip_location(){
    IpLocation loc;
    string url="https://ipinfo.io/json";
    string response=http_get(url);
    if(response.empty())return loc;
    try{
        json j=json::parse(response);
        if(j.contains("country")&&!j["country"].is_null())loc.country=j["country"].get<string>();
        if(j.contains("region")&&!j["region"].is_null())loc.province=j["region"].get<string>();
    }catch(const exception&){}
    return loc;
}

