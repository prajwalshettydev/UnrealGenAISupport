[![Discord](https://img.shields.io/badge/Discord-7289DA?style=for-the-badge&logo=discord&logoColor=white)](https://discord.com/invite/KBWmkCKv5U)
[![License: MIT](https://img.shields.io/badge/License-MIT-007EC7?style=for-the-badge)](LICENSE)

## Usage Examples:
#### MCP Example:
Claude spawning scene objects and controlling their transformations and materials, generating blueprints, functions, variables, adding components, running python scripts etc.

<img src="Docs/UnrealMcpDemo.gif" width="480"/>

#### API Example:
A project called become human, where NPCs are OpenAI agentic instances. Built using this plugin.
![Become Human](Docs/BHDemoGif.gif)

# Unreal Engine Generative AI Support Plugin:


Every month, hundreds of new AI models are released by various organizations, making it hard to keep up with the latest advancements.

The "Unreal Engine Generative AI Support Plugin" allows you to focus on game development without worrying about the LLM/GenAI integration layer.

<p align="center"><img src="Docs/Repo Card - Updated.png" width="512"/></p>

Currently working on chinese LLMs support like Alibaba Qwen for both Chinese Mainland and International markets.

This project aims to build a long-term support (LTS) plugin for various cutting-edge LLM/GenAI models and foster a
community around it. It currently includes OpenAI's GPT, Deepseek R1, Claude Sonnet 4, Claude Opus 4, and GPT-4o-mini for Unreal Engine, with plans to add
, real-time APIs, Gemini, MCP, and Grok 3 APIs soon. The plugin will focus exclusively on APIs useful for
game development, evals and interactive experiences. All suggestions and contributions are welcome. The plugin can also be used for setting up new evals and ways to compare models in game battlefields.

Compatible with Unreal Engine 5.4 to 5.7+

## Current Progress:

**This plugin will continue to get updates with the latest features and models as they become available. Contributions are welcome.**
**If you want a more production ready alternative, with more features and guaranteed stability please checkout the [Gen AI Pro](https://muddyterrain.com/t/genai-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-plugin), [Gen AI Pro China](https://muddyterrain.com/t/genai-china-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-china-plugin), and [GenAI Model Generator](https://muddyterrain.com/t/genai-model-generator-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=gen3d-plugin) plugins (Supports Unreal Engine 5.1 to 5.7), as it costs a lot of API credits for me to test different models per feature and per engine version to make sure everything works well and compatible. Together these plugins support 200+ AI models, from all over the globe! Otherwise, I feel this free plugin is good enough for many use cases (including the examples shown in the beginning) and you can use it for free, forever.**

### LLM/GenAI API Support:

- OpenAI API Support:
    - OpenAI Chat API ✅ 
        - `gpt-5.2`, `gpt-5.1`, `gpt-5`, `gpt-5-mini`, `gpt-5-nano` [(available in pro)](https://muddyterrain.com/t/genai-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-plugin) ☑️
        - `gpt-4.1`, `gpt-4.1-mini`, `gpt-4.1-nano`,`o4-mini`, `o3`, `o3-pro`, `o3-mini` Models ✅ 
    - Responses API: `gpt-5.4`, `gpt-5.4-pro`, `gpt-5.3-codex`, `gpt-5.2-codex`, `gpt-5.1-codex-max`, `gpt-5-codex-mini` [(available in pro)](https://muddyterrain.com/t/genai-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-plugin) ☑️
    - Image Generation API: `gpt-image-1.5`, `gpt-image-1`, `dall-e-3`, `dall-e-2` [(available in pro)](https://muddyterrain.com/t/genai-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-plugin) ☑️
    - OpenAI Vision API  [(available in pro)](https://muddyterrain.com/t/genai-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-plugin) ☑️
    - OpenAI Realtime API  
        - `gpt-realtime`, `gpt-realtime-mini` Models [(available in pro)](https://muddyterrain.com/t/genai-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-plugin) ☑️
    - OpenAI Structured Outputs ✅
    - Function Calling (Tool Use) [(available in pro)](https://muddyterrain.com/t/genai-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-plugin) ☑️
    - OpenAI Whisper & TTS API: `gpt-4o-mini-tts`, `tts-1`, `tts-1-hd`, `gpt-4o-transcribe`, `whisper-1` [(available in pro)](https://muddyterrain.com/t/genai-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-plugin) ☑️
    - Multimodal API Support  [(available in pro)](https://muddyterrain.com/t/genai-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-plugin) ☑️
    - Text and Audio Streaming  [(available in pro)](https://muddyterrain.com/t/genai-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-plugin) ☑️
- Anthropic Claude API Support:
    - Claude Chat API ✅
        - `claude-opus-4-6`, `claude-sonnet-4-6`, `claude-4.5-opus`, `claude-4.5-sonnet`, `claude-4.5-haiku` [(available in pro)](https://muddyterrain.com/t/genai-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-plugin) ☑️
        - `claude-4-latest`, `claude-3-7-sonnet-latest`, `claude-3-5-sonnet`, `claude-3-5-haiku-latest`, `claude-3-opus-latest` Model ✅
    - Function Calling (Tool Use) [(available in pro)](https://muddyterrain.com/t/genai-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-plugin) ☑️
    - Extended Thinking (Claude 4.5+) [(available in pro)](https://muddyterrain.com/t/genai-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-plugin) ☑️
    - Multimodal/Vision API Support  [(available in pro)](https://muddyterrain.com/t/genai-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-plugin) ☑️
- XAI (Grok) API Support:
    - XAI Chat Completions API ✅
        - `grok-3-latest`, `grok-3-mini-beta` Model ✅
        - `grok-3`, `grok-3-mini`, `grok-3-fast`, `grok-3-mini-fast`, `grok-2-vision-1212`, `grok-2-1212`.
        - `grok-4.1`, `grok-4`, `grok-4-eu`, `grok-code-fast-1` Reasoning API [(available in pro)](https://muddyterrain.com/t/genai-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-plugin) ☑️
    - XAI Image API 🚧
    - Text Streaming API  [(available in pro)](https://muddyterrain.com/t/genai-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-plugin) ☑️
    - Multimodal API Support  [(available in pro)](https://muddyterrain.com/t/genai-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-plugin) ☑️
- Google Gemini API Support:
    - Gemini Chat API 
        - `gemini-3.1-pro-preview`, `gemini-3.1-flash-lite-preview`, `gemini-3-pro-preview`, `gemini-3-flash-preview` [(available in pro)](https://muddyterrain.com/t/genai-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-plugin) ☑️
        - `gemini-2.5-pro`, `gemini-2.5-flash`, `gemini-2.5-flash-lite` [(available in pro)](https://muddyterrain.com/t/genai-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-plugin) ☑️
    - Gemini Imagen API:  
      - `imagen-4.0-generate`, `imagen-4.0-ultra`, `imagen-4.0-fast` [(available in pro)](https://muddyterrain.com/t/genai-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-plugin) ☑️
      - `nano-banana`, `nano-banana pro`, `nano-banana 2` [(available in pro)](https://muddyterrain.com/t/genai-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-plugin) ☑️
    - Gemini Realtime API: `gemini-2.5-flash-native-audio`, `gemini-live-2.5-flash` [(available in pro)](https://muddyterrain.com/t/genai-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-plugin) ☑️
    - Google TTS & Transcription API: `gemini-2.5-flash-preview-tts`, `gemini-2.5-pro-preview-tts` [(available in pro)](https://muddyterrain.com/t/genai-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-plugin) ☑️
    - Multimodal API Support  [(available in pro)](https://muddyterrain.com/t/genai-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-plugin) ☑️
- ElevenLabs:
    - Text To Speech (TTS): `eleven_v3`, `eleven_turbo_v2_5`, `eleven_flash_v2_5`, `eleven_multilingual_v2` [(available in pro)](https://muddyterrain.com/t/genai-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-plugin) ☑️
    - Transcription: `scribe_v2`, `scribe_v2_realtime`, `scribe_v1` [(available in pro)](https://muddyterrain.com/t/genai-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-plugin) ☑️
    - Sound Effects: `eleven_text_to_sound_v2` [(available in pro)](https://muddyterrain.com/t/genai-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-plugin) ☑️
- Inworld AI:
    - Text-to-Speech: `inworld-tts-1.5-max`, `inworld-tts-1.5-mini`, `inworld-tts-1` [(available in pro)](https://muddyterrain.com/t/genai-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-plugin) ☑️
- Meta AI API Support: ❌
- Local AI With Ollama: Available here: [unreal-ollama (MIT)](https://github.com/MuddyTerrain/unreal-ollama)
    - Supports local LLMs like Openai's `gpt-oss`, Alibaba's `qwen3-vl` etc. [unreal-ollama (MIT)](https://github.com/MuddyTerrain/unreal-ollama)
- OpenAI Compatible Mode: Alibaba Qwen, Mistral, Groq, OpenRouter, Meta Llama, BigModel GLM-4, Ollama [(available in pro)](https://muddyterrain.com/t/genai-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-plugin) ☑️
- Deepseek API Support:
    - Deepseek Chat API ✅
        - `deepseek-chat` (DeepSeek-V3.1) Model ✅
    - Deepseek Reasoning API, R1 ✅
        - `deepseek-reasoning-r1` Model ✅
- Alibaba (Dashscope):
    - `qwen3.5-plus`, `qwen3.5-flash`, `qwen3-max`, `qwen-plus`, `qwen-flash`, `qwen-coder`, `qwen3-coder-plus`, `qwen3-coder-flash`, `qwq-plus` etc [(available in "gen-ai pro china")](https://muddyterrain.com/t/genai-china-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-china-plugin) ☑️ 🇨🇳
    - Multimodal API Support: `qwen-omni-turbo`, `qwen-vl-max`, `qwen3-vl-plus`, `qwen3-vl-flash`, `qvq-max` etc [(available in "gen-ai pro china")](https://muddyterrain.com/t/genai-china-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-china-plugin) ☑️ 🇨🇳
    - Image Generation API: `qwen-image`, `qwen-image-edit`, `wan2.2-t2i-plus` [(available in "gen-ai pro china")](https://muddyterrain.com/t/genai-china-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-china-plugin) ☑️ 🇨🇳
    - Text to Speech: `qwen3-tts-flash`, `qwen-tts` [(available in "gen-ai pro china")](https://muddyterrain.com/t/genai-china-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-china-plugin) ☑️ 🇨🇳
- Moonshot AI:
    - Moonshot Chat API: `kimi-k2.5`, `kimi-k2-thinking`, `kimi-k2-thinking-turbo`, `kimi-k2-0905-preview` etc [(available in "gen-ai pro china")](https://muddyterrain.com/t/genai-china-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-china-plugin) ☑️ 🇨🇳
    - Multimodal Chat API: `kimi-latest`, `kimi-thinking-preview`, `moonshot-v1-8k-vision-preview` [(available in "gen-ai pro china")](https://muddyterrain.com/t/genai-china-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-china-plugin) ☑️ 🇨🇳
- Bytedance:
    - `seed-2-0-mini`, `seed-2-0-lite`, `seed-1-8`, `seed-1-6`, `seed-1-6-flash`, `deepseek-v3-2`, `skylark-pro-250415` etc [(available in "gen-ai pro china")](https://muddyterrain.com/t/genai-china-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-china-plugin) ☑️ 🇨🇳
    - Multimodal API Support: `seed-1-6`, `skylark-vision` [(available in "gen-ai pro china")](https://muddyterrain.com/t/genai-china-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-china-plugin) ☑️ 🇨🇳
    - Image Generation API: `seedream-4-0-250828`, `seedream-3-0-t2i`, `seededit-3-0-i2i` [(available in "gen-ai pro china")](https://muddyterrain.com/t/genai-china-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-china-plugin) ☑️ 🇨🇳
- ZhipuAI:
    - `glm-5`, `glm-4.7`, `glm-4.7-flash`, `glm-4.6`, `glm-4.5`, `glm-4.5-flash` etc [(available in "gen-ai pro china")](https://muddyterrain.com/t/genai-china-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-china-plugin) ☑️ 🇨🇳
    - Multimodal API Support: `glm-4.6v`, `glm-4.6v-flash`, `glm-4.5v` [(available in "gen-ai pro china")](https://muddyterrain.com/t/genai-china-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-china-plugin) ☑️ 🇨🇳
- Baidu:
    - `ernie-5.0-8k`, `ernie-4.5-8k`, `ernie-4.5-turbo-128k`, `ernie-x1-32k`, `ernie-x1-turbo-32k` etc [(available in "gen-ai pro china")](https://muddyterrain.com/t/genai-china-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-china-plugin) ☑️ 🇨🇳
- Meshy AI (3D Generation):
    - `meshy-6` - Text-to-3D (auto preview + refine), Image-to-3D, Retexture, Auto-Rigging [(available in "GenAI Model Generator")](https://muddyterrain.com/t/genai-model-generator-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=gen3d-plugin) ☑️
- Tripo AI (3D Generation):
    - `tripo-v2.5` - Text-to-3D, Image-to-3D [(available in "GenAI Model Generator")](https://muddyterrain.com/t/genai-model-generator-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=gen3d-plugin) ☑️
- Fal.ai - Hunyuan3D (3D Generation):
    - `hunyuan3d-v3.1-pro` (Tencent, open-source) - Text-to-3D [(available in "GenAI Model Generator")](https://muddyterrain.com/t/genai-model-generator-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=gen3d-plugin) ☑️
    - `hunyuan3d-v2.1` (Tencent, open-source) - Image-to-3D [(available in "GenAI Model Generator")](https://muddyterrain.com/t/genai-model-generator-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=gen3d-plugin) ☑️
- Fal.ai - TripoSR (3D Generation):
    - `triposr` (MIT, open-source) - Image-to-3D (fast, <1s) [(available in "GenAI Model Generator")](https://muddyterrain.com/t/genai-model-generator-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=gen3d-plugin) ☑️
- Fal.ai - Rodin (3D Generation):
    - `rodin-gen-2` (Hyper3D, proprietary) - Text-to-3D, Image-to-3D with PBR materials [(available in "GenAI Model Generator")](https://muddyterrain.com/t/genai-model-generator-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=gen3d-plugin) ☑️
- Fal.ai - Trellis 2 (3D Generation):
    - `trellis-2` (Microsoft, open-source) - Image-to-3D with PBR materials [(available in "GenAI Model Generator")](https://muddyterrain.com/t/genai-model-generator-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=gen3d-plugin) ☑️
- Google Gemini Texture Generation:
    - `gemini-3.1-flash` - PBR texture maps (base color, normal, roughness, metallic), reference image generation [(available in "GenAI Model Generator")](https://muddyterrain.com/t/genai-model-generator-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=gen3d-plugin) ☑️
- Plugin Example Project 🛠️ [here](https://github.com/prajwalshettydev/unreal-llm-api-test-project)
- Version Control Support
    - Perforce Support 🛠️
    - Git Submodule Support ✅ 
- Lightweight Plugin (In Builds) 
    - No External Dependencies ✅
    - Build Flags to enable/disable APIs 🚧
    - Exclude MCP from build ✅
- Testing 
    - Automated Testing  [(available in pro)](https://muddyterrain.com/t/genai-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-plugin) ☑️ and [(available in "gen-ai pro china")](https://muddyterrain.com/t/genai-china-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-china-plugin) ☑️ 🇨🇳
    - Different Platforms  [(available in pro)](https://muddyterrain.com/t/genai-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-plugin) ☑️, [(available in "gen-ai pro china")](https://muddyterrain.com/t/genai-china-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-china-plugin) ☑️ 🇨🇳, and [(available in "GenAI Model Generator")](https://muddyterrain.com/t/genai-model-generator-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=gen3d-plugin) ☑️
    - Different Engine Versions  [(available in pro)](https://muddyterrain.com/t/genai-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-plugin) ☑️, [(available in "gen-ai pro china")](https://muddyterrain.com/t/genai-china-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-china-plugin) ☑️ 🇨🇳, and [(available in "GenAI Model Generator")](https://muddyterrain.com/t/genai-model-generator-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=gen3d-plugin) ☑️

### Unreal MCP (Model Control Protocol):

- Clients Support ✅
    - Claude Desktop App Support ✅
    - Claude Code CLI Support ✅
    - Cursor IDE Support ✅
    - OpenAI Operator API Support 🚧
- Blueprints Auto Generation 🛠️
    - Creating new blueprint of types ✅
    - Adding new functions, function/blueprint variables ✅
    - Adding nodes and connections 🛠️ (buggy, issues open)
    - Advanced Blueprints Generation 🛠️
- Level/Scene Control for LLMs 🛠️
    - Spawning Objects and Shapes ✅
    - Moving, rotating and scaling objects ✅
    - Changing materials and color ✅
    - Advanced scene features 🛠️
- Generative AI:
    - Prompt to 3D model fetch and spawn 🛠️ 
- Control:
    - Ability to run Python scripts ✅
    - Ability to run Console Commands ✅
- UI:
    - Widgets generation 🛠️
    - UI Blueprint generation 🛠️
- Project Files:
    - Create/Edit project files/folders ️✅
    - Delete existing project files ❌
- Others:
    - Project Cleanup 🛠️ 

Where,
- ✅ - Completed
- ☑️ (Pro) - [Available in GenAI-Pro](https://muddyterrain.com/t/genai-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-plugin)
- ☑️ 🇨🇳 - [Available in "GenAI Pro China"](https://muddyterrain.com/t/genai-china-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=genai-china-plugin)
- ☑️ (3D) - [Available in "GenAI Model Generator"](https://muddyterrain.com/t/genai-model-generator-fab?utm_source=github.com&utm_medium=repo-free&utm_campaign=gen3d-plugin)
- 🛠️ - In Progress
- 🚧 - Planned
- 🤝 - Need Contributors
- ❌ - Won't Support For Now

## Table of Contents

- [Setting API Keys](#setting-api-keys)
    - [For Editor](#for-editor)
    - [For Packaged Builds](#for-packaged-builds)
- [Setting up MCP](#setting-up-mcp)
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
    - [XAI's Grok 3 API](#xais-grok-3-api)
        - [1. Chat](#1-chat-2)
    - [Model Control Protocol (MCP)](#model-control-protocol-mcp)
- [Known Issues](#known-issues)
- [Config Window](#config-window)
- [Contribution Guidelines](#contribution-guidelines)
    - [Setting up for Development](#setting-up-for-development)
    - [Project Structure](#project-structure)
- [References](#references)

## Setting API Keys:
> [!NOTE]  
> There is no need to set the API key for testing the MCP features in Claude app. Anthropic key only needed for Claude API.

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

## Setting up MCP:

> [!NOTE]  
> If your project only uses the LLM APIs and not the MCP, you can skip this section.

> [!CAUTION]  
> Discalimer: If you are using the MCP feature of the plugin, it will directly let the Claude Desktop App control your Unreal Engine project.
> Make sure you are aware of the security risks and only use it in a controlled environment.
>
> Please backup your project before using the MCP feature and use version control to track changes.


##### 1. Install any one of the below clients:
* Claude Desktop App from [here](https://claude.anthropic.com/).
* Claude Code CLI from [here](https://docs.anthropic.com/en/docs/claude-code).
* Codex CLI from [here](https://developers.openai.com/codex/cli).
* Cursor IDE from [here](https://www.cursor.com/).

##### 2. Setup the MCP config:
###### For Claude Desktop App:
`claude_desktop_config.json` file in Claude Desktop App's installation directory. (might ask claude where its located for your platform!)
The file will look something like this:
```json
{
    "mcpServers": {
      "unreal-handshake": {
        "command": "python",
        "args": ["<your_project_directoy_path>/Plugins/GenerativeAISupport/Content/Python/mcp_server.py"],
        "env": {
          "UNREAL_HOST": "localhost",
          "UNREAL_PORT": "9877" 
        }
      }
    }
}
```
###### For Claude Code CLI:
`.mcp.json` file in your project root directory. The file will look something like this:
```json
{
    "mcpServers": {
      "unreal-handshake": {
        "type": "stdio",
        "command": "python",
        "args": ["<your_project_directoy_path>/Plugins/GenerativeAISupport/Content/Python/mcp_server.py"],
        "env": {
          "UNREAL_HOST": "localhost",
          "UNREAL_PORT": "9877"
        }
      }
    }
}
```
###### For Codex CLI:
Project-scoped `.codex/config.toml` file in your Unreal project directory. The file will look something like this:
```toml
[mcp_servers.unreal-handshake]
command = "python"
args = ["<your_project_directoy_path>/Plugins/GenerativeAISupport/Content/Python/mcp_server.py"]
env = { UNREAL_HOST = "localhost", UNREAL_PORT = "9877" }
```
You can also place this block in `~/.codex/config.toml`, but the plugin's `Setup Codex` button writes the project-scoped config automatically.
###### For Cursor IDE:
`.cursor/mcp.json` file in your project directory. The file will look something like this:
```json
{
    "mcpServers": {
      "unreal-handshake": {
        "command": "python",
        "args": ["<your_project_directoy_path>/Plugins/GenerativeAISupport/Content/Python/mcp_server.py"],
        "env": {
          "UNREAL_HOST": "localhost",
          "UNREAL_PORT": "9877"
        }
      }
    }
}
```
##### 3. Install FastMCP.
```bash
pip install fastmcp
```
##### 4. Enable python plugin in Unreal Engine. (Edit -> Plugins -> Python Editor Script Plugin)

##### 5. [OPTIONAL] Enable AutoStart MCP server on editor open

<img src="Docs/Settings.png" width="782"/>

### Recommended for Codex CLI

If you want new Unreal Engine 5 projects to work with Codex CLI automatically, install this plugin as an engine plugin instead of copying it into each project's `Plugins/` folder.

- Engine-level install: place `GenerativeAISupport` under your Unreal Engine `Engine/Plugins/` directory.
- With the current defaults, the plugin enables the required editor scripting dependencies, defaults the Unreal socket server to auto-start, and bootstraps the preferred Codex MCP config on editor startup.
- When the plugin is installed inside a project, it writes `.codex/config.toml` in that project.
- When the plugin is installed at the engine level, it prefers the global Codex config at `~/.codex/config.toml`.


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
Coming soon, for free, in the Unreal Engine Marketplace.

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

##### Blueprint Example:
<img src="Docs/BpExampleOAIStructuredOp.png" width="782"/>

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

##### Blueprint Example:
<img src="Docs/BpExampleDeepseekChat.png" width="782"/>

### Anthropic API:
Currently the plugin supports Chat from Anthropic API. Both for C++ and Blueprints.
Tested models are `claude-sonnet-4-20250514`, `claude-opus-4-20250514`, `claude-3-7-sonnet-latest`, `claude-3-5-sonnet`, `claude-3-5-haiku-latest`, `claude-3-opus-latest`.

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

##### Blueprint Example:
<img src="Docs/BpExampleClaudeChat.png" width="782"/>

### XAI's Grok 3 API:
Currently the plugin supports Chat from XAI's Grok 3 API. Both for C++ and Blueprints.

#### 1. Chat:
```cpp
	FGenXAIChatSettings ChatSettings;
	ChatSettings.Model = TEXT("grok-3-latest");
		ChatSettings.Messages.Add(FGenXAIMessage{
		TEXT("system"),
		TEXT("You are a helpful AI assistant for a game. Please provide concise responses.")
	});
	ChatSettings.Messages.Add(FGenXAIMessage{TEXT("user"), TEXT("Create a brief description for a forest level in a fantasy game")});
	ChatSettings.MaxTokens = 1000;

	UGenXAIChat::SendChatRequest(
		ChatSettings,
		FOnXAIChatCompletionResponse::CreateLambda(
			[this](const FString& Response, const FString& ErrorMessage, bool bSuccess)
			{
				if (!UTHelper::IsContextStillValid(this))
				{
					return;
				}
				
				UE_LOG(LogTemp, Warning, TEXT("XAI Chat response: %s"), *Response);
				
				if (!bSuccess)
				{
					UE_LOG(LogTemp, Error, TEXT("XAI Chat error: %s"), *ErrorMessage);
				}
			})
	);
```

## Model Control Protocol (MCP):
This is currently work in progress. The plugin supports various clients like Claude Desktop App, Cursor etc.
### Usage:

#### If Autostart MCP server is enabled: (In plugin's settings)
##### 1. Open the Unreal Engine Editor.
##### 2. Open the Claude Desktop App, Claude Code CLI, or Cursor IDE. 

That's it! You can now use the MCP features of the plugin.

#### If Autostart MCP server is disabled:

##### 1. Run the MCP server from the plugin's python directory.
```bash
python <your_project_directoy>/Plugins/GenerativeAISupport/Content/Python/mcp_server.py
```
##### 2. Run the MCP client by opening or restarting Claude Desktop / Cursor, or by launching `codex` from the project after trusting the generated `.codex/config.toml`.

##### 3. Open a new Unreal Engine project and run the below python script from the plugin's python directory.

> Tools -> Run Python Script -> Select the `Plugins/GenerativeAISupport/Content/Python/unreal_socket_server.py` file.

#### 4. Now you should be able to prompt the Claude Desktop App to use Unreal Engine.

## Known Issues:
- Nodes fail to connect properly with MCP
- No undo redo support for MCP
- No streaming support for Deepseek reasoning model
- No complex material generation support for the create material tool
- Issues with running some llm generated valid python scripts
- When LLM compiles a blueprint no proper error handling in its response
- Issues spawning certain nodes, especially with getters and setters
- Doesn't open the right context window during scene and project files edit. 
- Doesn't dock the window properly in the editor for blueprints.

## Config Window:
(Still wip)
<img src="Docs/EditorWindow.png" width="782"/>

## Contribution Guidelines:

### Setting up for Development:

1. Install `unreal` python package and setup the IDE's python interpreter for proper intellisense.
```bash
pip install unreal
```

More details will be added soon.

### Project Structure:

More details will be added soon.

## References:

* Env Var set logic
  from: [OpenAI-Api-Unreal by KellanM](https://github.com/KellanM/OpenAI-Api-Unreal/blob/main/Source/OpenAIAPI/Private/OpenAIUtils.cpp)
* MCP Server inspiration
  from: [Blender-MCP by ahujasid](https://github.com/ahujasid/blender-mcp)


## Quick Links:

- [OpenAI API Documentation](https://platform.openai.com/docs/api-reference)
- [Anthropic API Documentation](https://docs.anthropic.com/en/docs/about-claude/models)
- [XAI API Documentation](https://docs.x.ai/api)
- [Google Gemini API Documentation](https://ai.google.dev/gemini-api/docs/models/gemini)
- [Meta AI API Documentation](https://docs.llama-api.com/quickstart#available-models)
- [Deepseek API Documentation](https://api-docs.deepseek.com/)
- [Model Control Protocol (MCP) Documentation](https://modelcontextprotocol.io/)
- [TripoSt Documentation](https://huggingface.co/stabilityai/TripoSR)

## Support This Project

<iframe src="https://github.com/sponsors/prajwalshettydev/button" title="Sponsor prajwalshettydev" height="32" width="114" style="border: 0; border-radius: 6px;"></iframe>

If you find UnrealGenAISupport helpful, consider sponsoring me to keep the project going! Click the "Sponsor" button above to contribute.
