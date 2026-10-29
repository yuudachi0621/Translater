#include "OpenAICompatibleEngine.h"
#include "File.h"
#include "RequestAPI.h"
#include "RateLimiter.h"
#include "json.hpp"

#include <cctype>
#include <cstdlib>

namespace {
std::string Trim(const std::string& text)
{
    std::size_t begin = 0;
    while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin])))
        ++begin;

    std::size_t end = text.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])))
        --end;

    return text.substr(begin, end - begin);
}

int ReadPositiveInt(const nlohmann::json& object, const char* key, int defaultValue)
{
    if (object.find(key) == object.end() || object[key].is_null())
        return defaultValue;

    int value = defaultValue;
    try
    {
        value = std::atoi(object[key].get<std::string>().c_str());
    } catch (const std::exception&)
    {
        return defaultValue;
    }
    return value > 0 ? value : defaultValue;
}

std::string ExtractMessageContent(const nlohmann::json& response)
{
    if (!response.is_object() || response.find("choices") == response.end() || !response["choices"].is_array() || response["choices"].empty())
        return std::string();

    const nlohmann::json& choice = response["choices"][0];
    if (!choice.is_object() || choice.find("message") == choice.end() || !choice["message"].is_object())
        return std::string();

    const nlohmann::json& message = choice["message"];
    if (message.find("content") == message.end())
        return std::string();

    const nlohmann::json& content = message["content"];
    if (content.is_string())
        return Trim(content.get<std::string>());

    if (!content.is_array())
        return std::string();

    std::string result;
    for (const auto& part : content)
    {
        if (part.is_string())
        {
            result += part.get<std::string>();
            continue;
        }

        if (!part.is_object())
            continue;

        if (part.find("text") != part.end() && part["text"].is_string())
            result += part["text"].get<std::string>();
        else if (part.find("content") != part.end() && part["content"].is_string())
            result += part["content"].get<std::string>();
    }
    return Trim(result);
}

nlohmann::json ParseJsonContent(const std::string& content)
{
    std::string text = Trim(content);
    if (text.compare(0, 3, "```") == 0)
    {
        const std::size_t firstLineEnd = text.find('\n');
        if (firstLineEnd != std::string::npos)
            text.erase(0, firstLineEnd + 1);

        const std::size_t fence = text.rfind("```");
        if (fence != std::string::npos)
            text.erase(fence);
        text = Trim(text);
    }

    nlohmann::json output = nlohmann::json::parse(text, nullptr, false);
    if (!output.is_discarded())
        return output;

    const std::size_t begin = text.find_first_of("[{");
    const std::size_t end   = text.find_last_of("]}");
    if (begin != std::string::npos && end != std::string::npos && end > begin)
        return nlohmann::json::parse(text.substr(begin, end - begin + 1), nullptr, false);

    return nlohmann::json::parse("", nullptr, false);
}
} // namespace

OpenAICompatibleEngine::OpenAICompatibleEngine()
{
    LoadDataFile();
}

std::string OpenAICompatibleEngine::TranslateText(const std::string& text)
{
    if (text.empty())
        return std::string();

    std::vector<std::tuple<std::string, std::string>> transResults = ToRequest(GetRequest(text));
    if (transResults.empty())
        return std::string();

    return std::get<1>(transResults[0]);
}

std::vector<std::string> OpenAICompatibleEngine::TranslateBatch(const std::vector<std::string>& texts)
{
    std::vector<std::string> result(texts.size());
    if (texts.empty())
        return result;

    RateLimiter limiter(m_qpsMax);
    std::vector<TextBlock> reqTexts = GetTextGroup(texts);

    for (std::size_t blockIndex = 0; blockIndex < reqTexts.size(); ++blockIndex)
    {
        const TextBlock& block = reqTexts[blockIndex];
        limiter.wait();

        std::vector<std::string> translations = TranslateGroup(block.lines);
        if (translations.size() == block.lines.size())
        {
            for (std::size_t i = 0; i < translations.size(); ++i)
                result[block.start_idx + i] = translations[i];
            continue;
        }

        // Fall back to independent calls when a provider does not follow the
        // requested JSON batch format.
        for (std::size_t i = 0; i < block.lines.size(); ++i)
        {
            if (!block.lines[i].empty())
                limiter.wait();
            result[block.start_idx + i] = TranslateText(block.lines[i]);
        }
    }

    return result;
}

void OpenAICompatibleEngine::LoadDataFile()
{
    m_from            = "en";
    m_to              = "zh";
    m_requestURL      = "https://api.openai.com/v1/chat/completions";
    m_model           = "gpt-4.1-mini";
    m_apiKey          = "";
    m_reasoningEffort = "";
    m_qpsMax          = 1;
    m_lengthMax       = 2000;
    m_reqQuota        = 20;
    m_timeoutMs       = 120000;
    m_maxTokens       = 1024;
    m_temperature     = 0.1;

    std::string config = File::ReadJsonConfig("config.txt");
    if (config.empty())
        return;

    nlohmann::json json_obj = nlohmann::json::parse(config, nullptr, false);
    if (json_obj.is_discarded() || json_obj.find("engine") == json_obj.end() || !json_obj["engine"].is_object())
        return;

    const nlohmann::json& engines = json_obj["engine"];
    if (engines.find("OpenAICompatible") != engines.end())
    {
        const nlohmann::json& engine_obj = engines["OpenAICompatible"];
        m_requestURL                     = engine_obj.value("requestURL", m_requestURL);
        m_model                          = engine_obj.value("model", m_model);
        m_apiKey                         = engine_obj.value("apiKey", m_apiKey);
        m_qpsMax                         = ReadPositiveInt(engine_obj, "qps", m_qpsMax);
        m_lengthMax                      = ReadPositiveInt(engine_obj, "length", m_lengthMax);
        m_reqQuota                       = ReadPositiveInt(engine_obj, "quota", m_reqQuota);
        m_timeoutMs                      = ReadPositiveInt(engine_obj, "timeout", m_timeoutMs);
        m_maxTokens                      = ReadPositiveInt(engine_obj, "maxTokens", m_maxTokens);

        if (engine_obj.find("temperature") != engine_obj.end() && engine_obj["temperature"].is_number())
            m_temperature = engine_obj["temperature"].get<double>();
        if (engine_obj.find("reasoningEffort") != engine_obj.end() && engine_obj["reasoningEffort"].is_string())
            m_reasoningEffort = engine_obj["reasoningEffort"].get<std::string>();
    }
    else if (engines.find("CustomAPI") != engines.end())
    {
        // Compatibility alias for early custom API configurations.
        const nlohmann::json& engine_obj = engines["CustomAPI"];
        m_requestURL                     = engine_obj.value("requestURL", m_requestURL);
        m_model                          = engine_obj.value("model", m_model);
        m_apiKey                         = engine_obj.value("apiKey", m_apiKey);
        m_qpsMax                         = ReadPositiveInt(engine_obj, "qps", m_qpsMax);
        m_lengthMax                      = ReadPositiveInt(engine_obj, "length", m_lengthMax);
        m_reqQuota                       = ReadPositiveInt(engine_obj, "quota", m_reqQuota);
        m_timeoutMs                      = ReadPositiveInt(engine_obj, "timeout", m_timeoutMs);
        m_maxTokens                      = ReadPositiveInt(engine_obj, "maxTokens", m_maxTokens);

        if (engine_obj.find("temperature") != engine_obj.end() && engine_obj["temperature"].is_number())
            m_temperature = engine_obj["temperature"].get<double>();
        if (engine_obj.find("reasoningEffort") != engine_obj.end() && engine_obj["reasoningEffort"].is_string())
            m_reasoningEffort = engine_obj["reasoningEffort"].get<std::string>();
    }

    m_from = json_obj.value("from", m_from);
    m_to   = json_obj.value("to", m_to);
}

std::vector<TextBlock> OpenAICompatibleEngine::GetTextGroup(const std::vector<std::string>& fileText)
{
    std::vector<TextBlock> result;

    std::size_t reqLen = 0;
    int blockStart     = 0;
    std::vector<std::string> reqText;

    for (std::size_t i = 0; i < fileText.size(); ++i)
    {
        const std::size_t textLen = fileText[i].size();
        const bool overQuota      = static_cast<int>(reqText.size()) >= m_reqQuota;
        const bool overLength     = !reqText.empty() && reqLen + textLen > static_cast<std::size_t>(m_lengthMax);

        if (overQuota || overLength)
        {
            result.push_back({blockStart, reqText});
            reqText.clear();
            reqLen     = 0;
            blockStart = static_cast<int>(i);
        }

        reqText.push_back(fileText[i]);
        reqLen += textLen;
    }

    if (!reqText.empty())
        result.push_back({blockStart, reqText});

    return result;
}

std::string OpenAICompatibleEngine::GetRequest(const std::string& str)
{
    const std::string systemPrompt =
        "You are a professional translation engine. Translate the text from \"" + m_from + "\" to \"" + m_to + "\". Return only the translated text. Do not add explanations, labels, quotes, or Markdown.";

    return BuildRequestBody(systemPrompt, "Text to translate:\n" + str);
}

std::string OpenAICompatibleEngine::GetRequest(const std::vector<std::string>& texts)
{
    nlohmann::json input;
    input["texts"] = texts;

    const std::string systemPrompt =
        "You are a professional translation engine. Translate every string in the input JSON array from \"" + m_from + "\" to \"" + m_to + "\". Return only a JSON object with this exact shape: "
                                                                                                                                           "{\"translations\":[\"translation 1\",\"translation 2\"]}. "
                                                                                                                                           "Keep the same number of items and their order. Do not add explanations, labels, or Markdown.";

    return BuildRequestBody(systemPrompt, "Translate this input JSON:\n" + input.dump());
}

std::vector<std::tuple<std::string, std::string>> OpenAICompatibleEngine::ToRequest(const std::string& reqStr)
{
    return GetResult(Request(reqStr));
}

std::vector<std::tuple<std::string, std::string>> OpenAICompatibleEngine::GetResult(const std::string& jsonStr)
{
    std::vector<std::tuple<std::string, std::string>> result;
    if (jsonStr.empty())
        return result;

    nlohmann::json json_obj = nlohmann::json::parse(jsonStr, nullptr, false);
    if (json_obj.is_discarded())
        return result;

    const std::string translated = ExtractMessageContent(json_obj);
    if (!translated.empty())
        result.push_back(std::make_tuple(std::string(), translated));

    return result;
}

std::string OpenAICompatibleEngine::BuildRequestBody(const std::string& systemPrompt,
                                                     const std::string& userPrompt) const
{
    nlohmann::json messages = nlohmann::json::array();
    messages.push_back({{"role", "system"}, {"content", systemPrompt}});
    messages.push_back({{"role", "user"}, {"content", userPrompt}});

    nlohmann::json body;
    body["model"]       = m_model;
    body["messages"]    = messages;
    body["temperature"] = m_temperature;
    body["max_tokens"]  = m_maxTokens;
    body["stream"]      = false;

    if (!m_reasoningEffort.empty())
        body["reasoning_effort"] = m_reasoningEffort;

    return body.dump();
}

std::string OpenAICompatibleEngine::Request(const std::string& reqStr)
{
    return RequestAPI::Request_POST(m_requestURL, reqStr, GetPostHeader(), m_timeoutMs);
}

std::vector<std::string> OpenAICompatibleEngine::TranslateGroup(const std::vector<std::string>& texts)
{
    if (texts.empty())
        return {};

    return GetBatchResult(Request(GetRequest(texts)));
}

std::vector<std::string> OpenAICompatibleEngine::GetBatchResult(const std::string& jsonStr)
{
    std::vector<std::string> result;
    if (jsonStr.empty())
        return result;

    nlohmann::json json_obj = nlohmann::json::parse(jsonStr, nullptr, false);
    if (json_obj.is_discarded())
        return result;

    const std::string content = ExtractMessageContent(json_obj);
    if (content.empty())
        return result;

    nlohmann::json output = ParseJsonContent(content);
    if (output.is_discarded())
        return result;

    nlohmann::json translations;
    if (output.is_array())
        translations = output;
    else if (output.is_object() && output.find("translations") != output.end())
        translations = output["translations"];

    if (!translations.is_array())
        return result;

    for (const auto& translation : translations)
    {
        if (!translation.is_string())
            return {};
        result.push_back(Trim(translation.get<std::string>()));
    }

    return result;
}

std::vector<std::tuple<std::string, std::string>> OpenAICompatibleEngine::GetPostHeader() const
{
    std::vector<std::tuple<std::string, std::string>> headers;
    headers.push_back(std::make_tuple("Content-Type", "application/json"));
    if (!m_apiKey.empty())
        headers.push_back(std::make_tuple("Authorization", "Bearer " + m_apiKey));
    return headers;
}
