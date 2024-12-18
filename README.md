# Unreal Engine Generative AI Support Plugin

Warning: This plugin is in development and is not yet ready for production use.

This plugin is an attempt to build a community ecosystem around the Unreal Engine support for various Generative AI
APIs. Will only focus on the APIs that can be useful for game development and interactive experiences. Any suggestions and contributions are welcome.
Currently working on OpenAI API support with real-time chat/audio completions.

## Current Progress:

- OpenAI API Support:
    - OpenAI Chat API ✅
        - `gpt-4o` Model ✅
        - `gpt-4o-mini` Model ✅
        - `o1-mini` Model 🚧
        - `o1` Model 🚧
    - OpenAI DALL-E API 🛠️
    - OpenAI Vision API 🚧
    - OpenAI Realtime API 🛠️
    - OpenAI Text-To-Speech API 🚧
    - OpenAI Whisper API 🚧
- Anthropic Claude API Support:
    - Claude Chat API 🚧
        - `claude-3-5-sonnet-latest` Model 🚧
        - `claude-3-5-haiku-latest` Model 🚧
        - `claude-3-opus-latest` Model 🚧
    - Claude Vision API 🚧
- XAI (Grok) API Support:
    - XAI Chat Completions API 🚧
        - `grok-beta` Model 🚧
        - `grok-beta` Streaming API 🚧
    - XAI Image API 🚧
- Google Gemini API Support:
    - Gemini Chat API 🚧
        - `gemini-2.0-flash-exp` Model 🚧
        - `gemini-1.5-flash` Model 🚧
        - `gemini-1.5-flash-8b` Model 🚧
    - Gemini Multi-Modal API 🚧
- Meta AI API Support:
    - Llama Chat API 🚧
        - `llama3.3-70b` Model 🚧
        - `llama3.1-8b` Model 🚧
    - Multi-Modal Vision API 🚧
        - `llama3.2-90b-vision` Model 🚧
    - Local Llama API 🚧
- API Key Management ✅
    - Cross-Platform Secure Key Storage ✅
    - Encrypted Key Storage 🛠️
    - Cross Platform Testing 🚧
    - Build System Integration 🛠️
    - Keys in Build Configuration 🛠️
- Plugin Documentation 🛠️
- Plugin Examples 🚧

## Quick Links:

- [OpenAI API Documentation](https://platform.openai.com/docs/api-reference)
- [Anthropic API Documentation](https://docs.anthropic.com/en/docs/about-claude/models)
- [XAI API Documentation](https://docs.x.ai/api)
- [Google Gemini API Documentation](https://ai.google.dev/gemini-api/docs/models/gemini)
- [Meta AI API Documentation](https://docs.llama-api.com/quickstart#available-models)

## Setting API Key:

### For Editor:
Set the environment variable `PS_OPENAIAPIKEY` to your API key.
In windows you can use[:]()

```cmd
setx PS_OPENAIAPIKEY "your api key"
```

### For Packaged Builds:

Still in development..

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

### OpenAI:

Function `GetGenerativeAIApiKey` by default responds with OpenAI API key, that you have securely set in the local
environment variable

1. Chat:

```cpp
	// Get the API key from the plugin
	FString ApiKey = UGenSecureKey::GetGenerativeAIApiKey();

	// Define chat settings for testing
	FGenChatSettings ChatSettings;
	ChatSettings.Model = TEXT("gpt-4o");
	ChatSettings.MaxTokens = 100;
    
	// Add initial user message
	FGenChatMessage UserMessage;
	UserMessage.Role = TEXT("user");
	UserMessage.Content = TEXT("Hello, AI! How are you?");
	ChatSettings.Messages.Add(UserMessage);

	// Create the chat node and initiate the request
	UGenOAIChat* ChatNode = UGenOAIChat::CallOpenAIChat(this, ChatSettings);
	if (ChatNode)
	{
		ChatNode->Finished.AddDynamic(this, &Aunreal_llm_api_testGameMode::OnChatCompletion);
		ChatNode->Activate();
	}
```