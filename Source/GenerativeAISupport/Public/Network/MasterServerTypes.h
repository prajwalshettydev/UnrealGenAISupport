// Master Server Types - Data structures for server communication
// Part of GenerativeAISupport Plugin

#pragma once

#include "CoreMinimal.h"
#include "MasterServerTypes.generated.h"

/**
 * Authentication Response
 */
USTRUCT(BlueprintType)
struct GENERATIVEAISUPPORT_API FAuthResponse
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Auth")
	bool bSuccess = false;

	UPROPERTY(BlueprintReadOnly, Category = "Auth")
	FString Error;

	UPROPERTY(BlueprintReadOnly, Category = "Auth")
	FString Token;

	UPROPERTY(BlueprintReadOnly, Category = "Auth")
	FString SessionID;

	UPROPERTY(BlueprintReadOnly, Category = "Auth")
	int32 AccountID = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Auth")
	FString Username;

	UPROPERTY(BlueprintReadOnly, Category = "Auth")
	bool bIsAdmin = false;

	UPROPERTY(BlueprintReadOnly, Category = "Auth")
	bool bIsGM = false;
};

/**
 * Character Data
 */
USTRUCT(BlueprintType)
struct GENERATIVEAISUPPORT_API FCharacterData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Character")
	int32 CharacterID = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Character")
	int32 AccountID = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Character")
	FString Name;

	UPROPERTY(BlueprintReadWrite, Category = "Character")
	FString ClassName;

	UPROPERTY(BlueprintReadWrite, Category = "Character")
	int32 Level = 1;

	UPROPERTY(BlueprintReadWrite, Category = "Character")
	int32 Experience = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Character")
	int32 Gold = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Character")
	FString LocationMap;

	UPROPERTY(BlueprintReadWrite, Category = "Character")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = "Character")
	int32 PlaytimeMinutes = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Character")
	FString CreatedAt;

	UPROPERTY(BlueprintReadWrite, Category = "Character")
	FString LastPlayed;
};

/**
 * Character List Response
 */
USTRUCT(BlueprintType)
struct GENERATIVEAISUPPORT_API FCharacterListResponse
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Character")
	bool bSuccess = false;

	UPROPERTY(BlueprintReadOnly, Category = "Character")
	FString Error;

	UPROPERTY(BlueprintReadOnly, Category = "Character")
	TArray<FCharacterData> Characters;
};

/**
 * Game Server Info
 */
USTRUCT(BlueprintType)
struct GENERATIVEAISUPPORT_API FGameServerInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Server")
	FString ServerID;

	UPROPERTY(BlueprintReadOnly, Category = "Server")
	FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "Server")
	FString Host;

	UPROPERTY(BlueprintReadOnly, Category = "Server")
	int32 Port = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Server")
	FString Status;

	UPROPERTY(BlueprintReadOnly, Category = "Server")
	int32 CurrentPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Server")
	int32 MaxPlayers = 100;

	UPROPERTY(BlueprintReadOnly, Category = "Server")
	FString MapName;

	UPROPERTY(BlueprintReadOnly, Category = "Server")
	FString LastHeartbeat;

	/** Is server online and available? */
	bool IsAvailable() const
	{
		return Status == TEXT("online") && CurrentPlayers < MaxPlayers;
	}
};

/**
 * Server List Response
 */
USTRUCT(BlueprintType)
struct GENERATIVEAISUPPORT_API FServerListResponse
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Server")
	bool bSuccess = false;

	UPROPERTY(BlueprintReadOnly, Category = "Server")
	FString Error;

	UPROPERTY(BlueprintReadOnly, Category = "Server")
	TArray<FGameServerInfo> Servers;
};

/**
 * Generic API Response
 */
USTRUCT(BlueprintType)
struct GENERATIVEAISUPPORT_API FAPIResponse
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "API")
	bool bSuccess = false;

	UPROPERTY(BlueprintReadOnly, Category = "API")
	FString Error;

	UPROPERTY(BlueprintReadOnly, Category = "API")
	FString Data;
};

/**
 * Server Statistics (Admin)
 */
USTRUCT(BlueprintType)
struct GENERATIVEAISUPPORT_API FServerStatistics
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 TotalAccounts = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 TotalCharacters = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 ActiveSessions = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 OnlineServers = 0;
};

// Delegates
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnAuthComplete, const FAuthResponse&, Response);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnCharacterListComplete, const FCharacterListResponse&, Response);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnServerListComplete, const FServerListResponse&, Response);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnAPIComplete, const FAPIResponse&, Response);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConnectionLost, const FString&, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReconnected);
