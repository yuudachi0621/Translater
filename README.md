# Translater

Translater 是一个基于 C++11 的 Windows 控制台翻译引擎示例。项目通过统一的 `EngineBase` 接口封装不同翻译服务，支持单条文本和批量文本翻译。

当前支持：

- 百度翻译 API
- DeepL API
- 本地 Ollama 模型
- OpenAI Chat Completions 兼容 API，包括 OpenAI、DeepSeek、兼容中转服务和本地服务

> 推荐使用 `x64` 配置。仓库同时定义了 Win32 配置，但当前自带的预编译依赖和已验证构建均以 `x64` 为主，详见[环境要求](#环境要求)。

## 功能

- 统一引擎接口：通过 `EngineBase` 提供语言设置、单条翻译和批量翻译。
- 多翻译服务：百度、DeepL、Ollama、OpenAI 兼容 API。
- 批量翻译：按配置的长度和条数分组，并保持返回结果与输入顺序一致。
- 限流与并发：`RateLimiter` 控制 QPS；百度和 DeepL 批量请求使用 4 线程线程池。
- JSON 批处理回退：Ollama 和 OpenAI 兼容引擎优先要求模型返回结构化 JSON；格式不符合要求时自动改为逐条翻译。
- JSON 配置：从进程当前工作目录读取 `config.txt`。
- 配置注释：支持 `//` 和 `/* ... */` 注释。
- 自定义 API：可配置完整接口地址、API Key、模型 ID、超时和生成参数。
- UTF-8 源码：Visual Studio 工程的所有配置均启用 `/utf-8`。

## 技术栈

| 类别 | 实际使用 |
| --- | --- |
| 语言 | C++11 风格源码；工程未固定额外 `/std` 版本 |
| 构建系统 | Visual Studio 2022 Solution / MSBuild |
| 工具集 | MSVC `v143` |
| 目标平台 | Windows，`WindowsTargetPlatformVersion` 为 `10.0` |
| HTTP/HTTPS | libhv 1.3.3，仓库包含头文件和预编译静态库 |
| JSON | nlohmann/json 3.7.3，随源码提供 |
| 程序类型 | Windows 控制台应用 |
| 字符集 | Unicode 工程配置；源码通过 `/utf-8` 编译 |

项目不使用 CMake、vcpkg、Conan、Node.js 或 Python，也没有包安装脚本。

## 架构概览

```text
main.cpp
  └─ EngineBase
      ├─ BaiDuEngine
      ├─ DeepLEngine
      ├─ OllamaEngine
      └─ OpenAICompatibleEngine
          └─ RequestAPI
              └─ libhv HTTP/HTTPS
                  └─ 翻译服务
```

- `EngineBase` 定义 `SetFromLanguage()`、`SetToLanguage()`、`TranslateText()` 和 `TranslateBatch()`。
- 每个具体引擎负责读取自身配置、构造请求、解析响应和处理批量分组。
- `RequestAPI` 封装 GET/POST 请求；POST 支持自定义超时。
- `File::ReadJsonConfig()` 读取 `config.txt`，并在解析前移除配置中的注释。
- `RateLimiter` 控制请求频率，`ThreadPool` 用于百度和 DeepL 的批量并发。
- 配置只提供各引擎的参数，不会自动选择引擎。调用方需要在代码中实例化所需引擎。

## 目录结构

```text
Translater/
├─ src/
│  ├─ main.cpp                    # 控制台入口和冒烟测试
│  ├─ algorithm/                  # MD5、URL 编码、Base64
│  ├─ file/                       # 文件读取和带注释 JSON 配置读取
│  ├─ hv/                         # 随项目提供的 libhv 头文件
│  ├─ json/                       # nlohmann/json 单头文件
│  ├─ limiter/                    # QPS 限流
│  ├─ network/                    # HTTP GET/POST 封装
│  ├─ thread/                     # 线程池
│  └─ transEngine/                # EngineBase 和各翻译引擎
├─ lib/
│  ├─ debug/hv_static.lib         # Debug x64 使用的 libhv 静态库
│  └─ release/hv_static.lib       # Release x64 使用的 libhv 静态库
├─ config.example.txt             # 带字段注释的完整配置模板
├─ config.txt                     # 本地运行配置
├─ Translater.sln                 # Visual Studio 解决方案
├─ Translater.vcxproj             # 项目及依赖配置
├─ Translater.vcxproj.filters     # Visual Studio 文件分组
├─ LICENSE                        # MIT License
└─ README.md
```

`x64/` 是构建输出目录，已被 `.gitignore` 忽略。`Translater.vcxproj.user` 是本地 Visual Studio 用户配置，不应作为项目功能文件提交。

## 环境要求

- Windows 10 或更高版本。
- Visual Studio 2022，安装“使用 C++ 的桌面开发”工作负载。
- MSVC `v143` 工具集。
- Windows 10 SDK。
- 推荐使用 `x64` 平台。
- 使用远程翻译服务时需要能够访问对应的 HTTPS 接口。
- 使用 Ollama 时需要本机或可访问的 Ollama 服务。

环境变量：当前源码没有读取任何项目专用环境变量。`config.txt` 和 Ollama 地址均由配置文件提供。

### 平台说明

- `Debug x64` 和 `Release x64` 已按当前工程配置验证可构建。
- Win32 配置存在于解决方案中，但 `Debug|Win32` 的 libhv include/lib 路径使用了仓库外的绝对路径 `E:\Code\libhv\...`。因此 Win32 构建尚未在本仓库自带依赖下验证。
- 项目未提供 Linux、macOS、CMake、容器或 CI 构建配置。

## 安装与构建

### 1. 准备配置

项目在运行时会从**进程当前工作目录**读取 `config.txt`，不是从可执行文件所在目录自动查找。

可以使用完整模板：

```powershell
Copy-Item .\config.example.txt .\config.txt -Force
```

如果不希望覆盖已有配置，请直接编辑现有的 `config.txt`。必须填写所测试引擎需要的 API Key、AppID 或模型名称。

> `config.txt` 可能包含真实密钥。不要把真实密钥提交到 Git；仓库中的 `config.example.txt` 仅保留空密钥和示例地址。

### 2. 构建

在 Visual Studio 2022 中打开 `Translater.sln`，选择 `Debug|x64` 或 `Release|x64` 后构建。

也可以在“Developer PowerShell for VS 2022”中运行：

```powershell
msbuild .\Translater.sln /m /p:Configuration=Release /p:Platform=x64
```

已验证的输出路径：

```text
x64\Debug\Translater.exe
x64\Release\Translater.exe
```

## 配置说明

`config.txt` 是带注释扩展的 JSON。注释会在解析前由 `File::ReadJsonConfig()` 删除，因此可以保留模板中的说明。

基本结构：

```json
{
    "from": "zh",
    "to": "en",
    "engine": {
        "BaiDu": {},
        "DeepL": {},
        "Ollama": {},
        "OpenAICompatible": {}
    }
}
```

### 公共字段

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `from` | string | 默认源语言；调用 `SetFromLanguage()` 后会被覆盖 |
| `to` | string | 默认目标语言；调用 `SetToLanguage()` 后会被覆盖 |

语言代码会原样传给对应服务，不进行统一转换。请使用目标服务要求的语言代码和大小写。

### 字段类型约定

- `qps`、`length`、`quota`、`timeout`、`numPredict`、`numCtx`、`maxTokens` 在配置中使用字符串形式。
- `temperature` 使用 JSON 数字。
- `think` 使用 JSON 布尔值。
- `length` 按字符串字节数计算，不是字符数。
- `requestURL` 必须是完整接口地址。
- 新配置统一使用 `requestURL`。百度、DeepL 和 Ollama 的读取代码仍兼容旧拼写 `requstURL`，但不应在新配置中继续使用。

### 百度

```json
"BaiDu": {
    "requestURL": "https://fanyi-api.baidu.com/api/trans/vip/translate",
    "apiKey": "",
    "appId": "",
    "qps": "1",
    "length": "2000"
}
```

| 字段 | 说明 |
| --- | --- |
| `apiKey` | 百度翻译 API Key |
| `appId` | 百度翻译 AppID |
| `qps` | 每秒最大请求数 |
| `length` | 单次批量请求累计长度上限，单位字节 |

百度批量翻译使用换行拼接文本，并依赖百度返回的分行结果。

### DeepL

```json
"DeepL": {
    "requestURL": "https://api-free.deepl.com/v2/translate",
    "apiKey": "",
    "qps": "1",
    "length": "2000",
    "quota": "50"
}
```

| 字段 | 说明 |
| --- | --- |
| `apiKey` | DeepL API Key，通过 `DeepL-Auth-Key` 请求头发送 |
| `qps` | 每秒最大请求数 |
| `length` | 单次批量请求累计长度上限，单位字节 |
| `quota` | 单次批量请求最多包含的文本条数 |

免费版通常使用 `api-free.deepl.com`，付费版地址可能不同。

### Ollama

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

| 字段 | 说明 |
| --- | --- |
| `requestURL` | Ollama `/api/generate` 完整地址 |
| `model` | 必须是 `ollama list` 中存在的模型名称 |
| `qps` | 每秒最大请求数 |
| `length` | 单次批量请求累计长度上限，单位字节 |
| `quota` | 单次批量请求最多包含的文本条数 |
| `timeout` | HTTP 超时，单位毫秒 |
| `temperature` | 生成温度 |
| `numPredict` | 单次最大输出 token 数 |
| `numCtx` | 模型上下文窗口大小 |
| `think` | 是否启用思考输出；翻译任务通常设为 `false` |

本机准备示例：

```powershell
ollama serve
ollama pull qwen3.5:2b
ollama list
```

### OpenAI 兼容 API

```json
"OpenAICompatible": {
    "requestURL": "https://api.deepseek.com/chat/completions",
    "apiKey": "",
    "model": "deepseek-chat",
    "qps": "1",
    "length": "2000",
    "quota": "20",
    "timeout": "120000",
    "temperature": 0.1,
    "maxTokens": "1024",
    "reasoningEffort": ""
}
```

| 字段 | 说明 |
| --- | --- |
| `requestURL` | 完整的 Chat Completions 地址 |
| `apiKey` | 非空时以 `Authorization: Bearer <key>` 发送；本地服务可留空 |
| `model` | 服务商要求的模型 ID |
| `qps` | 每秒最大请求数 |
| `length` | 单次批量请求累计长度上限，单位字节 |
| `quota` | 单次批量请求最多包含的文本条数 |
| `timeout` | HTTP 超时，单位毫秒 |
| `temperature` | 生成温度 |
| `maxTokens` | 请求中的 `max_tokens` |
| `reasoningEffort` | 可选；非空时作为 `reasoning_effort` 发送 |

常见地址：

- DeepSeek：`https://api.deepseek.com/chat/completions`
- OpenAI：`https://api.openai.com/v1/chat/completions`
- Ollama OpenAI 兼容端点：`http://127.0.0.1:11434/v1/chat/completions`

部分本地推理模型需要将 `reasoningEffort` 设为 `none` 才会直接返回正文；不支持该参数的服务应留空。本节只实现 OpenAI Chat Completions 协议，不包含 Anthropic 原生协议。

完整配置和逐字段注释见 `config.example.txt`。

## 运行与测试

所有命令应在包含 `config.txt` 的目录中执行，下面的示例以仓库根目录为当前目录。

### 默认入口

```powershell
.\x64\Release\Translater.exe
```

默认入口会依次运行：

1. 百度引擎示例
2. DeepL 引擎示例
3. Ollama 引擎示例

默认入口**不会**运行 OpenAI 兼容引擎。

### 单独运行 Ollama 示例

```powershell
.\x64\Release\Translater.exe ollama
```

只运行 `OllamaEngine`。需要 Ollama 服务已启动，且 `config.txt` 中的 `engine.Ollama.model` 不为空。

### 单独运行自定义 API 示例

```powershell
.\x64\Release\Translater.exe openai
```

或：

```powershell
.\x64\Release\Translater.exe custom
```

两个参数等价，都会只运行 `OpenAICompatibleEngine`。

这些入口是控制台冒烟测试，不是自动化测试套件。项目当前没有单元测试框架、测试脚本或 CI 配置。

### 已验证内容

当前代码库已实际验证：

- `Debug x64` 构建
- `Release x64` 构建
- 本机 Ollama `/api/generate` 单条、中英互译和批量翻译
- 本机 Ollama OpenAI 兼容 `/v1/chat/completions` 单条、中英互译和批量翻译

百度、DeepL、公网 OpenAI 和公网 DeepSeek 的实时调用未在本次文档审查中验证，实际结果取决于账号、密钥、网络和接口可用性。

## 在代码中使用

```cpp
#include <memory>
#include <string>
#include <vector>

#include "EngineBase.h"
#include "OpenAICompatibleEngine.h"

std::shared_ptr<EngineBase> engine(new OpenAICompatibleEngine());

engine->SetFromLanguage("en");
engine->SetToLanguage("zh");

std::string translated = engine->TranslateText("Hello");

std::vector<std::string> texts = {"Hello", "World"};
std::vector<std::string> results = engine->TranslateBatch(texts);
```

同样可以使用 `BaiDuEngine`、`DeepLEngine` 或 `OllamaEngine`。

行为说明：

- 引擎在构造函数中读取 `config.txt`。
- `TranslateBatch()` 返回与输入数量相同的字符串数组，并保持顺序。
- 请求失败、解析失败或 API 返回空内容时，对应结果可能为空字符串。
- `RequestAPI` 当前只接受 HTTP 200 响应；其他状态码会作为空响应处理。
- 当前没有网络请求重试机制；Ollama 和 OpenAI 兼容引擎只对批量 JSON 格式不符合要求的情况回退为逐条请求。

## 部署

项目是控制台程序，不包含安装器、Windows 服务、Docker 配置或远程部署脚本。

Release 部署步骤：

1. 构建 `Release|x64`。
2. 复制 `x64\Release\Translater.exe` 到部署目录。
3. 在同一目录或启动时的工作目录放置有效的 `config.txt`。
4. 从该目录启动程序。

示例：

```powershell
.\Translater.exe openai
```

运行时依赖：

- `Translater.exe` 依赖 MSVC 运行库，包括 `MSVCP140.dll`、`VCRUNTIME140.dll` 和 `VCRUNTIME140_1.dll`。目标机器需要安装 Microsoft Visual C++ 2015-2022 Redistributable（x64）。
- 远程服务需要对应网络和代理配置。
- 使用 Ollama 时需要 Ollama 服务处于运行状态。

不要把包含真实 API Key 的 `config.txt` 打包到公开制品或提交到版本库。

## 许可

本项目使用 MIT License，详见 `LICENSE`。
