#include "File.h"

#include <cstdio>
#include <fstream>

std::string File::ReadFile(const std::string& filename)
{
    FILE* fp = nullptr;
    fp       = fopen(filename.data(), "rb");
    if (!fp) return std::string();

    if (fseek(fp, 0, SEEK_END) != 0)
    {
        fclose(fp);
        return std::string();
    }

    const long fileSize = ftell(fp);
    if (fileSize < 0 || fseek(fp, 0, SEEK_SET) != 0)
    {
        fclose(fp);
        return std::string();
    }

    std::string fileBin(static_cast<std::size_t>(fileSize), '\0');
    if (!fileBin.empty())
    {
        const std::size_t readSize = fread((char*)fileBin.data(), 1, fileBin.size(), fp);
        if (readSize != fileBin.size())
        {
            fclose(fp);
            return std::string();
        }
    }

    fclose(fp);
    return fileBin;
}

void File::WriteFile(const std::string& filename, const std::string& fileBin)
{
    std::ofstream ofs(filename, std::ios::binary);
    if (!ofs.is_open()) return;
    ofs.write((char*)fileBin.data(), fileBin.size());
}

std::string File::ReadJsonConfig(const std::string& filename)
{
    std::string text = ReadFile(filename);
    if (text.size() >= 3 && static_cast<unsigned char>(text[0]) == 0xEF && static_cast<unsigned char>(text[1]) == 0xBB && static_cast<unsigned char>(text[2]) == 0xBF)
        text.erase(0, 3);

    std::string result;
    result.reserve(text.size());
    bool inString = false;

    for (std::size_t i = 0; i < text.size();)
    {
        const char current = text[i];
        if (inString)
        {
            result.push_back(current);
            if (current == '\\' && i + 1 < text.size())
            {
                result.push_back(text[i + 1]);
                i += 2;
                continue;
            }
            if (current == '"')
                inString = false;
            ++i;
            continue;
        }

        if (current == '"')
        {
            inString = true;
            result.push_back(current);
            ++i;
            continue;
        }

        if (current == '/' && i + 1 < text.size() && text[i + 1] == '/')
        {
            i += 2;
            while (i < text.size() && text[i] != '\r' && text[i] != '\n')
                ++i;
            continue;
        }

        if (current == '/' && i + 1 < text.size() && text[i + 1] == '*')
        {
            i += 2;
            while (i + 1 < text.size() && !(text[i] == '*' && text[i + 1] == '/'))
            {
                if (text[i] == '\r' || text[i] == '\n')
                    result.push_back(text[i]);
                ++i;
            }
            if (i + 1 < text.size())
                i += 2;
            continue;
        }

        result.push_back(current);
        ++i;
    }

    return result;
}