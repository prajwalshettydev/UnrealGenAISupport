// Copyright (c) 2026. Licensed under the MIT License.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Models/MiniMax/GenMiniMaxChat.h"
#include "Interfaces/IHttpRequest.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenMiniMaxRequestTest, "GenerativeAISupport.MiniMax.Request",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FGenMiniMaxRequestTest::RunTest(const FString& Parameters)
{
	static_assert(static_cast<uint8>(EMiniMaxModels::MiniMax_M3) == 0, "Preserve saved M3 selections");
	static_assert(static_cast<uint8>(EMiniMaxModels::MiniMax_M2_7) == 1, "Preserve saved legacy selections");
	static_assert(static_cast<uint8>(EMiniMaxModels::Unknown) == 2, "Preserve the unknown sentinel");

	FGenMiniMaxChatSettings Settings;
	TestTrue(TEXT("New settings default to Flash"), Settings.Model == EMiniMaxModels::MiniMax_M3_1_Flash);
	Settings.MaxTokens = 4096;
	FGenMiniMaxMessage SystemMessage;
	SystemMessage.Role = TEXT("system");
	SystemMessage.Content = TEXT("Preserve the conversation.");
	Settings.Messages.Add(SystemMessage);
	FGenMiniMaxMessage UserMessage;
	UserMessage.Role = TEXT("user");
	UserMessage.Content = TEXT("Quoted \"text\"\nsecond line");
	Settings.Messages.Add(UserMessage);

	const EMiniMaxModels Models[] = {Settings.Model, static_cast<EMiniMaxModels>(0),
		static_cast<EMiniMaxModels>(1), static_cast<EMiniMaxModels>(2)};
	const TCHAR* ExpectedIds[] = {TEXT("MiniMax-M3.1-Flash"), TEXT("MiniMax-M3"),
		TEXT("MiniMax-M2.7"), TEXT("unknown")};

	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Models); ++Index)
	{
		Settings.Model = Models[Index];
		// This is the same populated HTTP request MakeRequest submits. Never call ProcessRequest here.
		const auto Request = UGenMiniMaxChat::CreateHttpRequest(Settings, TEXT("automation-placeholder"));
		TestEqual(TEXT("Method"), Request->GetVerb(), FString(TEXT("POST")));
		TestEqual(TEXT("Endpoint"), Request->GetURL(), FString(TEXT("https://api.minimax.io/v1/chat/completions")));
		TestEqual(TEXT("Content type"), Request->GetHeader(TEXT("Content-Type")), FString(TEXT("application/json")));
		TestEqual(TEXT("Authorization"), Request->GetHeader(TEXT("Authorization")), FString(TEXT("Bearer automation-placeholder")));

		const TArray<uint8>& Bytes = Request->GetContent();
		const FUTF8ToTCHAR Converted(reinterpret_cast<const ANSICHAR*>(Bytes.GetData()), Bytes.Num());
		const FString PayloadString(Converted.Length(), Converted.Get());
		TSharedPtr<FJsonObject> Payload;
		if (!TestTrue(TEXT("Serialized HTTP body is JSON"),
			FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(PayloadString), Payload) && Payload.IsValid()))
		{
			return false;
		}
		TestEqual(TEXT("Exact wire model ID"), Payload->GetStringField(TEXT("model")), FString(ExpectedIds[Index]));
		TestEqual(TEXT("Explicit output budget"), Payload->GetNumberField(TEXT("max_tokens")), 4096.0);
		const auto& Messages = Payload->GetArrayField(TEXT("messages"));
		if (TestEqual(TEXT("History is preserved"), Messages.Num(), 2))
		{
			TestEqual(TEXT("System role"), Messages[0]->AsObject()->GetStringField(TEXT("role")), SystemMessage.Role);
			TestEqual(TEXT("System content"), Messages[0]->AsObject()->GetStringField(TEXT("content")), SystemMessage.Content);
			TestEqual(TEXT("User role"), Messages[1]->AsObject()->GetStringField(TEXT("role")), UserMessage.Role);
			TestEqual(TEXT("User content"), Messages[1]->AsObject()->GetStringField(TEXT("content")), UserMessage.Content);
		}
		TestFalse(TEXT("Use API default thinking"), Payload->HasField(TEXT("thinking")));
		TestFalse(TEXT("Use API default effort"), Payload->HasField(TEXT("reasoning_effort")));
		TestFalse(TEXT("No unsupported service tier"), Payload->HasField(TEXT("service_tier")));
		TestFalse(TEXT("No reasoning split override"), Payload->HasField(TEXT("reasoning_split")));
	}

#if WITH_EDITOR
	const UEnum* ModelsEnum = StaticEnum<EMiniMaxModels>();
	TArray<FString> Choices;
	for (int32 Index = 0; Index < ModelsEnum->NumEnums() - 1; ++Index)
	{
		if (!ModelsEnum->HasMetaData(TEXT("Hidden"), Index))
		{
			Choices.Add(ModelsEnum->GetDisplayNameTextByIndex(Index).ToString());
		}
	}
	if (TestEqual(TEXT("Only two ordinary choices"), Choices.Num(), 2))
	{
		TestEqual(TEXT("Flash is first"), Choices[0], FString(TEXT("MiniMax-M3.1-Flash")));
		TestEqual(TEXT("M3 remains available"), Choices[1], FString(TEXT("MiniMax-M3")));
	}
#endif
	return true;
}

#endif
