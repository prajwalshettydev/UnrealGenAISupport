# Unreal Engine Generative AI Support Plugin

> [!WARNING]  
> This plugin is currently under development and not ready for production use.

Every week, hundreds of new AI models are released by various organizations, making it hard to keep up with the latest advancements.

The Unreal Engine Generative AI Support Plugin allows you to focus on game development without worrying about the LLM/GenAI integration layer.

Currently integrating Model Control Protocol (MCP) with Unreal Engine 5.5.

This project aims to build a long-term support (LTS) plugin for various cutting-edge LLM/GenAI models and foster a
community around it. It currently includes OpenAI's GPT-4o, Deepseek R1, Claude Sonnet 3.7 and GPT-4o-mini for Unreal Engine 5.1 or higher, with plans to add
, real-time APIs, Gemini, MCP, and Grok 3 APIs soon. The plugin will focus exclusively on APIs useful for
game development and interactive experiences. All suggestions and contributions are welcome.

## Current Progress:

- OpenAI API Support:
    - OpenAI Chat API ✅
        - `gpt-4o` Model ✅
        - `gpt-4o-mini` Model ✅
        - `gpt-4.5` Model 🛠️
        - `o1-mini` Model 🚧
        - `o1` Model 🚧
        - `o3-mini-high` Model 🚧
    - OpenAI DALL-E API 🚧
    - OpenAI Vision API 🚧
    - OpenAI Realtime API 🛠️
    - OpenAI Structured Outputs ✅
    - OpenAI Text-To-Speech API 🚧
    - OpenAI Whisper API 🚧
- Anthropic Claude API Support:
    - Claude Chat API ✅
        - `claude-3-7-sonnet-latest` Model ✅
        - `claude-3-5-sonnet` Model ✅
        - `claude-3-5-haiku-latest` Model ✅
        - `claude-3-opus-latest` Model ✅ 
    - Claude Vision API 🚧
- Model Control Protocol (MCP) 🛠️
    - Clients Support ✅
        - Claude Desktop App Support ✅
        - OpenAI Operator API Support 🚧
    - Blueprints Auto Generation 🛠️
    - Level/Scene Control for LLMs 🛠️
- XAI (Grok 3) API Support:
    - XAI Chat Completions API 🚧
        - `grok-beta` Model 🚧
        - `grok-beta` Streaming API 🚧
    - XAI Image API 🚧
- Google Gemini API Support:
    - Gemini Chat API 🚧🤝
        - `gemini-2.0-flash-exp` Model 🚧🤝
        - `gemini-1.5-flash` Model 🚧🤝
        - `gemini-1.5-flash-8b` Model 🚧🤝
    - Gemini Multi-Modal API 🚧🤝
- Meta AI API Support:
    - Llama Chat API 🚧
        - `llama3.3-70b` Model 🚧
        - `llama3.1-8b` Model 🚧
    - Multi-Modal Vision API 🚧
        - `llama3.2-90b-vision` Model 🚧
    - Local Llama API 🚧🤝
- Deepseek API Support:
    - Deepseek Chat API ✅
        - `deepseek-chat` (DeepSeek-V3) Model ✅
    - Deepseek Reasoning API, R1 ✅
        - `deepseek-reasoning-r1` Model ✅
        - `deepseek-reasoning-r1` CoT Streaming ❌
    - Independently Hosted Deepseek Models 🚧
- Baidu API Support:
    - Baidu Chat API 🚧
        - `baidu-chat` Model 🚧
- API Key Management 
    - Cross-Platform Secure Key Storage ✅
    - Encrypted Key Storage 🛠️
    - Cross Platform Testing 🚧
    - Build System Integration 🛠️
    - Keys in Build Configuration 🛠️
- Unreal Engine Integration
    - Blueprint Support 🛠️
    - C++ Support 🛠️
    - C++ Latent Functions For Blueprints 🛠️
    - Packaged Build Support 🛠️
- Plugin Documentation 🛠️🤝
- Plugin Example Project 🛠️ [here](https://github.com/prajwalshettydev/unreal-llm-api-test-project)
- Version Control Support
    - Perforce Support 🚧
    - Git Submodule Support ✅ 
- LTS Branching 🚧
    - Stable Branch with Bug Fixes 🚧
    - Dedicated Contributor for LTS 🚧
- Lightweight Plugin (In Builds) 
    - No External Dependencies ✅
    - Build Flags to enable/disable APIs 🚧
    - Submodules per API Organization 🚧
- Testing 
    - Automated Testing 🚧
    - Build Testing 🚧
    - Different Platforms 🚧🤝
    - Different Engine Versions 🚧🤝


Where,
- ✅ - Completed
- 🛠️ - In Progress
- 🚧 - Planned
- 🤝 - Need Contributors
- ❌ - Won't Support For Now

## Quick Links:

- [OpenAI API Documentation](https://platform.openai.com/docs/api-reference)
- [Anthropic API Documentation](https://docs.anthropic.com/en/docs/about-claude/models)
- [XAI API Documentation](https://docs.x.ai/api)
- [Google Gemini API Documentation](https://ai.google.dev/gemini-api/docs/models/gemini)
- [Meta AI API Documentation](https://docs.llama-api.com/quickstart#available-models)
- [Deepseek API Documentation](https://api-docs.deepseek.com/)
- [Model Control Protocol (MCP) Documentation](https://modelcontextprotocol.io/)

# Table of Contents

- [Current Progress](#current-progress)
- [Quick Links](#quick-links)
- [Setting API Key](#setting-api-key)
    - [For Editor](#for-editor)
    - [For Packaged Builds](#for-packaged-builds)
- [Adding the plugin to your project](#adding-the-plugin-to-your-project)
    - [With Git](#with-git)
    - [With Perforce](#with-perforce)
    - [With Unreal Marketplace](#with-unreal-marketplace)
- [Fetching the Latest Plugin Changes](#fetching-the-latest-plugin-changes)
    - [With Git](#with-git-1)
    - [With Perforce](#with-perforce-1)
- [Usage](#usage)
    - [OpenAI](#openai)
        - [1. Chat](#1-chat)
        - [2. Structured Outputs](#2-structured-outputs)
    - [DeepSeek API](#deepseek-api)
        - [1. Chat and Reasoning](#1-chat-and-reasoning)
    - [Anthropic API](#anthropic-api)
        - [1. Chat](#1-chat-1)
    - [Model Control Protocol (MCP)](#model-control-protocol-mcp)
        - [Setting up](#setting-up)
        - [Usage](#usage-1)
- [Contribution Guidelines](#contribution-guidelines)
- [Project Structure](#project-structure)
- [References](#references)

## Setting API Key:

### For Editor:

Set the environment variable `PS_<ORGNAME>` to your API key.
#### For Windows:
```cmd
setx PS_<ORGNAME> "your api key"
```

#### For Linux/MacOS:

1. Run the following command in your terminal, replacing yourkey with your API key.
    ```bash
    echo "export PS_<ORGNAME>='yourkey'" >> ~/.zshrc
    ```

2. Update the shell with the new variable:
    ```bash
    source ~/.zshrc
    ```

PS: Don't forget to restart the Editor and ALSO the connected IDE after setting the environment variable.

Where `<ORGNAME>` can be:
`PS_OPENAIAPIKEY`, `PS_DEEPSEEKAPIKEY`, `PS_ANTHROPICAPIKEY`, `PS_METAAPIKEY`, `PS_GOOGLEAPIKEY` etc.

### For Packaged Builds:

Storing API keys in packaged builds is a security risk. This is what the OpenAI API documentation says about it:
>"Exposing your OpenAI API key in client-side environments like browsers or mobile apps allows malicious users to take that key and make requests on your behalf – which may lead to unexpected charges or compromise of certain account data. Requests should always be routed through your own backend server where you can keep your API key secure."

Read more about it [here](https://help.openai.com/en/articles/5112595-best-practices-for-api-key-safety).

For test builds you can call the `GenSecureKey::SetGenAIApiKeyRuntime` either in c++ or blueprints function with your API key in the packaged build.

## Adding the plugin to your project:

### With Git:

1. Add the Plugin Repository as a Submodule in your project's repository.

   ```cmd
   git submodule add https://github.com/prajwalshettydev/UnrealGenAISupport Plugins/GenerativeAISupport
   ```

2. Regenerate Project Files:
   Right-click your .uproject file and select Generate Visual Studio project files.
3. Enable the Plugin in Unreal Editor:
   Open your project in Unreal Editor. Go to Edit > Plugins. Search for the Plugin in the list and enable it.
4. For Unreal C++ Projects, include the Plugin's module in your project's Build.cs file:

   ```cpp
   PrivateDependencyModuleNames.AddRange(new string[] { "GenerativeAISupport" });
   ```

### With Perforce:

Still in development..

### With Unreal Marketplace:
Still in development..

## Fetching the Latest Plugin Changes:

### With Git:

you can pull the latest changes with:

```cmd
cd Plugins/GenerativeAISupport
git pull origin main
```

Or update all submodules in the project:

```cmd
git submodule update --recursive --remote
```

### With Perforce:

Still in development..

## Usage:
There is a example Unreal project that already implements the plugin. You can find it [here](https://github.com/prajwalshettydev/unreal-llm-api-test-project).

### OpenAI:

Currently the plugin supports Chat and Structured Outputs from OpenAI API. Both for C++ and Blueprints.
Tested models are `gpt-4o`, `gpt-4o-mini`, `gpt-4.5`, `o1-mini`, `o1`, `o3-mini-high`.

#### 1. Chat:

   ##### C++ Example:
    ```cpp
    void SomeDebugSubsystem::CallGPT(const FString& Prompt, 
        const TFunction<void(const FString&, const FString&, bool)>& Callback)
    {
        FGenChatSettings ChatSettings;
        ChatSettings.Model = TEXT("gpt-4o-mini");
        ChatSettings.MaxTokens = 500;
        ChatSettings.Messages.Add(FGenChatMessage{ TEXT("system"), Prompt });
    
        FOnChatCompletionResponse OnComplete = FOnChatCompletionResponse::CreateLambda(
            [Callback](const FString& Response, const FString& ErrorMessage, bool bSuccess)
        {
            Callback(Response, ErrorMessage, bSuccess);
        });
    
        UGenOAIChat::SendChatRequest(ChatSettings, OnComplete);
    }
    ```

   ##### Blueprint Example:

<img src="Docs/BpExampleOAIChat.png" width="782"/>

#### 2. Structured Outputs:
   ##### C++ Example 1:
   Sending a custom schema json directly to function call
   ```cpp
   FString MySchemaJson = R"({
   "type": "object",
   "properties": {
       "count": {
           "type": "integer",
           "description": "The total number of users."
       },
       "users": {
           "type": "array",
           "items": {
               "type": "object",
               "properties": {
                   "name": { "type": "string", "description": "The user's name." },
                   "heading_to": { "type": "string", "description": "The user's destination." }
               },
               "required": ["name", "role", "age", "heading_to"]
           }
       }
   },
   "required": ["count", "users"]
   })";
   
   UGenAISchemaService::RequestStructuredOutput(
       TEXT("Generate a list of users and their details"),
       MySchemaJson,
       [](const FString& Response, const FString& Error, bool Success) {
          if (Success)
          {
              UE_LOG(LogTemp, Log, TEXT("Structured Output: %s"), *Response);
          }
          else
          {
              UE_LOG(LogTemp, Error, TEXT("Error: %s"), *Error);
          }
       }
   );
   ```
   ##### C++ Example 2:
   Sending a custom schema json from a file
   ```cpp
   #include "Misc/FileHelper.h"
   #include "Misc/Paths.h"
   FString SchemaFilePath = FPaths::Combine(
       FPaths::ProjectDir(),
       TEXT("Source/:ProjectName/Public/AIPrompts/SomeSchema.json")
   );
   
   FString MySchemaJson;
   if (FFileHelper::LoadFileToString(MySchemaJson, *SchemaFilePath))
   {
       UGenAISchemaService::RequestStructuredOutput(
           TEXT("Generate a list of users and their details"),
           MySchemaJson,
           [](const FString& Response, const FString& Error, bool Success) {
              if (Success)
              {
                  UE_LOG(LogTemp, Log, TEXT("Structured Output: %s"), *Response);
              }
              else
              {
                  UE_LOG(LogTemp, Error, TEXT("Error: %s"), *Error);
              }
           }
       );
   }
   ```

### DeepSeek API:

Currently the plugin supports Chat and Reasoning from DeepSeek API. Both for C++ and Blueprints.
Points to note:
* System messages are currently mandatory for the reasoning model. API otherwise seems to return null
* Also, from the documentation: "Please note that if the reasoning_content field is included in the sequence of input messages, the API will return a 400 error.
  Read more about it [here](https://api-docs.deepseek.com/guides/reasoning_model)"

> [!WARNING]  
> While using the R1 reasoning model, make sure the Unreal's HTTP timeouts are not the default values at 30 seconds.
> As these API calls can take longer than 30 seconds to respond. Simply setting the `HttpRequest->SetTimeout(<N Seconds>);` is not enough
> So the following lines need to be added to your project's `DefaultEngine.ini` file:
> ```ini
> [HTTP]
> HttpConnectionTimeout=180
> HttpReceiveTimeout=180
> ```

#### 1. Chat and Reasoning:
##### C++ Example:

   ```cpp
    FGenDSeekChatSettings ReasoningSettings;
    ReasoningSettings.Model = EDeepSeekModels::Reasoner; // or EDeepSeekModels::Chat for Chat API
    ReasoningSettings.MaxTokens = 100;
    ReasoningSettings.Messages.Add(FGenChatMessage{TEXT("system"), TEXT("You are a helpful assistant.")});
    ReasoningSettings.Messages.Add(FGenChatMessage{TEXT("user"), TEXT("9.11 and 9.8, which is greater?")});
    ReasoningSettings.bStreamResponse = false;
    UGenDSeekChat::SendChatRequest(
        ReasoningSettings,
        FOnDSeekChatCompletionResponse::CreateLambda(
            [this](const FString& Response, const FString& ErrorMessage, bool bSuccess)
            {
                if (!UTHelper::IsContextStillValid(this))
                {
                    return;
                }

                // Log response details regardless of success
                UE_LOG(LogTemp, Warning, TEXT("DeepSeek Reasoning Response Received - Success: %d"), bSuccess);
                UE_LOG(LogTemp, Warning, TEXT("Response: %s"), *Response);
                if (!ErrorMessage.IsEmpty())
                {
                    UE_LOG(LogTemp, Error, TEXT("Error Message: %s"), *ErrorMessage);
                }
            })
    );
   ```

### Anthropic API:
Currently the plugin supports Chat from Anthropic API. Both for C++ and Blueprints.
Tested models are `claude-3-7-sonnet-latest`, `claude-3-5-sonnet`, `claude-3-5-haiku-latest`, `claude-3-opus-latest`.

#### 1. Chat:
##### C++ Example:
```cpp
    // ---- Claude Chat Test ----
    FGenClaudeChatSettings ChatSettings;
    ChatSettings.Model = EClaudeModels::Claude_3_7_Sonnet; // Use Claude 3.7 Sonnet model
    ChatSettings.MaxTokens = 4096;
    ChatSettings.Temperature = 0.7f;
    ChatSettings.Messages.Add(FGenChatMessage{TEXT("system"), TEXT("You are a helpful assistant.")});
    ChatSettings.Messages.Add(FGenChatMessage{TEXT("user"), TEXT("What is the capital of France?")});
    
    UGenClaudeChat::SendChatRequest(
        ChatSettings,
        FOnClaudeChatCompletionResponse::CreateLambda(
            [this](const FString& Response, const FString& ErrorMessage, bool bSuccess)
            {
                if (!UTHelper::IsContextStillValid(this))
                {
                    return;
                }
    
                if (bSuccess)
                {
                    UE_LOG(LogTemp, Warning, TEXT("Claude Chat Response: %s"), *Response);
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("Claude Chat Error: %s"), *ErrorMessage);
                }
            })
    );
```

## Model Control Protocol (MCP):
This is currently work in progress. The plugin will support various clients like Claude Desktop App, OpenAI Operator API etc.

### Setting up:
##### 1. Install the Claude Desktop App from [here](https://claude.anthropic.com/).
##### 2. Setup the `claude_desktop_config.json` file in Claude Desktop App's installation directory. (might ask claude where its located for your platform!)
The file will look something like this:
```json
{
    "mcpServers": {
      "unreal-handshake": {
        "command": python,
        "args": ["<your_project_directoy>/ExamplesForUEGenAIPlugin/Plugins/GenerativeAISupport/Content/Python/mcp_server.py"],
        "env": {
          "UNREAL_HOST": "localhost",
          "UNREAL_PORT": "9877" 
        }
      }
    }
}
```
##### 3. Install MCP and MCP[CLI] from with either pip or cv
##### 4. Enable python plugin in Unreal Engine. (Edit -> Plugins -> Python Editor Script Plugin)

### Usage:

More documentation will follow.

## Contribution Guidelines:


### Project Structure:


## References:

* Env Var set logic
  from: [OpenAI-Api-Unreal by KellanM](https://github.com/KellanM/OpenAI-Api-Unreal/blob/main/Source/OpenAIAPI/Private/OpenAIUtils.cpp)
* MCP Server inspiration
  from: [Blender-MCP by ahujasid](https://github.com/ahujasid/blender-mcp)