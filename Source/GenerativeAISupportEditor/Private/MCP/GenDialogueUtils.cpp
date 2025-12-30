// Copyright (c) 2025 Prajwal Shetty. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the root directory of this
// source tree or http://opensource.org/licenses/MIT.

#include "MCP/GenDialogueUtils.h"
#include "NPC/NPCDialogComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Editor.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

AActor* UGenDialogueUtils::FindActorByName(const FString& ActorName)
{
	if (!GEditor) return nullptr;

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World) return nullptr;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor && Actor->GetActorLabel() == ActorName)
		{
			return Actor;
		}
	}

	return nullptr;
}

UNPCDialogComponent* UGenDialogueUtils::FindDialogComponent(const FString& ActorName)
{
	AActor* Actor = FindActorByName(ActorName);
	if (!Actor) return nullptr;

	return Actor->FindComponentByClass<UNPCDialogComponent>();
}

ENPCMood UGenDialogueUtils::StringToMood(const FString& MoodString)
{
	if (MoodString.Equals(TEXT("Happy"), ESearchCase::IgnoreCase)) return ENPCMood::Happy;
	if (MoodString.Equals(TEXT("Sad"), ESearchCase::IgnoreCase)) return ENPCMood::Sad;
	if (MoodString.Equals(TEXT("Angry"), ESearchCase::IgnoreCase)) return ENPCMood::Angry;
	if (MoodString.Equals(TEXT("Surprised"), ESearchCase::IgnoreCase)) return ENPCMood::Surprised;
	if (MoodString.Equals(TEXT("Thoughtful"), ESearchCase::IgnoreCase)) return ENPCMood::Thoughtful;
	if (MoodString.Equals(TEXT("Afraid"), ESearchCase::IgnoreCase)) return ENPCMood::Afraid;
	if (MoodString.Equals(TEXT("Suspicious"), ESearchCase::IgnoreCase)) return ENPCMood::Suspicious;
	return ENPCMood::Neutral;
}

FString UGenDialogueUtils::MoodToString(ENPCMood Mood)
{
	switch (Mood)
	{
	case ENPCMood::Happy: return TEXT("Happy");
	case ENPCMood::Sad: return TEXT("Sad");
	case ENPCMood::Angry: return TEXT("Angry");
	case ENPCMood::Surprised: return TEXT("Surprised");
	case ENPCMood::Thoughtful: return TEXT("Thoughtful");
	case ENPCMood::Afraid: return TEXT("Afraid");
	case ENPCMood::Suspicious: return TEXT("Suspicious");
	default: return TEXT("Neutral");
	}
}

FString UGenDialogueUtils::ListDialogActors()
{
	if (!GEditor)
	{
		return TEXT("{\"success\": false, \"error\": \"GEditor not available\"}");
	}

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return TEXT("{\"success\": false, \"error\": \"No world found\"}");
	}

	TArray<TSharedPtr<FJsonValue>> ActorsArray;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor) continue;

		UNPCDialogComponent* DialogComp = Actor->FindComponentByClass<UNPCDialogComponent>();
		if (!DialogComp) continue;

		TSharedPtr<FJsonObject> ActorJson = MakeShareable(new FJsonObject);
		ActorJson->SetStringField("actor_name", Actor->GetActorLabel());
		ActorJson->SetStringField("npc_id", DialogComp->NPCID);
		ActorJson->SetStringField("display_name", DialogComp->DisplayName.ToString());
		ActorJson->SetStringField("title", DialogComp->Title.ToString());
		ActorJson->SetBoolField("can_trade", DialogComp->bCanTrade);
		ActorJson->SetStringField("shop_id", DialogComp->ShopID);
		ActorJson->SetNumberField("quick_options_count", DialogComp->QuickOptions.Num());

		FVector Loc = Actor->GetActorLocation();
		ActorJson->SetStringField("location", FString::Printf(TEXT("(%f, %f, %f)"), Loc.X, Loc.Y, Loc.Z));

		ActorsArray.Add(MakeShareable(new FJsonValueObject(ActorJson)));
	}

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetNumberField("count", ActorsArray.Num());
	ResultJson->SetArrayField("dialog_actors", ActorsArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}

FString UGenDialogueUtils::GetDialogConfig(const FString& ActorName)
{
	UNPCDialogComponent* DialogComp = FindDialogComponent(ActorName);
	if (!DialogComp)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Actor '%s' not found or has no dialog component\"}"), *ActorName);
	}

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetStringField("actor_name", ActorName);
	ResultJson->SetStringField("npc_id", DialogComp->NPCID);
	ResultJson->SetStringField("display_name", DialogComp->DisplayName.ToString());
	ResultJson->SetStringField("title", DialogComp->Title.ToString());
	ResultJson->SetStringField("portrait_path", DialogComp->PortraitPath);
	ResultJson->SetStringField("faction_id", DialogComp->FactionID);
	ResultJson->SetStringField("shop_id", DialogComp->ShopID);
	ResultJson->SetBoolField("can_trade", DialogComp->bCanTrade);
	ResultJson->SetStringField("greeting", DialogComp->GreetingMessage.ToString());
	ResultJson->SetStringField("farewell", DialogComp->FarewellMessage.ToString());
	ResultJson->SetNumberField("interaction_range", DialogComp->InteractionRange);
	ResultJson->SetNumberField("max_history_length", DialogComp->MaxHistoryLength);
	ResultJson->SetNumberField("quick_options_count", DialogComp->QuickOptions.Num());

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}

FString UGenDialogueUtils::SetDialogConfig(const FString& ActorName, const FString& NPCID,
										   const FString& DisplayName, const FString& Title)
{
	UNPCDialogComponent* DialogComp = FindDialogComponent(ActorName);
	if (!DialogComp)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Actor '%s' not found or has no dialog component\"}"), *ActorName);
	}

	DialogComp->NPCID = NPCID;
	DialogComp->DisplayName = FText::FromString(DisplayName);
	DialogComp->Title = FText::FromString(Title);
	DialogComp->MarkPackageDirty();

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Dialog config updated for '%s'\"}"), *ActorName);
}

FString UGenDialogueUtils::AddDialogComponent(const FString& ActorName, const FString& NPCID, const FString& DisplayName)
{
	AActor* Actor = FindActorByName(ActorName);
	if (!Actor)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Actor '%s' not found\"}"), *ActorName);
	}

	// Check if already has component
	if (Actor->FindComponentByClass<UNPCDialogComponent>())
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Actor '%s' already has a dialog component\"}"), *ActorName);
	}

	UNPCDialogComponent* NewComp = NewObject<UNPCDialogComponent>(Actor, UNPCDialogComponent::StaticClass(), NAME_None, RF_Transactional);
	if (!NewComp)
	{
		return TEXT("{\"success\": false, \"error\": \"Failed to create dialog component\"}");
	}

	NewComp->NPCID = NPCID;
	NewComp->DisplayName = FText::FromString(DisplayName);
	NewComp->RegisterComponent();
	Actor->AddInstanceComponent(NewComp);
	Actor->MarkPackageDirty();

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Dialog component added to '%s'\"}"), *ActorName);
}

FString UGenDialogueUtils::RemoveDialogComponent(const FString& ActorName)
{
	AActor* Actor = FindActorByName(ActorName);
	if (!Actor)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Actor '%s' not found\"}"), *ActorName);
	}

	UNPCDialogComponent* DialogComp = Actor->FindComponentByClass<UNPCDialogComponent>();
	if (!DialogComp)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Actor '%s' has no dialog component\"}"), *ActorName);
	}

	DialogComp->DestroyComponent();
	Actor->MarkPackageDirty();

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Dialog component removed from '%s'\"}"), *ActorName);
}

FString UGenDialogueUtils::SetGreeting(const FString& ActorName, const FString& GreetingText)
{
	UNPCDialogComponent* DialogComp = FindDialogComponent(ActorName);
	if (!DialogComp)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Actor '%s' not found or has no dialog component\"}"), *ActorName);
	}

	DialogComp->GreetingMessage = FText::FromString(GreetingText);
	DialogComp->MarkPackageDirty();

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Greeting set for '%s'\"}"), *ActorName);
}

FString UGenDialogueUtils::SetFarewell(const FString& ActorName, const FString& FarewellText)
{
	UNPCDialogComponent* DialogComp = FindDialogComponent(ActorName);
	if (!DialogComp)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Actor '%s' not found or has no dialog component\"}"), *ActorName);
	}

	DialogComp->FarewellMessage = FText::FromString(FarewellText);
	DialogComp->MarkPackageDirty();

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Farewell set for '%s'\"}"), *ActorName);
}

FString UGenDialogueUtils::GetQuickOptions(const FString& ActorName)
{
	UNPCDialogComponent* DialogComp = FindDialogComponent(ActorName);
	if (!DialogComp)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Actor '%s' not found or has no dialog component\"}"), *ActorName);
	}

	TArray<TSharedPtr<FJsonValue>> OptionsArray;

	for (int32 i = 0; i < DialogComp->QuickOptions.Num(); i++)
	{
		const FQuickDialogOption& Option = DialogComp->QuickOptions[i];

		TSharedPtr<FJsonObject> OptionJson = MakeShareable(new FJsonObject);
		OptionJson->SetNumberField("index", i);
		OptionJson->SetStringField("display_text", Option.DisplayText.ToString());
		OptionJson->SetStringField("message", Option.Message);
		OptionJson->SetStringField("icon_path", Option.IconPath);
		OptionJson->SetNumberField("hotkey", Option.Hotkey);

		OptionsArray.Add(MakeShareable(new FJsonValueObject(OptionJson)));
	}

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetStringField("actor_name", ActorName);
	ResultJson->SetNumberField("count", OptionsArray.Num());
	ResultJson->SetArrayField("quick_options", OptionsArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}

FString UGenDialogueUtils::AddQuickOption(const FString& ActorName, const FString& DisplayText,
										  const FString& Message, int32 Hotkey)
{
	UNPCDialogComponent* DialogComp = FindDialogComponent(ActorName);
	if (!DialogComp)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Actor '%s' not found or has no dialog component\"}"), *ActorName);
	}

	FQuickDialogOption NewOption;
	NewOption.DisplayText = FText::FromString(DisplayText);
	NewOption.Message = Message;
	NewOption.Hotkey = FMath::Clamp(Hotkey, 0, 9);

	DialogComp->QuickOptions.Add(NewOption);
	DialogComp->MarkPackageDirty();

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Quick option added to '%s', total: %d\"}"),
		*ActorName, DialogComp->QuickOptions.Num());
}

FString UGenDialogueUtils::ClearQuickOptions(const FString& ActorName)
{
	UNPCDialogComponent* DialogComp = FindDialogComponent(ActorName);
	if (!DialogComp)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Actor '%s' not found or has no dialog component\"}"), *ActorName);
	}

	int32 Count = DialogComp->QuickOptions.Num();
	DialogComp->QuickOptions.Empty();
	DialogComp->MarkPackageDirty();

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Cleared %d quick options from '%s'\"}"), Count, *ActorName);
}

FString UGenDialogueUtils::SetTradeConfig(const FString& ActorName, bool bCanTrade, const FString& ShopID)
{
	UNPCDialogComponent* DialogComp = FindDialogComponent(ActorName);
	if (!DialogComp)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Actor '%s' not found or has no dialog component\"}"), *ActorName);
	}

	DialogComp->bCanTrade = bCanTrade;
	DialogComp->ShopID = ShopID;
	DialogComp->MarkPackageDirty();

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Trade config updated for '%s': canTrade=%s, shopID=%s\"}"),
		*ActorName, bCanTrade ? TEXT("true") : TEXT("false"), *ShopID);
}

FString UGenDialogueUtils::SetMood(const FString& ActorName, const FString& MoodName)
{
	UNPCDialogComponent* DialogComp = FindDialogComponent(ActorName);
	if (!DialogComp)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Actor '%s' not found or has no dialog component\"}"), *ActorName);
	}

	ENPCMood NewMood = StringToMood(MoodName);
	DialogComp->SetMood(NewMood);

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Mood set to '%s' for '%s'\"}"),
		*MoodToString(NewMood), *ActorName);
}

FString UGenDialogueUtils::SetReputation(const FString& ActorName, int32 Reputation)
{
	UNPCDialogComponent* DialogComp = FindDialogComponent(ActorName);
	if (!DialogComp)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Actor '%s' not found or has no dialog component\"}"), *ActorName);
	}

	int32 ClampedRep = FMath::Clamp(Reputation, -1000, 1000);
	// Since we don't have direct access to set reputation, we calculate the delta
	int32 CurrentRep = DialogComp->GetReputation();
	int32 Delta = ClampedRep - CurrentRep;
	DialogComp->ModifyReputation(Delta);

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Reputation set to %d for '%s'\"}"),
		ClampedRep, *ActorName);
}

FString UGenDialogueUtils::SetPortrait(const FString& ActorName, const FString& PortraitPath)
{
	UNPCDialogComponent* DialogComp = FindDialogComponent(ActorName);
	if (!DialogComp)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Actor '%s' not found or has no dialog component\"}"), *ActorName);
	}

	DialogComp->PortraitPath = PortraitPath;
	DialogComp->MarkPackageDirty();

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Portrait path set for '%s'\"}"), *ActorName);
}

FString UGenDialogueUtils::SetFaction(const FString& ActorName, const FString& FactionID)
{
	UNPCDialogComponent* DialogComp = FindDialogComponent(ActorName);
	if (!DialogComp)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Actor '%s' not found or has no dialog component\"}"), *ActorName);
	}

	DialogComp->FactionID = FactionID;
	DialogComp->MarkPackageDirty();

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Faction set to '%s' for '%s'\"}"), *FactionID, *ActorName);
}

FString UGenDialogueUtils::StartDialog(const FString& ActorName)
{
	// This only works during PIE
	UWorld* PIEWorld = nullptr;

	if (GEditor)
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.WorldType == EWorldType::PIE)
			{
				PIEWorld = Context.World();
				break;
			}
		}
	}

	if (!PIEWorld)
	{
		return TEXT("{\"success\": false, \"error\": \"No PIE world found. Start Play-In-Editor first.\"}");
	}

	// Find actor in PIE world
	AActor* Actor = nullptr;
	for (TActorIterator<AActor> It(PIEWorld); It; ++It)
	{
		if ((*It)->GetActorLabel() == ActorName)
		{
			Actor = *It;
			break;
		}
	}

	if (!Actor)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Actor '%s' not found in PIE world\"}"), *ActorName);
	}

	UNPCDialogComponent* DialogComp = Actor->FindComponentByClass<UNPCDialogComponent>();
	if (!DialogComp)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Actor '%s' has no dialog component\"}"), *ActorName);
	}

	// Find player
	APlayerController* PC = PIEWorld->GetFirstPlayerController();
	if (!PC || !PC->GetPawn())
	{
		return TEXT("{\"success\": false, \"error\": \"No player found in PIE\"}");
	}

	bool bStarted = DialogComp->StartDialog(PC->GetPawn());

	if (bStarted)
	{
		return FString::Printf(TEXT("{\"success\": true, \"message\": \"Dialog started with '%s'\"}"), *ActorName);
	}

	return FString::Printf(TEXT("{\"success\": false, \"error\": \"Failed to start dialog with '%s'\"}"), *ActorName);
}

FString UGenDialogueUtils::EndDialog(const FString& ActorName)
{
	// Similar PIE world lookup
	UWorld* PIEWorld = nullptr;

	if (GEditor)
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.WorldType == EWorldType::PIE)
			{
				PIEWorld = Context.World();
				break;
			}
		}
	}

	if (!PIEWorld)
	{
		return TEXT("{\"success\": false, \"error\": \"No PIE world found\"}");
	}

	AActor* Actor = nullptr;
	for (TActorIterator<AActor> It(PIEWorld); It; ++It)
	{
		if ((*It)->GetActorLabel() == ActorName)
		{
			Actor = *It;
			break;
		}
	}

	if (!Actor)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Actor '%s' not found\"}"), *ActorName);
	}

	UNPCDialogComponent* DialogComp = Actor->FindComponentByClass<UNPCDialogComponent>();
	if (!DialogComp)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Actor '%s' has no dialog component\"}"), *ActorName);
	}

	DialogComp->EndDialog();

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Dialog ended with '%s'\"}"), *ActorName);
}

FString UGenDialogueUtils::SendMessage(const FString& ActorName, const FString& Message)
{
	// Similar PIE world lookup
	UWorld* PIEWorld = nullptr;

	if (GEditor)
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.WorldType == EWorldType::PIE)
			{
				PIEWorld = Context.World();
				break;
			}
		}
	}

	if (!PIEWorld)
	{
		return TEXT("{\"success\": false, \"error\": \"No PIE world found\"}");
	}

	AActor* Actor = nullptr;
	for (TActorIterator<AActor> It(PIEWorld); It; ++It)
	{
		if ((*It)->GetActorLabel() == ActorName)
		{
			Actor = *It;
			break;
		}
	}

	if (!Actor)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Actor '%s' not found\"}"), *ActorName);
	}

	UNPCDialogComponent* DialogComp = Actor->FindComponentByClass<UNPCDialogComponent>();
	if (!DialogComp)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Actor '%s' has no dialog component\"}"), *ActorName);
	}

	if (!DialogComp->IsDialogActive())
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Dialog not active with '%s'\"}"), *ActorName);
	}

	DialogComp->SendMessageAsync(Message);

	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Message sent to '%s'\"}"), *ActorName);
}

FString UGenDialogueUtils::ExportAllDialogConfigs()
{
	if (!GEditor)
	{
		return TEXT("{\"success\": false, \"error\": \"GEditor not available\"}");
	}

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return TEXT("{\"success\": false, \"error\": \"No world found\"}");
	}

	TArray<TSharedPtr<FJsonValue>> ConfigsArray;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor) continue;

		UNPCDialogComponent* DialogComp = Actor->FindComponentByClass<UNPCDialogComponent>();
		if (!DialogComp) continue;

		TSharedPtr<FJsonObject> ConfigJson = MakeShareable(new FJsonObject);
		ConfigJson->SetStringField("actor_name", Actor->GetActorLabel());
		ConfigJson->SetStringField("npc_id", DialogComp->NPCID);
		ConfigJson->SetStringField("display_name", DialogComp->DisplayName.ToString());
		ConfigJson->SetStringField("title", DialogComp->Title.ToString());
		ConfigJson->SetStringField("portrait_path", DialogComp->PortraitPath);
		ConfigJson->SetStringField("faction_id", DialogComp->FactionID);
		ConfigJson->SetStringField("shop_id", DialogComp->ShopID);
		ConfigJson->SetBoolField("can_trade", DialogComp->bCanTrade);
		ConfigJson->SetStringField("greeting", DialogComp->GreetingMessage.ToString());
		ConfigJson->SetStringField("farewell", DialogComp->FarewellMessage.ToString());
		ConfigJson->SetNumberField("interaction_range", DialogComp->InteractionRange);

		// Quick options
		TArray<TSharedPtr<FJsonValue>> OptionsArray;
		for (const FQuickDialogOption& Option : DialogComp->QuickOptions)
		{
			TSharedPtr<FJsonObject> OptionJson = MakeShareable(new FJsonObject);
			OptionJson->SetStringField("display_text", Option.DisplayText.ToString());
			OptionJson->SetStringField("message", Option.Message);
			OptionJson->SetNumberField("hotkey", Option.Hotkey);
			OptionsArray.Add(MakeShareable(new FJsonValueObject(OptionJson)));
		}
		ConfigJson->SetArrayField("quick_options", OptionsArray);

		// Location
		FVector Loc = Actor->GetActorLocation();
		TSharedPtr<FJsonObject> LocJson = MakeShareable(new FJsonObject);
		LocJson->SetNumberField("x", Loc.X);
		LocJson->SetNumberField("y", Loc.Y);
		LocJson->SetNumberField("z", Loc.Z);
		ConfigJson->SetObjectField("location", LocJson);

		ConfigsArray.Add(MakeShareable(new FJsonValueObject(ConfigJson)));
	}

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetNumberField("count", ConfigsArray.Num());
	ResultJson->SetArrayField("configs", ConfigsArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}

FString UGenDialogueUtils::ListMoods()
{
	TArray<TSharedPtr<FJsonValue>> MoodsArray;

	TArray<FString> Moods = {
		TEXT("Neutral"), TEXT("Happy"), TEXT("Sad"), TEXT("Angry"),
		TEXT("Surprised"), TEXT("Thoughtful"), TEXT("Afraid"), TEXT("Suspicious")
	};

	for (const FString& Mood : Moods)
	{
		MoodsArray.Add(MakeShareable(new FJsonValueString(Mood)));
	}

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetArrayField("moods", MoodsArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}

FString UGenDialogueUtils::ListChoiceTypes()
{
	TArray<TSharedPtr<FJsonValue>> TypesArray;

	TArray<FString> Types = {
		TEXT("Normal"), TEXT("Trade"), TEXT("Quest"), TEXT("Threaten"),
		TEXT("Persuade"), TEXT("Goodbye"), TEXT("Back")
	};

	for (const FString& Type : Types)
	{
		TypesArray.Add(MakeShareable(new FJsonValueString(Type)));
	}

	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetBoolField("success", true);
	ResultJson->SetArrayField("choice_types", TypesArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return OutputString;
}
