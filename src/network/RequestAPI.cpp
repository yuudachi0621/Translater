#include "RequestAPI.h"
#include "requests.h"
std::string RequestAPI::Request_GET(const std::string& url)
{
    requests::Request req(new HttpRequest);
    req->method  = HTTP_GET;
    req->url     = url;
    req->timeout = 5000;
    requests::Response resp;
    resp = requests::request(req);
    if (resp == NULL || resp->status_code != HTTP_STATUS_OK)
    {
        return std::string();
    }
    std::string response = std::string(resp->body.data(), resp->body.size());
    return response;
}

std::string RequestAPI::Request_POST(const std::string& url, const std::string& body, const std::vector<std::tuple<std::string, std::string>>& headers, int timeoutMs)
{
    http_headers httpHeader;
    for (auto& header : headers)
    {
        httpHeader.insert(std::make_pair(std::get<0>(header), std::get<1>(header)));
    }

    requests::Request req(new HttpRequest);
    req->method  = HTTP_POST;
    req->url     = url;
    req->body    = body;
    req->headers = httpHeader;
    req->timeout = timeoutMs;
    requests::Response resp;
    resp = requests::request(req);
    if (resp == NULL || resp->status_code != HTTP_STATUS_OK)
    {
        return std::string();
    }
    std::string response = std::string(resp->body.data(), resp->body.size());
    return response;
}
