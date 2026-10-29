#include "BaiDuEngine.h"
#include "EncryptMD5.h"
#include "RequestAPI.h"
#include "json.hpp"
#include "URLEncode.h"
#include "File.h"
#include "ThreadPool.h"
#include "RateLimiter.h"

#include <random>
#include <iostream>
#include <tuple>
#include <atomic>
#include <chrono>
#include <thread>
#include <mutex>

BaiDuEngine::BaiDuEngine()
{
    LoadDataFile();
}

std::string BaiDuEngine::TranslateText(const std::string& text)
{
    std::vector<std::tuple<std::string, std::string>> transResults = ToRequest(GetRequest(text));
    if (transResults.empty())
        return std::string();

    std::tuple<std::string, std::string> transResult = transResults[0];
    std::string resutlt                              = std::get<1>(transResult);
    return resutlt;
}

std::vector<std::string> BaiDuEngine::TranslateBatch(const std::vector<std::string>& texts)
{
    std::vector<std::string> result(texts.size());

    // 速率限制器 - 每秒m_qpsMax个请求
    RateLimiter limiter(m_qpsMax);
    ThreadPool pool(4);
    std::mutex resultMutex;
    std::vector<std::future<void>> futures;

    std::vector<TextBlock> reqTexts = GetTextGroup(texts);

    for (auto& req : reqTexts)
    {
        auto func = [&]() {
            limiter.wait(); // 等待获取请求许可

            std::vector<std::tuple<std::string, std::string>> res = ToRequest(GetRequest(req.lines));
            if (res.empty()) return;

            std::lock_guard<std::mutex> lock(resultMutex);
            // Some providers may return extra rows for multiline input.
            const std::size_t copyCount = res.size() < req.lines.size() ? res.size() : req.lines.size();
            for (std::size_t index = 0; index < copyCount; ++index)
            {
                const std::size_t resultIndex = static_cast<std::size_t>(req.start_idx) + index;
                if (resultIndex < result.size())
                    result[resultIndex] = std::get<1>(res[index]);
            }
        };
        futures.push_back(pool.enqueue(func));
    }

    for (auto& future : futures)
    {
        future.get();
    }
    return result;
}

void BaiDuEngine::LoadDataFile()
{
    std::string config       = File::ReadJsonConfig("config.txt");
    nlohmann::json json_obj  = nlohmann::json::parse(config);
    nlohmann::json baidu_obj = json_obj["engine"]["BaiDu"];

    m_from = json_obj["from"];
    m_to   = json_obj["to"];

    if (baidu_obj.find("requestURL") != baidu_obj.end())
        m_requestURL = baidu_obj["requestURL"];
    else
    {
        // Compatibility with older config files.
        m_requestURL = baidu_obj["requstURL"];
    }
    m_apiKey = baidu_obj["apiKey"];
    m_appId  = baidu_obj["appId"];

    m_qpsMax    = ParsePositiveInt(baidu_obj["qps"], 1);
    m_lengthMax = ParsePositiveInt(baidu_obj["length"], 100);
}

std::vector<TextBlock> BaiDuEngine::GetTextGroup(const std::vector<std::string>& fileText)
{
    std::vector<TextBlock> result;

    int reqLen      = 0;
    int block_start = 0;
    std::vector<std::string> reqText;
    for (int i = 0; i < fileText.size(); i++)
    {
        if (reqLen + fileText[i].size() >= m_lengthMax)
        {
            if (!reqText.empty())
                result.push_back({block_start, reqText});

            reqText.clear();
            reqText.push_back(fileText[i]);
            reqLen      = fileText[i].size();
            block_start = i;
        }
        else
        {
            reqText.push_back(fileText[i]);
            reqLen += fileText[i].size();
        }
    }
    if (!reqText.empty())
    {
        result.push_back({block_start, reqText});
    }
    return result;
}

std::string BaiDuEngine::GetRequest(const std::string& str)
{
    std::random_device rd;                          // 用于获取随机数种子
    std::mt19937 gen(rd());                         // 使用随机数种子初始化Mersenne Twister生成器
    std::uniform_int_distribution<> dis(1, 100000); // 定义一个在1到100之间均匀分布的整数生成器
    int random_number = dis(gen);                   // 生成随机数

    std::string salt = std::to_string(random_number); // 随机数
    std::string sign = EncryptString(m_appId + str + salt + m_apiKey);
    std::string url  = m_requestURL;
    url += "?q=" + URLEncode::encode(str);
    url += "&from=" + m_from;
    url += "&to=" + m_to;
    url += "&appid=" + m_appId;
    url += "&salt=" + salt;
    url += "&sign=" + sign;
    return url;
}

std::string BaiDuEngine::GetRequest(const std::vector<std::string>& texts)
{
    std::string requestStr;
    for (auto& text : texts)
    {
        requestStr += text + u8"\n";
    }
    if (requestStr.empty())
        return std::string();
    return GetRequest(requestStr.substr(0, requestStr.size() - 1));
}

std::vector<std::tuple<std::string, std::string>> BaiDuEngine::ToRequest(const std::string& reqStr)
{
    std::string response = RequestAPI::Request_GET(reqStr);
    return GetResult(response);
}

std::vector<std::tuple<std::string, std::string>> BaiDuEngine::GetResult(const std::string& jsonStr)
{
    if (jsonStr.empty()) return {};

    std::vector<std::tuple<std::string, std::string>> result;

    nlohmann::json json_obj     = nlohmann::json::parse(jsonStr);
    nlohmann::json trans_result = json_obj["trans_result"];
    for (auto& trans : trans_result.items())
    {
        nlohmann::json value = trans.value();
        std::string src      = value["src"];
        std::string dst      = value["dst"];

        result.push_back(std::make_tuple(src, dst));
    }
    return result;
}

std::string BaiDuEngine::EncryptString(const std::string& str)
{
    EncryptMD5 md5;
    return md5.compute(str);
}
