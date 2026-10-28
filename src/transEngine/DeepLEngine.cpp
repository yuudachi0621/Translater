#include "DeepLEngine.h"
#include "File.h"
#include "json.hpp"
#include "RequestAPI.h"
#include "ThreadPool.h"
#include "RateLimiter.h"

DeepLEngine::DeepLEngine()
{
    LoadDataFile();
}

std::string DeepLEngine::TranslateText(const std::string& text)
{
    std::vector<std::tuple<std::string, std::string>> transResults = ToRequest(GetRequest(text));
    if (transResults.empty())
        return std::string();

    std::tuple<std::string, std::string> transResult = transResults[0];
    std::string resutlt                              = std::get<1>(transResult);
    return resutlt;
}

std::vector<std::string> DeepLEngine::TranslateBatch(const std::vector<std::string>& texts)
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
            for (int index = 0; index < res.size(); index++)
            {
                result[req.start_idx + index] = std::get<1>(res[index]);
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

void DeepLEngine::LoadDataFile()
{
    std::string config       = File::ReadJsonConfig("config.txt");
    nlohmann::json json_obj  = nlohmann::json::parse(config);
    nlohmann::json deepl_obj = json_obj["engine"]["DeepL"];

    m_from = json_obj["from"];
    m_to   = json_obj["to"];

    if (deepl_obj.find("requestURL") != deepl_obj.end())
        m_requestURL = deepl_obj["requestURL"];
    else
    {
        // Compatibility with older config files.
        m_requestURL = deepl_obj["requstURL"];
    }
    m_apiKey = deepl_obj["apiKey"];

    std::string qps = deepl_obj["qps"];
    if (qps.empty())
        m_qpsMax = 1;
    else
        m_qpsMax = std::atoi(qps.data());

    std::string length = deepl_obj["length"];
    if (length.empty())
        m_lengthMax = 100;
    else
        m_lengthMax = std::atoi(length.data());

    std::string quota = deepl_obj["quota"];
    if (quota.empty())
        m_reqQuota = 10;
    else
        m_reqQuota = std::atoi(quota.data());
}

std::vector<TextBlock> DeepLEngine::GetTextGroup(const std::vector<std::string>& fileText)
{
    std::vector<TextBlock> result;

    int reqLen      = 0;
    int block_start = 0;
    std::vector<std::string> reqText;
    for (int i = 0; i < fileText.size(); i++)
    {
        if (reqText.size() >= m_reqQuota || reqLen + fileText[i].size() >= m_lengthMax)
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

std::string DeepLEngine::GetRequest(const std::string& str)
{
    nlohmann::json json_obj;
    json_obj["source_lang"] = m_from;
    json_obj["target_lang"] = m_to;
    json_obj["text"].push_back(str);
    return json_obj.dump();
}

std::string DeepLEngine::GetRequest(const std::vector<std::string>& texts)
{
    nlohmann::json json_obj;
    json_obj["source_lang"] = m_from;
    json_obj["target_lang"] = m_to;
    for (auto& text : texts)
    {
        json_obj["text"].push_back(text);
    }
    return json_obj.dump();
}

std::vector<std::tuple<std::string, std::string>> DeepLEngine::ToRequest(const std::string& reqStr)
{
    std::string response = RequestAPI::Request_POST(m_requestURL, reqStr, GetPostHeader());
    return GetResult(response);
}

std::vector<std::tuple<std::string, std::string>> DeepLEngine::GetResult(const std::string& jsonStr)
{
    if (jsonStr.empty()) return {};

    std::vector<std::tuple<std::string, std::string>> result;

    nlohmann::json json_obj     = nlohmann::json::parse(jsonStr);
    nlohmann::json trans_result = json_obj["translations"];
    for (auto& trans : trans_result.items())
    {
        nlohmann::json value = trans.value();
        std::string src      = "";
        std::string dst      = value["text"];

        result.push_back(std::make_tuple(src, dst));
    }
    return result;
}

std::vector<std::tuple<std::string, std::string>> DeepLEngine::GetPostHeader()
{
    std::vector<std::tuple<std::string, std::string>> headers;
    headers.push_back(std::make_tuple("Content-Type", "application/json"));
    headers.push_back(std::make_tuple("Authorization", "DeepL-Auth-Key " + m_apiKey));
    return headers;
}
