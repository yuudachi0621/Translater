#include "EngineBase.h"

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
