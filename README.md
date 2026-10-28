# Translater


A simple C++11 translation engine API call implementation.
一个简单的 C++11 翻译引擎 API 调用实现。



Currently supports Baidu API, DeepL API, and local Ollama models.
目前支持百度 API、DeepL API 和本地 Ollama 模型。

Before use, you need to fill in the corresponding API information in config.txt.
For Ollama, start the local service and pull the configured model first.
使用前，需要在 config.txt 文件中填写相应的 API 信息；使用 Ollama 前需先启动本地服务并拉取配置的模型。

A complete configuration template is available in `config.example.txt`.
完整的配置模板见 `config.example.txt`，复制为 `config.txt` 后填写 API 密钥和 Ollama 模型名即可。

The project supports `//` and `/* ... */` comments in `config.txt`, so the annotated template can be used without removing comments.
项目支持 `config.txt` 中的 `//` 和 `/* ... */` 注释，因此模板可以带注释直接使用。

## Ollama

```bash
ollama serve
ollama pull qwen3.5:2b
```

Configure the local engine in `config.txt`:

```json
"Ollama": {
    "requestURL": "http://127.0.0.1:11434/api/generate",
    "model": "qwen3.5:2b",
    "qps": "1",
    "length": "2000",
    "quota": "20",
    "timeout": "120000",
    "temperature": 0.1,
    "numPredict": "512",
    "numCtx": "4096",
    "think": false
}
```

`model` can be changed to any model already available in Ollama. `think` is disabled by default so reasoning models return the translation directly. `timeout` is measured in milliseconds.

`model` 可改为本机已安装的任意 Ollama 模型。`think` 默认关闭，避免推理模型只返回思考内容；`timeout` 单位为毫秒。

Basic usage:
基本用法：

```c++

// Create the translation engine needed
// 创建所需的翻译引擎
std::shared_ptr<EngineBase> ptr(new BaiDuEngine());
std::shared_ptr<EngineBase> ptr2(new DeepLEngine());
std::shared_ptr<EngineBase> ptr3(new OllamaEngine());

// Set the language of the text to be translated and the target language type.
// 设置待翻译文本的语言及目标语言类型。
ptr3->SetFromLanguage("en");
ptr3->SetToLanguage("zh");

// Translate a single text
// 翻译单条文本
ptr3->TranslateText(std::string{});

// Translate multiple texts
// 翻译多条文本
ptr3->TranslateBatch(std::vector<std::string>{});

```

The included demo can test Ollama without calling the remote APIs:
项目自带示例可直接测试 Ollama，不调用远程 API：

```bash
Translater.exe ollama
```
