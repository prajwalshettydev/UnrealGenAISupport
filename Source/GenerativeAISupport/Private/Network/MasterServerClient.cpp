// Master Server Client Implementation
// Part of GenerativeAISupport Plugin

#include "Network/MasterServerClient.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "TimerManager.h"
#include "Engine/World.h"

// ============================================
// LIFECYCLE
// ============================================

void UMasterServerClient::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogTemp, Log, TEXT("MasterServerClient initialisiert"));

	// Start health check timer
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			HealthCheckTimer,
			this,
			&UMasterServerClient::CheckServerHealth,
			HealthCheckInterval,
			true,
			5.0f  // Initial delay
		);
	}
}

void UMasterServerClient::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HealthCheckTimer);
	}

	Super::Deinitialize();
	UE_LOG(LogTemp, Log, TEXT("MasterServerClient heruntergefahren"));
}

UMasterServerClient* UMasterServerClient::Get(UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance)
	{
		return nullptr;
	}

	return GameInstance->GetSubsystem<UMasterServerClient>();
}

// ============================================
// CONFIGURATION
// ============================================

void UMasterServerClient::SetServerURL(const FString& URL)
{
	ServerURL = URL;
	UE_LOG(LogTemp, Log, TEXT("Master Server URL gesetzt: %s"), *ServerURL);

	// Trigger immediate health check
	CheckServerHealth();
}

// ============================================
// HTTP HELPERS
// ============================================

void UMasterServerClient::SendGetRequest(const FString& Endpoint, TFunction<void(TSharedPtr<FJsonObject>)> OnComplete)
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();

	Request->SetURL(ServerURL + Endpoint);
	Request->SetVerb(TEXT("GET"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

	if (!SessionID.IsEmpty())
	{
		Request->SetHeader(TEXT("X-Session-ID"), SessionID);
	}

	Request->OnProcessRequestComplete().BindLambda(
		[this, OnComplete](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess)
		{
			ProcessResponse(Req, Resp, bSuccess, OnComplete);
		});

	Request->ProcessRequest();
}

void UMasterServerClient::SendPostRequest(const FString& Endpoint, TSharedPtr<FJsonObject> Body,
	TFunction<void(TSharedPtr<FJsonObject>)> OnComplete)
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();

	Request->SetURL(ServerURL + Endpoint);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

	if (!SessionID.IsEmpty())
	{
		Request->SetHeader(TEXT("X-Session-ID"), SessionID);
	}

	// Serialize JSON body
	FString BodyString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyString);
	FJsonSerializer::Serialize(Body.ToSharedRef(), Writer);
	Request->SetContentAsString(BodyString);

	Request->OnProcessRequestComplete().BindLambda(
		[this, OnComplete](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess)
		{
			ProcessResponse(Req, Resp, bSuccess, OnComplete);
		});

	Request->ProcessRequest();
}

void UMasterServerClient::SendPutRequest(const FString& Endpoint, TSharedPtr<FJsonObject> Body,
	TFunction<void(TSharedPtr<FJsonObject>)> OnComplete)
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();

	Request->SetURL(ServerURL + Endpoint);
	Request->SetVerb(TEXT("PUT"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

	if (!SessionID.IsEmpty())
	{
		Request->SetHeader(TEXT("X-Session-ID"), SessionID);
	}

	FString BodyString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyString);
	FJsonSerializer::Serialize(Body.ToSharedRef(), Writer);
	Request->SetContentAsString(BodyString);

	Request->OnProcessRequestComplete().BindLambda(
		[this, OnComplete](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess)
		{
			ProcessResponse(Req, Resp, bSuccess, OnComplete);
		});

	Request->ProcessRequest();
}

void UMasterServerClient::SendDeleteRequest(const FString& Endpoint, TFunction<void(TSharedPtr<FJsonObject>)> OnComplete)
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();

	Request->SetURL(ServerURL + Endpoint);
	Request->SetVerb(TEXT("DELETE"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

	if (!SessionID.IsEmpty())
	{
		Request->SetHeader(TEXT("X-Session-ID"), SessionID);
	}

	Request->OnProcessRequestComplete().BindLambda(
		[this, OnComplete](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess)
		{
			ProcessResponse(Req, Resp, bSuccess, OnComplete);
		});

	Request->ProcessRequest();
}

void UMasterServerClient::ProcessResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess,
	TFunction<void(TSharedPtr<FJsonObject>)> OnComplete)
{
	TSharedPtr<FJsonObject> JsonResponse = MakeShared<FJsonObject>();

	if (!bSuccess || !Response.IsValid())
	{
		JsonResponse->SetBoolField(TEXT("success"), false);
		JsonResponse->SetStringField(TEXT("error"), TEXT("Verbindung zum Server fehlgeschlagen"));

		if (OnComplete)
		{
			OnComplete(JsonResponse);
		}
		return;
	}

	const int32 ResponseCode = Response->GetResponseCode();
	const FString ResponseContent = Response->GetContentAsString();

	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);
	if (FJsonSerializer::Deserialize(Reader, JsonResponse) && JsonResponse.IsValid())
	{
		if (OnComplete)
		{
			OnComplete(JsonResponse);
		}
	}
	else
	{
		JsonResponse = MakeShared<FJsonObject>();
		JsonResponse->SetBoolField(TEXT("success"), false);
		JsonResponse->SetStringField(TEXT("error"), TEXT("Ungültige Server-Antwort"));

		if (OnComplete)
		{
			OnComplete(JsonResponse);
		}
	}
}

void UMasterServerClient::CheckServerHealth()
{
	SendGetRequest(TEXT("/health"), [this](TSharedPtr<FJsonObject> Response)
	{
		OnHealthCheckComplete(Response);
	});
}

void UMasterServerClient::OnHealthCheckComplete(TSharedPtr<FJsonObject> Response)
{
	if (Response.IsValid() && Response->HasField(TEXT("status")))
	{
		FString Status = Response->GetStringField(TEXT("status"));
		if (Status == TEXT("healthy"))
		{
			if (!bIsConnected)
			{
				bIsConnected = true;
				HealthCheckRetryCount = 0;
				UE_LOG(LogTemp, Log, TEXT("Verbindung zum Master Server hergestellt"));
				OnReconnected.Broadcast();
			}
			return;
		}
	}

	// Health check failed
	HealthCheckRetryCount++;

	if (HealthCheckRetryCount >= MaxHealthCheckRetries && bIsConnected)
	{
		bIsConnected = false;
		UE_LOG(LogTemp, Warning, TEXT("Verbindung zum Master Server verloren"));
		OnConnectionLost.Broadcast(TEXT("Server nicht erreichbar"));
	}
}

// ============================================
// AUTHENTICATION
// ============================================

void UMasterServerClient::Register(const FString& Username, const FString& Email,
	const FString& Password, FOnAuthComplete OnComplete)
{
	TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("username"), Username);
	Body->SetStringField(TEXT("email"), Email);
	Body->SetStringField(TEXT("password"), Password);

	SendPostRequest(TEXT("/api/auth/register"), Body, [OnComplete](TSharedPtr<FJsonObject> Response)
	{
		FAuthResponse AuthResponse;
		AuthResponse.bSuccess = Response->GetBoolField(TEXT("success"));

		if (!AuthResponse.bSuccess)
		{
			AuthResponse.Error = Response->GetStringField(TEXT("error"));
		}
		else
		{
			AuthResponse.AccountID = Response->GetIntegerField(TEXT("account_id"));
		}

		OnComplete.ExecuteIfBound(AuthResponse);
	});
}

void UMasterServerClient::Login(const FString& Username, const FString& Password, FOnAuthComplete OnComplete)
{
	TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("username"), Username);
	Body->SetStringField(TEXT("password"), Password);

	SendPostRequest(TEXT("/api/auth/login"), Body, [this, OnComplete](TSharedPtr<FJsonObject> Response)
	{
		FAuthResponse AuthResponse;
		AuthResponse.bSuccess = Response->GetBoolField(TEXT("success"));

		if (!AuthResponse.bSuccess)
		{
			AuthResponse.Error = Response->GetStringField(TEXT("error"));
		}
		else
		{
			AuthResponse.Token = Response->GetStringField(TEXT("token"));
			AuthResponse.SessionID = Response->GetStringField(TEXT("session_id"));
			AuthResponse.AccountID = Response->GetIntegerField(TEXT("account_id"));
			AuthResponse.Username = Response->GetStringField(TEXT("username"));
			AuthResponse.bIsAdmin = Response->GetBoolField(TEXT("is_admin"));
			AuthResponse.bIsGM = Response->GetBoolField(TEXT("is_gm"));

			// Store session data
			SessionID = AuthResponse.SessionID;
			AuthToken = AuthResponse.Token;
			CurrentAccountID = AuthResponse.AccountID;
			CurrentUsername = AuthResponse.Username;
			bIsAdmin = AuthResponse.bIsAdmin;
			bIsGM = AuthResponse.bIsGM;

			UE_LOG(LogTemp, Log, TEXT("Login erfolgreich: %s"), *CurrentUsername);
		}

		OnComplete.ExecuteIfBound(AuthResponse);
	});
}

void UMasterServerClient::Logout(FOnAPIComplete OnComplete)
{
	SendPostRequest(TEXT("/api/auth/logout"), MakeShared<FJsonObject>(), [this, OnComplete](TSharedPtr<FJsonObject> Response)
	{
		FAPIResponse APIResponse;
		APIResponse.bSuccess = Response->GetBoolField(TEXT("success"));

		if (!APIResponse.bSuccess)
		{
			APIResponse.Error = Response->GetStringField(TEXT("error"));
		}

		// Clear session data regardless
		SessionID.Empty();
		AuthToken.Empty();
		CurrentAccountID = 0;
		CurrentUsername.Empty();
		bIsAdmin = false;
		bIsGM = false;
		SelectedCharacterID = 0;
		CurrentServerID.Empty();

		UE_LOG(LogTemp, Log, TEXT("Logout erfolgreich"));

		OnComplete.ExecuteIfBound(APIResponse);
	});
}

void UMasterServerClient::ValidateSession(FOnAuthComplete OnComplete)
{
	SendGetRequest(TEXT("/api/auth/validate"), [this, OnComplete](TSharedPtr<FJsonObject> Response)
	{
		FAuthResponse AuthResponse;
		AuthResponse.bSuccess = Response->GetBoolField(TEXT("success"));

		if (!AuthResponse.bSuccess)
		{
			AuthResponse.Error = Response->GetStringField(TEXT("error"));
			// Clear invalid session
			SessionID.Empty();
		}
		else
		{
			AuthResponse.AccountID = Response->GetIntegerField(TEXT("account_id"));
			AuthResponse.Username = Response->GetStringField(TEXT("username"));
			AuthResponse.bIsAdmin = Response->GetBoolField(TEXT("is_admin"));
			AuthResponse.bIsGM = Response->GetBoolField(TEXT("is_gm"));

			CurrentAccountID = AuthResponse.AccountID;
			CurrentUsername = AuthResponse.Username;
			bIsAdmin = AuthResponse.bIsAdmin;
			bIsGM = AuthResponse.bIsGM;

			if (Response->HasField(TEXT("character_id")))
			{
				SelectedCharacterID = Response->GetIntegerField(TEXT("character_id"));
			}
		}

		OnComplete.ExecuteIfBound(AuthResponse);
	});
}

// ============================================
// CHARACTERS
// ============================================

void UMasterServerClient::GetCharacters(FOnCharacterListComplete OnComplete)
{
	SendGetRequest(TEXT("/api/characters"), [OnComplete](TSharedPtr<FJsonObject> Response)
	{
		FCharacterListResponse CharResponse;
		CharResponse.bSuccess = Response->GetBoolField(TEXT("success"));

		if (!CharResponse.bSuccess)
		{
			CharResponse.Error = Response->GetStringField(TEXT("error"));
		}
		else
		{
			const TArray<TSharedPtr<FJsonValue>>* CharactersArray;
			if (Response->TryGetArrayField(TEXT("characters"), CharactersArray))
			{
				for (const TSharedPtr<FJsonValue>& CharValue : *CharactersArray)
				{
					TSharedPtr<FJsonObject> CharObj = CharValue->AsObject();
					if (CharObj.IsValid())
					{
						FCharacterData CharData;
						CharData.CharacterID = CharObj->GetIntegerField(TEXT("character_id"));
						CharData.AccountID = CharObj->GetIntegerField(TEXT("account_id"));
						CharData.Name = CharObj->GetStringField(TEXT("name"));
						CharData.ClassName = CharObj->GetStringField(TEXT("class_name"));
						CharData.Level = CharObj->GetIntegerField(TEXT("level"));
						CharData.Experience = CharObj->GetIntegerField(TEXT("experience"));
						CharData.Gold = CharObj->GetIntegerField(TEXT("gold"));
						CharData.LocationMap = CharObj->GetStringField(TEXT("location_map"));
						CharData.Location.X = CharObj->GetNumberField(TEXT("location_x"));
						CharData.Location.Y = CharObj->GetNumberField(TEXT("location_y"));
						CharData.Location.Z = CharObj->GetNumberField(TEXT("location_z"));
						CharData.PlaytimeMinutes = CharObj->GetIntegerField(TEXT("playtime_minutes"));
						CharData.CreatedAt = CharObj->GetStringField(TEXT("created_at"));
						CharData.LastPlayed = CharObj->GetStringField(TEXT("last_played"));

						CharResponse.Characters.Add(CharData);
					}
				}
			}
		}

		OnComplete.ExecuteIfBound(CharResponse);
	});
}

void UMasterServerClient::CreateCharacter(const FString& Name, const FString& ClassName, FOnAPIComplete OnComplete)
{
	TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("name"), Name);
	Body->SetStringField(TEXT("class_name"), ClassName);

	SendPostRequest(TEXT("/api/characters"), Body, [OnComplete](TSharedPtr<FJsonObject> Response)
	{
		FAPIResponse APIResponse;
		APIResponse.bSuccess = Response->GetBoolField(TEXT("success"));

		if (!APIResponse.bSuccess)
		{
			APIResponse.Error = Response->GetStringField(TEXT("error"));
		}
		else if (Response->HasField(TEXT("character_id")))
		{
			APIResponse.Data = FString::FromInt(Response->GetIntegerField(TEXT("character_id")));
		}

		OnComplete.ExecuteIfBound(APIResponse);
	});
}

void UMasterServerClient::UpdateCharacter(int32 CharacterID, const FCharacterData& Data, FOnAPIComplete OnComplete)
{
	TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetNumberField(TEXT("level"), Data.Level);
	Body->SetNumberField(TEXT("experience"), Data.Experience);
	Body->SetNumberField(TEXT("gold"), Data.Gold);
	Body->SetStringField(TEXT("location_map"), Data.LocationMap);
	Body->SetNumberField(TEXT("location_x"), Data.Location.X);
	Body->SetNumberField(TEXT("location_y"), Data.Location.Y);
	Body->SetNumberField(TEXT("location_z"), Data.Location.Z);
	Body->SetNumberField(TEXT("playtime_minutes"), Data.PlaytimeMinutes);

	FString Endpoint = FString::Printf(TEXT("/api/characters/%d"), CharacterID);

	SendPutRequest(Endpoint, Body, [OnComplete](TSharedPtr<FJsonObject> Response)
	{
		FAPIResponse APIResponse;
		APIResponse.bSuccess = Response->GetBoolField(TEXT("success"));

		if (!APIResponse.bSuccess)
		{
			APIResponse.Error = Response->GetStringField(TEXT("error"));
		}

		OnComplete.ExecuteIfBound(APIResponse);
	});
}

void UMasterServerClient::DeleteCharacter(int32 CharacterID, FOnAPIComplete OnComplete)
{
	FString Endpoint = FString::Printf(TEXT("/api/characters/%d"), CharacterID);

	SendDeleteRequest(Endpoint, [OnComplete](TSharedPtr<FJsonObject> Response)
	{
		FAPIResponse APIResponse;
		APIResponse.bSuccess = Response->GetBoolField(TEXT("success"));

		if (!APIResponse.bSuccess)
		{
			APIResponse.Error = Response->GetStringField(TEXT("error"));
		}

		OnComplete.ExecuteIfBound(APIResponse);
	});
}

void UMasterServerClient::SelectCharacter(int32 CharacterID, FOnAPIComplete OnComplete)
{
	TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetNumberField(TEXT("character_id"), CharacterID);

	SendPostRequest(TEXT("/api/session/select-character"), Body, [this, CharacterID, OnComplete](TSharedPtr<FJsonObject> Response)
	{
		FAPIResponse APIResponse;
		APIResponse.bSuccess = Response->GetBoolField(TEXT("success"));

		if (!APIResponse.bSuccess)
		{
			APIResponse.Error = Response->GetStringField(TEXT("error"));
		}
		else
		{
			SelectedCharacterID = CharacterID;
		}

		OnComplete.ExecuteIfBound(APIResponse);
	});
}

// ============================================
// SERVERS
// ============================================

void UMasterServerClient::GetServerList(FOnServerListComplete OnComplete)
{
	SendGetRequest(TEXT("/api/servers"), [OnComplete](TSharedPtr<FJsonObject> Response)
	{
		FServerListResponse ServerResponse;
		ServerResponse.bSuccess = Response->GetBoolField(TEXT("success"));

		if (!ServerResponse.bSuccess)
		{
			ServerResponse.Error = Response->GetStringField(TEXT("error"));
		}
		else
		{
			const TArray<TSharedPtr<FJsonValue>>* ServersArray;
			if (Response->TryGetArrayField(TEXT("servers"), ServersArray))
			{
				for (const TSharedPtr<FJsonValue>& ServerValue : *ServersArray)
				{
					TSharedPtr<FJsonObject> ServerObj = ServerValue->AsObject();
					if (ServerObj.IsValid())
					{
						FGameServerInfo ServerInfo;
						ServerInfo.ServerID = ServerObj->GetStringField(TEXT("server_id"));
						ServerInfo.Name = ServerObj->GetStringField(TEXT("name"));
						ServerInfo.Host = ServerObj->GetStringField(TEXT("host"));
						ServerInfo.Port = ServerObj->GetIntegerField(TEXT("port"));
						ServerInfo.Status = ServerObj->GetStringField(TEXT("status"));
						ServerInfo.CurrentPlayers = ServerObj->GetIntegerField(TEXT("current_players"));
						ServerInfo.MaxPlayers = ServerObj->GetIntegerField(TEXT("max_players"));
						ServerInfo.MapName = ServerObj->GetStringField(TEXT("map_name"));
						ServerInfo.LastHeartbeat = ServerObj->GetStringField(TEXT("last_heartbeat"));

						ServerResponse.Servers.Add(ServerInfo);
					}
				}
			}
		}

		OnComplete.ExecuteIfBound(ServerResponse);
	});
}

void UMasterServerClient::JoinServer(const FString& ServerID, FOnAPIComplete OnComplete)
{
	TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("server_id"), ServerID);

	SendPostRequest(TEXT("/api/session/join-server"), Body, [this, ServerID, OnComplete](TSharedPtr<FJsonObject> Response)
	{
		FAPIResponse APIResponse;
		APIResponse.bSuccess = Response->GetBoolField(TEXT("success"));

		if (!APIResponse.bSuccess)
		{
			APIResponse.Error = Response->GetStringField(TEXT("error"));
		}
		else
		{
			CurrentServerID = ServerID;
		}

		OnComplete.ExecuteIfBound(APIResponse);
	});
}

// ============================================
// GAME SERVER API
// ============================================

void UMasterServerClient::RegisterGameServer(const FString& ServerID, const FString& Name,
	const FString& Host, int32 Port, int32 MaxPlayers, const FString& MapName, FOnAPIComplete OnComplete)
{
	TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("server_id"), ServerID);
	Body->SetStringField(TEXT("name"), Name);
	Body->SetStringField(TEXT("host"), Host);
	Body->SetNumberField(TEXT("port"), Port);
	Body->SetNumberField(TEXT("max_players"), MaxPlayers);
	Body->SetStringField(TEXT("map_name"), MapName);

	SendPostRequest(TEXT("/api/servers/register"), Body, [OnComplete](TSharedPtr<FJsonObject> Response)
	{
		FAPIResponse APIResponse;
		APIResponse.bSuccess = Response->GetBoolField(TEXT("success"));

		if (!APIResponse.bSuccess)
		{
			APIResponse.Error = Response->GetStringField(TEXT("error"));
		}

		OnComplete.ExecuteIfBound(APIResponse);
	});
}

void UMasterServerClient::SendHeartbeat(const FString& ServerID, int32 CurrentPlayers, FOnAPIComplete OnComplete)
{
	TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("server_id"), ServerID);
	Body->SetNumberField(TEXT("current_players"), CurrentPlayers);

	SendPostRequest(TEXT("/api/servers/heartbeat"), Body, [OnComplete](TSharedPtr<FJsonObject> Response)
	{
		FAPIResponse APIResponse;
		APIResponse.bSuccess = Response->GetBoolField(TEXT("success"));

		if (!APIResponse.bSuccess)
		{
			APIResponse.Error = Response->GetStringField(TEXT("error"));
		}

		OnComplete.ExecuteIfBound(APIResponse);
	});
}

// ============================================
// ADMIN
// ============================================

void UMasterServerClient::GetServerStats(FOnAPIComplete OnComplete)
{
	SendGetRequest(TEXT("/api/admin/stats"), [OnComplete](TSharedPtr<FJsonObject> Response)
	{
		FAPIResponse APIResponse;
		APIResponse.bSuccess = Response->GetBoolField(TEXT("success"));

		if (!APIResponse.bSuccess)
		{
			APIResponse.Error = Response->GetStringField(TEXT("error"));
		}
		else
		{
			// Serialize stats to JSON string
			const TSharedPtr<FJsonObject>* StatsObj;
			if (Response->TryGetObjectField(TEXT("stats"), StatsObj))
			{
				FString StatsString;
				TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&StatsString);
				FJsonSerializer::Serialize(StatsObj->ToSharedRef(), Writer);
				APIResponse.Data = StatsString;
			}
		}

		OnComplete.ExecuteIfBound(APIResponse);
	});
}

void UMasterServerClient::BanAccount(int32 AccountID, const FString& Reason, bool bPermanent,
	int32 DurationHours, FOnAPIComplete OnComplete)
{
	TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetNumberField(TEXT("account_id"), AccountID);
	Body->SetStringField(TEXT("reason"), Reason);
	Body->SetBoolField(TEXT("is_permanent"), bPermanent);
	Body->SetNumberField(TEXT("duration_hours"), DurationHours);

	SendPostRequest(TEXT("/api/admin/ban"), Body, [OnComplete](TSharedPtr<FJsonObject> Response)
	{
		FAPIResponse APIResponse;
		APIResponse.bSuccess = Response->GetBoolField(TEXT("success"));

		if (!APIResponse.bSuccess)
		{
			APIResponse.Error = Response->GetStringField(TEXT("error"));
		}

		OnComplete.ExecuteIfBound(APIResponse);
	});
}
