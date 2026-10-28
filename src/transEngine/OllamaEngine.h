#pragma once
#include "EngineBase.h"

class OllamaEngine : public EngineBase
{
public:
    OllamaEngine();
    OllamaEngine(const OllamaEngine& reader)            = delete;
    OllamaEngine& operator=(const OllamaEngine& reader) = delete;

    // Translate a single text.
    std::string TranslateText(const std::string& text);

    // Translate multiple texts while preserving their order.
    std::vector<std::string> TranslateBatch(const std::vector<std::string>& texts);

protected:
    // Read the Ollama section from config.txt.
    void LoadDataFile();

    // Group texts into requests that fit the configured limits.
    std::vector<TextBlock> GetTextGroup(const std::vector<std::string>& text);

    // Build request bodies for single and batch translation.
    std::string GetRequest(const std::string& str);
    std::string GetRequest(const std::vector<std::string>& text);

    // Send a request and parse its translations.
    std::vector<std::tuple<std::string, std::string>> ToRequest(const std::string& reqStr);

    // Parse an Ollama response for a single translation.
    std::vector<std::tuple<std::string, std::string>> GetResult(const std::string& jsonStr);

private:
    std::string BuildRequestBody(const std::string& prompt, bool jsonResponse) const;
    std::string Request(const std::string& reqStr);
    std::vector<std::string> TranslateGroup(const std::vector<std::string>& texts);
    std::vector<std::string> GetBatchResult(const std::string& jsonStr);
    std::vector<std::tuple<std::string, std::string>> GetPostHeader() const;

private:
    std::string m_model;
    int m_reqQuota;
    int m_timeoutMs;
    int m_numPredict;
    int m_numCtx;
    double m_temperature;
    bool m_think;
};
