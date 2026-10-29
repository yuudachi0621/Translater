#include "EngineBase.h"

#include <cerrno>
#include <climits>
#include <cstdlib>

std::string EngineBase::GetRequestURL() const
{
    return m_requestURL;
}

void EngineBase::SetFromLanguage(const std::string& language)
{
    m_from = language;
}

void EngineBase::SetToLanguage(const std::string& language)
{
    m_to = language;
}

int EngineBase::ParsePositiveInt(const std::string& value, int defaultValue)
{
    if (value.empty())
        return defaultValue;

    errno        = 0;
    char* end    = nullptr;
    const long parsed = std::strtol(value.c_str(), &end, 10);
    if (end == value.c_str() || *end != '\0' || errno == ERANGE || parsed <= 0 || parsed > INT_MAX)
        return defaultValue;

    return static_cast<int>(parsed);
}
