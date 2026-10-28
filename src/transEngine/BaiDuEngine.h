#pragma once
#include "EngineBase.h"
class BaiDuEngine : public EngineBase
{
public:
    BaiDuEngine();
    BaiDuEngine(const BaiDuEngine& reader)            = delete;
    BaiDuEngine& operator=(const BaiDuEngine& reader) = delete;

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
    // 计算MD5
    std::string EncryptString(const std::string& str);

private:
    std::string m_appId;
};
