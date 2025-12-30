// Master Server Client - HTTP client for Master Server communication
// Part of GenerativeAISupport Plugin

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "MasterServerTypes.h"
#include "MasterServerClient.generated.h"

/**
 * Master Server Client Subsystem
 * Handles all communication with the Master Server
 */
UCLASS()
class GENERATIVEAISUPPORT_API UMasterServerClient : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// ============================================
	// LIFECYCLE
	// ============================================

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Get the singleton instance */
	UFUNCTION(BlueprintPure, Category = "Master Server", meta = (WorldContext = "WorldContextObject"))
	static UMasterServerClient* Get(UObject* WorldContextObject);

	// ============================================
	// CONFIGURATION
	// ============================================

	/** Set the Master Server URL */
	UFUNCTION(BlueprintCallable, Category = "Master Server|Config")
	void SetServerURL(const FString& URL);

	/** Get the current server URL */
	UFUNCTION(BlueprintPure, Category = "Master Server|Config")
	FString GetServerURL() const { return ServerURL; }

	/** Check if connected to server */
	UFUNCTION(BlueprintPure, Category = "Master Server|Config")
	bool IsConnected() const { return bIsConnected; }

	/** Check if authenticated */
	UFUNCTION(BlueprintPure, Category = "Master Server|Config")
	bool IsAuthenticated() const { return !SessionID.IsEmpty(); }

	// ============================================
	// AUTHENTICATION
	// ============================================

	/** Register a new account */
	UFUNCTION(BlueprintCallable, Category = "Master Server|Auth")
	void Register(const FString& Username, const FString& Email, const FString& Password, FOnAuthComplete OnComplete);

	/** Login to existing account */
	UFUNCTION(BlueprintCallable, Category = "Master Server|Auth")
	void Login(const FString& Username, const FString& Password, FOnAuthComplete OnComplete);

	/** Logout current session */
	UFUNCTION(BlueprintCallable, Category = "Master Server|Auth")
	void Logout(FOnAPIComplete OnComplete);

	/** Validate current session */
	UFUNCTION(BlueprintCallable, Category = "Master Server|Auth")
	void ValidateSession(FOnAuthComplete OnComplete);

	/** Get current session ID */
	UFUNCTION(BlueprintPure, Category = "Master Server|Auth")
	FString GetSessionID() const { return SessionID; }

	/** Get current username */
	UFUNCTION(BlueprintPure, Category = "Master Server|Auth")
	FString GetUsername() const { return CurrentUsername; }

	/** Get current account ID */
	UFUNCTION(BlueprintPure, Category = "Master Server|Auth")
	int32 GetAccountID() const { return CurrentAccountID; }

	/** Is current user admin? */
	UFUNCTION(BlueprintPure, Category = "Master Server|Auth")
	bool IsAdmin() const { return bIsAdmin; }

	/** Is current user GM? */
	UFUNCTION(BlueprintPure, Category = "Master Server|Auth")
	bool IsGM() const { return bIsGM; }

	// ============================================
	// CHARACTERS
	// ============================================

	/** Get character list for current account */
	UFUNCTION(BlueprintCallable, Category = "Master Server|Characters")
	void GetCharacters(FOnCharacterListComplete OnComplete);

	/** Create a new character */
	UFUNCTION(BlueprintCallable, Category = "Master Server|Characters")
	void CreateCharacter(const FString& Name, const FString& ClassName, FOnAPIComplete OnComplete);

	/** Update character data */
	UFUNCTION(BlueprintCallable, Category = "Master Server|Characters")
	void UpdateCharacter(int32 CharacterID, const FCharacterData& Data, FOnAPIComplete OnComplete);

	/** Delete a character */
	UFUNCTION(BlueprintCallable, Category = "Master Server|Characters")
	void DeleteCharacter(int32 CharacterID, FOnAPIComplete OnComplete);

	/** Select character for current session */
	UFUNCTION(BlueprintCallable, Category = "Master Server|Characters")
	void SelectCharacter(int32 CharacterID, FOnAPIComplete OnComplete);

	/** Get currently selected character ID */
	UFUNCTION(BlueprintPure, Category = "Master Server|Characters")
	int32 GetSelectedCharacterID() const { return SelectedCharacterID; }

	// ============================================
	// SERVERS
	// ============================================

	/** Get list of game servers */
	UFUNCTION(BlueprintCallable, Category = "Master Server|Servers")
	void GetServerList(FOnServerListComplete OnComplete);

	/** Join a game server */
	UFUNCTION(BlueprintCallable, Category = "Master Server|Servers")
	void JoinServer(const FString& ServerID, FOnAPIComplete OnComplete);

	/** Get currently joined server */
	UFUNCTION(BlueprintPure, Category = "Master Server|Servers")
	FString GetCurrentServerID() const { return CurrentServerID; }

	// ============================================
	// GAME SERVER API (for dedicated servers)
	// ============================================

	/** Register this server with master (for dedicated servers) */
	UFUNCTION(BlueprintCallable, Category = "Master Server|GameServer")
	void RegisterGameServer(const FString& ServerID, const FString& Name,
		const FString& Host, int32 Port, int32 MaxPlayers,
		const FString& MapName, FOnAPIComplete OnComplete);

	/** Send heartbeat to master server */
	UFUNCTION(BlueprintCallable, Category = "Master Server|GameServer")
	void SendHeartbeat(const FString& ServerID, int32 CurrentPlayers, FOnAPIComplete OnComplete);

	// ============================================
	// ADMIN
	// ============================================

	/** Get server statistics (admin only) */
	UFUNCTION(BlueprintCallable, Category = "Master Server|Admin")
	void GetServerStats(FOnAPIComplete OnComplete);

	/** Ban an account (admin/gm only) */
	UFUNCTION(BlueprintCallable, Category = "Master Server|Admin")
	void BanAccount(int32 AccountID, const FString& Reason, bool bPermanent,
		int32 DurationHours, FOnAPIComplete OnComplete);

	// ============================================
	// EVENTS
	// ============================================

	/** Called when connection to server is lost */
	UPROPERTY(BlueprintAssignable, Category = "Master Server|Events")
	FOnConnectionLost OnConnectionLost;

	/** Called when reconnected to server */
	UPROPERTY(BlueprintAssignable, Category = "Master Server|Events")
	FOnReconnected OnReconnected;

protected:
	// ============================================
	// HTTP HELPERS
	// ============================================

	/** Send a GET request */
	void SendGetRequest(const FString& Endpoint, TFunction<void(TSharedPtr<FJsonObject>)> OnComplete);

	/** Send a POST request */
	void SendPostRequest(const FString& Endpoint, TSharedPtr<FJsonObject> Body,
		TFunction<void(TSharedPtr<FJsonObject>)> OnComplete);

	/** Send a PUT request */
	void SendPutRequest(const FString& Endpoint, TSharedPtr<FJsonObject> Body,
		TFunction<void(TSharedPtr<FJsonObject>)> OnComplete);

	/** Send a DELETE request */
	void SendDeleteRequest(const FString& Endpoint, TFunction<void(TSharedPtr<FJsonObject>)> OnComplete);

	/** Process HTTP response */
	void ProcessResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess,
		TFunction<void(TSharedPtr<FJsonObject>)> OnComplete);

	/** Check server health */
	void CheckServerHealth();

	/** Handle health check response */
	void OnHealthCheckComplete(TSharedPtr<FJsonObject> Response);

private:
	/** Master Server URL */
	FString ServerURL = TEXT("http://192.168.178.73:9877");

	/** Current Session ID */
	FString SessionID;

	/** JWT Token */
	FString AuthToken;

	/** Current Account ID */
	int32 CurrentAccountID = 0;

	/** Current Username */
	FString CurrentUsername;

	/** Is Admin */
	bool bIsAdmin = false;

	/** Is GM */
	bool bIsGM = false;

	/** Currently selected character */
	int32 SelectedCharacterID = 0;

	/** Currently joined server */
	FString CurrentServerID;

	/** Connection status */
	bool bIsConnected = false;

	/** Health check timer handle */
	FTimerHandle HealthCheckTimer;

	/** Retry count for health checks */
	int32 HealthCheckRetryCount = 0;

	/** Max retries before considering disconnected */
	static constexpr int32 MaxHealthCheckRetries = 3;

	/** Health check interval in seconds */
	static constexpr float HealthCheckInterval = 30.0f;
};
