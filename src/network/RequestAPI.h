#pragma once
#include <string>
#include <tuple>
#include <vector>
class RequestAPI
{
public:
    static std::string Request_GET(const std::string& url);

    static std::string Request_POST(const std::string& url, const std::string& body, const std::vector<std::tuple<std::string, std::string>>& headers, int timeoutMs = 5000);
};
