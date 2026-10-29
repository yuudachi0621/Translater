#pragma once
#include "EngineBase.h"

// OpenAI Chat Completions compatible engine.
// Works with OpenAI, DeepSeek, compatible gateways, and local servers that
// expose /v1/chat/completions.
class OpenAICompatibleEngine : public EngineBase
{
public:
    OpenAICompatibleEngine();
    OpenAICompatibleEngine(const OpenAICompatibleEngine& reader)            = delete;
    OpenAICompatibleEngine& operator=(const OpenAICompatibleEngine& reader) = delete;

    // Translate a single text.
    std::string TranslateText(const std::string& text);

    // Translate multiple texts while preserving their order.
    std::vector<std::string> TranslateBatch(const std::vector<std::string>& texts);

protected:
    // Read the OpenAICompatible section from config.txt.
    void LoadDataFile();

    // Group texts into requests that fit the configured limits.
    std::vector<TextBlock> GetTextGroup(const std::vector<std::string>& text);

    // Build request bodies for single and batch translation.
    std::string GetRequest(const std::string& str);
    std::string GetRequest(const std::vector<std::string>& text);

    // Send a request and parse its translations.
    std::vector<std::tuple<std::string, std::string>> ToRequest(const std::string& reqStr);

    // Parse an OpenAI-compatible response for a single translation.
    std::vector<std::tuple<std::string, std::string>> GetResult(const std::string& jsonStr);

private:
    std::string BuildRequestBody(const std::string& systemPrompt,
                                 const std::string& userPrompt) const;
    std::string Request(const std::string& reqStr);
    std::vector<std::string> TranslateGroup(const std::vector<std::string>& texts);
    std::vector<std::string> GetBatchResult(const std::string& jsonStr);
    std::vector<std::tuple<std::string, std::string>> GetPostHeader() const;

private:
    std::string m_model;
    std::string m_apiKey;
    std::string m_reasoningEffort;
    int m_reqQuota;
    int m_timeoutMs;
    int m_maxTokens;
    double m_temperature;
};
