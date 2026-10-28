#pragma once
#include "EngineBase.h"
class DeepLEngine : public EngineBase
{
public:
    DeepLEngine();
    DeepLEngine(const DeepLEngine& reader)            = delete;
    DeepLEngine& operator=(const DeepLEngine& reader) = delete;

    // 翻译文本
    std::string TranslateText(const std::string& text);

    // 批量翻译
    std::vector<std::string> TranslateBatch(const std::vector<std::string>& texts);

protected:
    // 读取配置文件
    void LoadDataFile();

    // 文本分组
    std::vector<TextBlock> GetTextGroup(const std::vector<std::string>& text);

    // 生成请求
    std::string GetRequest(const std::string& str);
    std::string GetRequest(const std::vector<std::string>& text);

    // 发送请求
    std::vector<std::tuple<std::string, std::string>> ToRequest(const std::string&);

    // 处理结果
    std::vector<std::tuple<std::string, std::string>> GetResult(const std::string& jsonStr);

private:
    // 获取请求头
    std::vector<std::tuple<std::string, std::string>> GetPostHeader();

private:
    int m_reqQuota;
};
