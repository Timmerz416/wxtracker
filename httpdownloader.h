#ifndef __HTTPDOWNLOADER_H__
#define __HTTPDOWNLOADER_H__

#include <string>
#include <curl/curl.h>

class CurlError { };

class HTTPDownloader {
private:
    CURL *curl_handle;

public:
    HTTPDownloader();
    ~HTTPDownloader();

    std::string download(const std::string url);
};

#endif // __HTTPDOWNLOADER_H__
