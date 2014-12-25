#include "httpdownloader.h"
#include <curl/easy.h>
#include <curl/curlbuild.h>
#include <sstream>
#include <iostream>

using namespace std;

// Redirect the received data and write to a stringstream
size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream) {
    string data((const char*) ptr, (size_t) size*nmemb);
    *((stringstream*) stream) << data;
    return size*nmemb;
}

HTTPDownloader::HTTPDownloader() {
    // Initialize the curl handle
    curl_handle = curl_easy_init();
}

HTTPDownloader::~HTTPDownloader() {
    // Clear off the handle
    curl_easy_cleanup(curl_handle);
}

string HTTPDownloader::download(const string url) {
    // Create hte string stream
    stringstream output;

    // Check the handle
    if(curl_handle) {
        // Set options
        curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());    // Set the URL
        curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);  // Follow redirection
        curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);         // Prevent "longjmp causes uninitialized stack frame" bug
        curl_easy_setopt(curl_handle, CURLOPT_ACCEPT_ENCODING, "deflate");
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);   // Set the callback for writing
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &output);          // Redirect to the string stream

        // Perform the request, result will get the return code
        CURLcode result = curl_easy_perform(curl_handle);

        // Check for errors
        if (result != CURLE_OK) cerr << "curl_easy_perform() failed: " << string(curl_easy_strerror(result)) << "\n";
    } else throw CurlError();   // Throw an exception that there is no handle for curl

    // Return the string
    return output.str();
}
