### Set API Key:

Set the environment variable `PS_OPENAIAPIKEY` to your API key.
In windows you can use[:]()

```cmd
setx PS_OPENAIAPIKEY "your api key"
```

### Adding the plugin to your project:

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

### Fetching the Latest Plugin Changes:

you can pull the latest changes with:

```cmd
cd Plugins/<PLUGIN_NAME>
git pull origin main
```

Or update all submodules in the project:

```cmd
git submodule update --recursive --remote
```

### Usage:

#### OpenAI:

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