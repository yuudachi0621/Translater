#include <iostream>
#include <chrono>
#include <thread>

#include "BaiDuEngine.h"
#include "DeepLEngine.h"
#include "OllamaEngine.h"
#include "OpenAICompatibleEngine.h"

#ifdef _WIN32
#include <windows.h>
#endif
using namespace std;

// data set     数据集
std::string transText1;
std::string transText2;
std::string transText3;
std::string transText4;
std::vector<std::string> transTextVec;

auto resetFunc = [&]() {
    transTextVec.clear();

    transText1 = "The beauty of life lies in the journey, not the destination. ";
    transText2 = "The greatest glory in living lies not in never falling, but in rising every time we fall.";
    transText3 = "苹果";
    transText4 = "小米";
    transTextVec.push_back("香辣鸡腿堡");
    transTextVec.push_back("生椰拿铁");
    transTextVec.push_back("阿玛特拉斯");
    transTextVec.push_back("我不知道将去何方，但我已在路上。");
};

auto printFunc = [](const string& src, const string& dst) {
    cout << "src: " << src << "\ndst: " << dst << endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
};
auto printVecFunc = [](const std::vector<std::string>& src, const std::vector<std::string>& dst) {
    for (int i = 0; i < dst.size(); i++)
    {
        cout << "src: " << src[i] << "\n"
             << "dst: " << dst[i] << endl;
    }
};

void testBaiDuFunc()
{
    resetFunc();
    cout << "*BaiDu Engine:" << endl;

    std::shared_ptr<EngineBase> ptr(new BaiDuEngine());
    ptr->SetFromLanguage("en");
    ptr->SetToLanguage("zh");
    printFunc(transText1, ptr->TranslateText(transText1));
    printFunc(transText2, ptr->TranslateText(transText2));

    ptr->SetFromLanguage("zh");
    ptr->SetToLanguage("en");
    printFunc(transText3, ptr->TranslateText(transText3));
    printFunc(transText4, ptr->TranslateText(transText4));

    cout << endl;
    printVecFunc(transTextVec, ptr->TranslateBatch(transTextVec));
    cout << endl;
}

void testDeepLFunc()
{
    resetFunc();
    cout << "*DeepL Engine:" << endl;

    std::shared_ptr<EngineBase> ptr(new DeepLEngine());
    ptr->SetFromLanguage("en");
    ptr->SetToLanguage("zh");
    printFunc(transText1, ptr->TranslateText(transText1));
    printFunc(transText2, ptr->TranslateText(transText2));

    ptr->SetFromLanguage("zh");
    ptr->SetToLanguage("en");
    printFunc(transText3, ptr->TranslateText(transText3));
    printFunc(transText4, ptr->TranslateText(transText4));

    cout << endl;
    printVecFunc(transTextVec, ptr->TranslateBatch(transTextVec));
    cout << endl;
}

void testOllamaFunc()
{
    resetFunc();
    cout << "*Ollama Engine:" << endl;

    std::shared_ptr<EngineBase> ptr(new OllamaEngine());
    ptr->SetFromLanguage("en");
    ptr->SetToLanguage("zh");
    printFunc(transText1, ptr->TranslateText(transText1));
    printFunc(transText2, ptr->TranslateText(transText2));

    ptr->SetFromLanguage("zh");
    ptr->SetToLanguage("en");
    printFunc(transText3, ptr->TranslateText(transText3));
    printFunc(transText4, ptr->TranslateText(transText4));

    cout << endl;
    printVecFunc(transTextVec, ptr->TranslateBatch(transTextVec));
    cout << endl;
}

void testOpenAICompatibleFunc()
{
    resetFunc();
    cout << "OpenAI Compatible Engine:" << endl;

    std::shared_ptr<EngineBase> ptr(new OpenAICompatibleEngine());
    ptr->SetFromLanguage("en");
    ptr->SetToLanguage("zh");
    printFunc(transText1, ptr->TranslateText(transText1));
    printFunc(transText2, ptr->TranslateText(transText2));

    ptr->SetFromLanguage("zh");
    ptr->SetToLanguage("en");
    printFunc(transText3, ptr->TranslateText(transText3));
    printFunc(transText4, ptr->TranslateText(transText4));

    cout << endl;
    printVecFunc(transTextVec, ptr->TranslateBatch(transTextVec));
    cout << endl;
}
int main(int argc, char* argv[])
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    if (argc > 1 && std::string(argv[1]) == "ollama")
    {
        testOllamaFunc();
        return 0;
    }

    if (argc > 1 && (std::string(argv[1]) == "openai" || std::string(argv[1]) == "custom"))
    {
        testOpenAICompatibleFunc();
        return 0;
    }

    testBaiDuFunc();
    testDeepLFunc();
    testOllamaFunc();
}
