#include "OllamaEngine.h"
#include "File.h"
#include "RequestAPI.h"
#include "RateLimiter.h"
#include "json.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>

namespace
{
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
    }
    catch (const std::exception&)
    {
        return defaultValue;
    }
    return value > 0 ? value : defaultValue;
}
} // namespace

OllamaEngine::OllamaEngine()
{
    LoadDataFile();
}

std::string OllamaEngine::TranslateText(const std::string& text)
{
    if (text.empty())
        return std::string();

    std::vector<std::tuple<std::string, std::string>> transResults = ToRequest(GetRequest(text));
    if (transResults.empty())
        return std::string();

    return std::get<1>(transResults[0]);
}

std::vector<std::string> OllamaEngine::TranslateBatch(const std::vector<std::string>& texts)
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

        // Some local models do not reliably follow JSON batch output. Fall
        // back to independent requests so every line still has a mapping.
        for (std::size_t i = 0; i < block.lines.size(); ++i)
        {
            if (!block.lines[i].empty())
                limiter.wait();
            result[block.start_idx + i] = TranslateText(block.lines[i]);
        }
    }

    return result;
}

void OllamaEngine::LoadDataFile()
{
    m_from       = "en";
    m_to         = "zh";
    m_requestURL  = "http://127.0.0.1:11434/api/generate";
    m_model      = "qwen3.5:2b";
    m_qpsMax     = 1;
    m_lengthMax  = 2000;
    m_reqQuota   = 20;
    m_timeoutMs  = 120000;
    m_numPredict = 512;
    m_numCtx     = 4096;
    m_temperature = 0.1;
    m_think       = false;

    std::string config = File::ReadJsonConfig("config.txt");
    if (config.empty())
        return;

    nlohmann::json json_obj = nlohmann::json::parse(config, nullptr, false);
    if (json_obj.is_discarded() || json_obj.find("engine") == json_obj.end() ||
        json_obj["engine"].find("Ollama") == json_obj["engine"].end())
        return;

    nlohmann::json ollama_obj = json_obj["engine"]["Ollama"];

    m_from = json_obj.value("from", std::string("en"));
    m_to   = json_obj.value("to", std::string("zh"));

    if (ollama_obj.find("requestURL") != ollama_obj.end())
        m_requestURL = ollama_obj["requestURL"];
    else if (ollama_obj.find("requstURL") != ollama_obj.end())
    {
        // Compatibility with older config files.
        m_requestURL = ollama_obj["requstURL"];
    }
    m_model     = ollama_obj.value("model", std::string("qwen3.5:2b"));

    m_qpsMax     = ReadPositiveInt(ollama_obj, "qps", 1);
    m_lengthMax  = ReadPositiveInt(ollama_obj, "length", 2000);
    m_reqQuota   = ReadPositiveInt(ollama_obj, "quota", 20);
    m_timeoutMs  = ReadPositiveInt(ollama_obj, "timeout", 120000);
    m_numPredict = ReadPositiveInt(ollama_obj, "numPredict", 512);
    m_numCtx     = ReadPositiveInt(ollama_obj, "numCtx", 4096);

    m_temperature = 0.1;
    if (ollama_obj.find("temperature") != ollama_obj.end() && ollama_obj["temperature"].is_number())
        m_temperature = ollama_obj["temperature"].get<double>();

    m_think = false;
    if (ollama_obj.find("think") != ollama_obj.end() && ollama_obj["think"].is_boolean())
        m_think = ollama_obj["think"].get<bool>();
}

std::vector<TextBlock> OllamaEngine::GetTextGroup(const std::vector<std::string>& fileText)
{
    std::vector<TextBlock> result;

    std::size_t reqLen = 0;
    int blockStart     = 0;
    std::vector<std::string> reqText;

    for (std::size_t i = 0; i < fileText.size(); ++i)
    {
        const std::size_t textLen = fileText[i].size();
        const bool overQuota       = static_cast<int>(reqText.size()) >= m_reqQuota;
        const bool overLength      = !reqText.empty() && reqLen + textLen > static_cast<std::size_t>(m_lengthMax);

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

std::string OllamaEngine::GetRequest(const std::string& str)
{
    const std::string prompt =
        "You are a translation engine. Translate the text from \"" + m_from + "\" to \"" + m_to +
        "\". Return only the translated text. Do not add explanations, quotes, labels, or Markdown."
        "\n\nText:\n" + str;

    return BuildRequestBody(prompt, false);
}

std::string OllamaEngine::GetRequest(const std::vector<std::string>& texts)
{
    nlohmann::json input;
    input["texts"] = texts;

    const std::string prompt =
        "You are a translation engine. Translate every string in the input JSON array from \"" +
        m_from + "\" to \"" + m_to +
        "\". Return only a JSON object with this exact shape: "
        "{\"translations\":[\"translation 1\",\"translation 2\"]}. "
        "Keep the same number of items and their order. Do not add explanations, labels, or Markdown."
        "\n\nInput:\n" + input.dump();

    return BuildRequestBody(prompt, true);
}

std::vector<std::tuple<std::string, std::string>> OllamaEngine::ToRequest(const std::string& reqStr)
{
    return GetResult(Request(reqStr));
}

std::vector<std::tuple<std::string, std::string>> OllamaEngine::GetResult(const std::string& jsonStr)
{
    std::vector<std::tuple<std::string, std::string>> result;
    if (jsonStr.empty())
        return result;

    nlohmann::json json_obj = nlohmann::json::parse(jsonStr, nullptr, false);
    if (json_obj.is_discarded() || json_obj.find("response") == json_obj.end() ||
        !json_obj["response"].is_string())
        return result;

    const std::string translated = Trim(json_obj["response"].get<std::string>());
    if (!translated.empty())
        result.push_back(std::make_tuple(std::string(), translated));

    return result;
}

std::string OllamaEngine::BuildRequestBody(const std::string& prompt, bool jsonResponse) const
{
    nlohmann::json body;
    body["model"]  = m_model;
    body["prompt"] = prompt;
    body["stream"] = false;
    body["think"]  = m_think;
    body["options"]["temperature"] = m_temperature;
    body["options"]["num_predict"] = m_numPredict;
    body["options"]["num_ctx"]     = m_numCtx;

    if (jsonResponse)
        body["format"] = "json";

    return body.dump();
}

std::string OllamaEngine::Request(const std::string& reqStr)
{
    return RequestAPI::Request_POST(m_requestURL, reqStr, GetPostHeader(), m_timeoutMs);
}

std::vector<std::string> OllamaEngine::TranslateGroup(const std::vector<std::string>& texts)
{
    if (texts.empty())
        return {};

    return GetBatchResult(Request(GetRequest(texts)));
}

std::vector<std::string> OllamaEngine::GetBatchResult(const std::string& jsonStr)
{
    std::vector<std::string> result;
    if (jsonStr.empty())
        return result;

    nlohmann::json json_obj = nlohmann::json::parse(jsonStr, nullptr, false);
    if (json_obj.is_discarded() || json_obj.find("response") == json_obj.end() ||
        !json_obj["response"].is_string())
        return result;

    nlohmann::json output = nlohmann::json::parse(json_obj["response"].get<std::string>(), nullptr, false);
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

std::vector<std::tuple<std::string, std::string>> OllamaEngine::GetPostHeader() const
{
    std::vector<std::tuple<std::string, std::string>> headers;
    headers.push_back(std::make_tuple("Content-Type", "application/json"));
    return headers;
}
