#pragma once
#include <string>
class File
{
public:
    // 读取文件
    static std::string ReadFile(const std::string& filename);

    // Read a JSON configuration file and remove // and /* */ comments.
    static std::string ReadJsonConfig(const std::string& filename);

    // 写入文件
    static void WriteFile(const std::string& filename, const std::string& fileBin);
};
