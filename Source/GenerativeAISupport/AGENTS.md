<!-- Parent: ../../AGENTS.md -->
<!-- Generated: 2026-04-08 | Updated: 2026-04-08 -->

# GenerativeAISupport Module (C++)

## Purpose
Runtime C++ module providing LLM API clients for Claude (Anthropic), OpenAI, DeepSeek, and XAI. This module is linked into both the editor and packaged game, allowing Blueprint and C++ code to call LLM APIs directly. No gameplay logic — only transport and credential management.

## Key Files
| Category | Files | Purpose |
|----------|-------|---------|
| **Core** | `GenerativeAISupport.h/cpp` | Module interface, startup/shutdown |
| **Vendors** | `Models/Anthropic/GenClaudeChat.*` | Claude API via HTTP |
| | `Models/OpenAI/GenOAIChat.*` | OpenAI API via HTTP |
| | `Models/DeepSeek/GenDSeekChat.*` | DeepSeek API via HTTP |
| | `Models/XAI/GenXAIChat.*` | XAI (Grok) API via HTTP |
| **Data Structs** | `Data/Anthropic/GenClaudeChatStructs.*` | Claude message/response serialization |
| | `Data/OpenAI/GenOAIChatStructs.*` | OpenAI message/response serialization |
| | `Data/XAI/GenXAIChatStructs.*` | XAI message/response serialization |
| **Organization** | `Data/GenAIOrgs.h/cpp` | Enum for vendor selection, org constants |
| **Security** | `Secure/GenSecureKey.h/cpp` | API key storage (encrypted via Unreal credential system) |
| **Utilities** | `Utilities/GenGlobalDefinitions.h/cpp` | Global constants, log categories |
| | `Utilities/GenUtils.h/cpp` | Helper functions (JSON serialization, HTTP utilities) |
| **Structured Output** | `Models/OpenAI/GenOAIStructuredOpService.*` | OpenAI structured output (JSON schema) |

## Architecture

### Module Structure
```
GenerativeAISupport/
├── Public/
│   ├── GenerativeAISupport.h          (module interface)
│   ├── Models/
│   │   ├── Anthropic/GenClaudeChat.h
│   │   ├── OpenAI/GenOAIChat.h
│   │   ├── DeepSeek/GenDSeekChat.h
│   │   └── XAI/GenXAIChat.h
│   ├── Data/
│   │   ├── Anthropic/GenClaudeChatStructs.h
│   │   ├── OpenAI/GenOAIChatStructs.h
│   │   └── GenAIOrgs.h
│   └── Secure/GenSecureKey.h
└── Private/
    ├── Models/
    │   ├── Anthropic/GenClaudeChat.cpp
    │   ├── OpenAI/GenOAIChat.cpp
    │   ├── DeepSeek/GenDSeekChat.cpp
    │   └── XAI/GenXAIChat.cpp
    ├── Data/
    └── Secure/GenSecureKey.cpp
```

### LLM Client Pattern
All vendor clients (Claude, OpenAI, DeepSeek, XAI) follow a common interface:
1. **Constructor**: Initialize with org name and API key
2. **SendMessage**: Queue a message (async, returns handle)
3. **GetResponse**: Poll handle for response (blocking or callback-based)
4. **OnResponseReceived**: Delegate fired when async response arrives

### Credential Management
- API keys stored via Unreal's `ISecureCredentialsManager`
- Keys are encrypted at rest
- Accessed via `UGenSecureKey::GetApiKey(org_name)`
- Never logged or exposed in editor UI

## API Clients

### Claude Client (`GenClaudeChat`)
**Endpoint**: https://api.anthropic.com/v1/messages

**Features**:
- Streaming via Server-Sent Events (SSE)
- Vision support (image input)
- Extended thinking (claude-opus)
- Token counting

**Response Struct**: `FGenClaudeChatMessage`, `FGenClaudeChatResponse`

### OpenAI Client (`GenOAIChat`)
**Endpoint**: https://api.openai.com/v1/chat/completions

**Features**:
- Function calling (tool_use)
- JSON mode (structured output)
- Vision (gpt-4-vision)

**Response Struct**: `FGenOAIChatMessage`, `FGenOAIChatResponse`

**Special**: `GenOAIStructuredOpService` for JSON schema validation and repair

### DeepSeek Client (`GenDSeekChat`)
**Endpoint**: https://api.deepseek.com/v1/chat/completions

**Features**:
- Compatible with OpenAI API format
- Cost-effective alternative for batch inference

**Response Struct**: (extends OpenAI structs)

### XAI Client (`GenXAIChat`)
**Endpoint**: https://api.x.ai/v1/chat/completions

**Features**:
- Grok model (general-purpose)
- OpenAI API compatible

**Response Struct**: (extends OpenAI structs)

## Message Serialization

### Claude Format
```json
{
  "model": "claude-3-5-sonnet-20241022",
  "max_tokens": 1024,
  "messages": [
    {
      "role": "user",
      "content": [
        { "type": "text", "text": "..." },
        { "type": "image", "source": { "type": "base64", "media_type": "image/png", "data": "..." } }
      ]
    }
  ]
}
```

### OpenAI Format
```json
{
  "model": "gpt-4-turbo",
  "messages": [
    { "role": "user", "content": "..." }
  ],
  "tools": [
    {
      "type": "function",
      "function": { "name": "...", "parameters": {...} }
    }
  ]
}
```

## For AI Agents

### Working In This Directory
- **Do NOT modify** vendor API structs without consulting API documentation.
- Add new vendor by inheriting from base chat client pattern and implementing `SendMessage`/`GetResponse`.
- All HTTP calls use `FHttpModule` — async by default, use `FHttpManager` for blocking operations in editor.
- Credential lookups happen in `UGenSecureKey` — add new org types to `EGenAIOrgType` enum.

### Testing Requirements
- Unit test each vendor client with mocked HTTP (use `FakeHttp` or similar).
- Integration test with real API keys (CI environment variable gated).
- Verify message serialization matches vendor API schema exactly.
- Test error handling: network failures, auth errors, rate limiting, malformed responses.
- Test streaming (Claude) with large responses (> 10MB tokens).

### Common Patterns
- **Use delegates for async responses**: `FOnLLMResponseReceived.BindDynamic(this, &Handler::OnResponse);`
- **Store API keys in secure storage**: Never hardcode or log keys.
- **Validate all JSON responses** before deserializing — vendor APIs can return unexpected formats.
- **Implement exponential backoff** for retries on transient failures.

### Dependencies
- `CoreMinimal.h` — Unreal core types
- `Http/Public/HttpModule.h` — HTTP client
- `Json/Public/Json.h` — JSON serialization
- `Interfaces/IHttpRequest.h` — async HTTP interface
- `GenAIOrgs.h` — vendor enum and constants

## Data Flow

```
Blueprint / C++ Code
    ↓
GenClaudeChat::SendMessage() / GenOAIChat::SendMessage() / etc.
    ↓
Message serialization (GenClaudeChatStructs / GenOAIChatStructs)
    ↓
FHttpModule (async HTTP POST)
    ↓
Vendor API (https://api.anthropic.com / https://api.openai.com / etc.)
    ↓
Response deserialization
    ↓
FOnLLMResponseReceived delegate fire / GetResponse() poll
    ↓
Caller receives response
```

<!-- MANUAL: -->
