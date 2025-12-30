// NPC Chat Library Implementation

#include "NPC/NPCChatLibrary.h"
#include "Networking.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Common/TcpSocketBuilder.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Components/PrimitiveComponent.h"
#include "CollisionQueryParams.h"

bool UNPCChatLibrary::SendNPCChat(const FString& Message, const FString& NpcId, FString& OutResponse)
{
	// Build JSON payload
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField(TEXT("type"), TEXT("chat"));
	JsonObject->SetStringField(TEXT("npc_id"), NpcId);
	JsonObject->SetStringField(TEXT("personality"), NpcId);
	JsonObject->SetStringField(TEXT("message"), Message);

	FString JsonPayload;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonPayload);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	FString Response;
	if (!SendToNPCServer(JsonPayload, Response))
	{
		OutResponse = TEXT("*Der NPC scheint gerade nicht ansprechbar zu sein*");
		return false;
	}

	// Parse response
	TSharedPtr<FJsonObject> ResponseJson;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response);

	if (FJsonSerializer::Deserialize(Reader, ResponseJson) && ResponseJson.IsValid())
	{
		FString ResponseType;
		if (ResponseJson->TryGetStringField(TEXT("type"), ResponseType))
		{
			if (ResponseType == TEXT("response"))
			{
				ResponseJson->TryGetStringField(TEXT("message"), OutResponse);
				return true;
			}
			else if (ResponseType == TEXT("error"))
			{
				FString Error;
				ResponseJson->TryGetStringField(TEXT("error"), Error);
				OutResponse = FString::Printf(TEXT("*Fehler: %s*"), *Error);
				return false;
			}
		}
	}

	OutResponse = TEXT("*Unverständliche Antwort vom NPC*");
	return false;
}

bool UNPCChatLibrary::ResetNPCConversation(const FString& NpcId)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField(TEXT("type"), TEXT("reset"));
	JsonObject->SetStringField(TEXT("npc_id"), NpcId);

	FString JsonPayload;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonPayload);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	FString Response;
	return SendToNPCServer(JsonPayload, Response);
}

bool UNPCChatLibrary::IsNPCServerRunning()
{
	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SocketSubsystem)
	{
		return false;
	}

	TSharedRef<FInternetAddr> Addr = SocketSubsystem->CreateInternetAddr();
	bool bIsValid;
	Addr->SetIp(TEXT("192.168.178.73"), bIsValid);  // Claude Server
	Addr->SetPort(NPC_SERVER_PORT);

	FSocket* Socket = SocketSubsystem->CreateSocket(NAME_Stream, TEXT("NPCServerCheck"), false);
	if (!Socket)
	{
		return false;
	}

	bool bConnected = Socket->Connect(*Addr);
	Socket->Close();
	SocketSubsystem->DestroySocket(Socket);

	return bConnected;
}

bool UNPCChatLibrary::SendToNPCServer(const FString& JsonPayload, FString& OutResponse)
{
	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SocketSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("NPCChat: Failed to get socket subsystem"));
		return false;
	}

	TSharedRef<FInternetAddr> Addr = SocketSubsystem->CreateInternetAddr();
	bool bIsValid;
	Addr->SetIp(TEXT("192.168.178.73"), bIsValid);  // Claude Server
	Addr->SetPort(NPC_SERVER_PORT);

	FSocket* Socket = SocketSubsystem->CreateSocket(NAME_Stream, TEXT("NPCChatSocket"), false);
	if (!Socket)
	{
		UE_LOG(LogTemp, Error, TEXT("NPCChat: Failed to create socket"));
		return false;
	}

	// Set timeout
	Socket->SetNonBlocking(false);
	Socket->SetRecvErr();

	if (!Socket->Connect(*Addr))
	{
		UE_LOG(LogTemp, Error, TEXT("NPCChat: Failed to connect to NPC server on port %d"), NPC_SERVER_PORT);
		Socket->Close();
		SocketSubsystem->DestroySocket(Socket);
		return false;
	}

	// Send data
	FTCHARToUTF8 Converter(*JsonPayload);
	int32 BytesSent = 0;
	if (!Socket->Send((const uint8*)Converter.Get(), Converter.Length(), BytesSent))
	{
		UE_LOG(LogTemp, Error, TEXT("NPCChat: Failed to send data"));
		Socket->Close();
		SocketSubsystem->DestroySocket(Socket);
		return false;
	}

	// Receive response
	TArray<uint8> RecvBuffer;
	RecvBuffer.SetNumZeroed(8192);
	int32 BytesRead = 0;

	// Wait for data with timeout
	Socket->Wait(ESocketWaitConditions::WaitForRead, FTimespan::FromSeconds(30.0));

	if (Socket->Recv(RecvBuffer.GetData(), RecvBuffer.Num(), BytesRead))
	{
		if (BytesRead > 0)
		{
			RecvBuffer.SetNum(BytesRead);
			OutResponse = FString(UTF8_TO_TCHAR(RecvBuffer.GetData()));
			UE_LOG(LogTemp, Log, TEXT("NPCChat: Received %d bytes"), BytesRead);
		}
	}

	Socket->Close();
	SocketSubsystem->DestroySocket(Socket);

	return BytesRead > 0;
}

bool UNPCChatLibrary::FindGroundPosition(UObject* WorldContextObject, FVector Location, FVector& GroundLocation, float MaxTraceDistance)
{
	if (!WorldContextObject)
	{
		return false;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return false;
	}

	// Start trace from above the location, trace downward
	FVector TraceStart = Location + FVector(0, 0, 500.f);  // Start 500 units above
	FVector TraceEnd = Location - FVector(0, 0, MaxTraceDistance);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = false;
	QueryParams.bReturnPhysicalMaterial = false;

	// Trace against world static geometry
	bool bHit = World->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		ECC_WorldStatic,
		QueryParams
	);

	if (bHit)
	{
		GroundLocation = HitResult.ImpactPoint;
		return true;
	}

	return false;
}

bool UNPCChatLibrary::SpawnActorOnGround(UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, FVector Location, FRotator Rotation, AActor*& SpawnedActor)
{
	SpawnedActor = nullptr;

	if (!WorldContextObject || !ActorClass)
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnActorOnGround: Invalid parameters"));
		return false;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnActorOnGround: Could not get world"));
		return false;
	}

	// Find ground position
	FVector GroundLocation;
	if (!FindGroundPosition(WorldContextObject, Location, GroundLocation))
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnActorOnGround: Could not find ground, using original location"));
		GroundLocation = Location;
	}

	// Fix rotation - keep only Yaw, zero out Pitch and Roll for upright spawn
	FRotator UprightRotation(0.f, Rotation.Yaw, 0.f);

	// Spawn parameters
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// Spawn the actor
	SpawnedActor = World->SpawnActor<AActor>(ActorClass, GroundLocation, UprightRotation, SpawnParams);

	if (!SpawnedActor)
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnActorOnGround: Failed to spawn actor"));
		return false;
	}

	// Adjust to ground after spawn (in case bounding box affects position)
	AdjustActorToGround(SpawnedActor, true, true);

	UE_LOG(LogTemp, Log, TEXT("SpawnActorOnGround: Successfully spawned %s at %s"),
		*ActorClass->GetName(), *SpawnedActor->GetActorLocation().ToString());

	return true;
}

bool UNPCChatLibrary::AdjustActorToGround(AActor* Actor, bool bSnapToGround, bool bFixRotation)
{
	if (!Actor)
	{
		return false;
	}

	bool bAdjusted = false;

	// Fix rotation first (no tipping, rolling, or headstand)
	if (bFixRotation)
	{
		FRotator CurrentRotation = Actor->GetActorRotation();
		FRotator UprightRotation(0.f, CurrentRotation.Yaw, 0.f);

		if (!CurrentRotation.Equals(UprightRotation, 0.1f))
		{
			Actor->SetActorRotation(UprightRotation);
			bAdjusted = true;
		}
	}

	// Snap to ground
	if (bSnapToGround)
	{
		UWorld* World = Actor->GetWorld();
		if (!World)
		{
			return bAdjusted;
		}

		FVector ActorLocation = Actor->GetActorLocation();

		// Trace from actor's current position DOWN to find ground below
		FVector TraceStart = ActorLocation;
		FVector TraceEnd = ActorLocation - FVector(0, 0, 10000.f);

		FHitResult HitResult;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(Actor);  // Don't hit ourselves
		QueryParams.bTraceComplex = false;

		bool bHit = World->LineTraceSingleByChannel(
			HitResult,
			TraceStart,
			TraceEnd,
			ECC_WorldStatic,
			QueryParams
		);

		if (bHit)
		{
			// Simple approach: Put actor directly on ground
			// Most character pivots are at feet, so this works
			// Add tiny offset (2 units) to prevent clipping into ground
			float NewZ = HitResult.ImpactPoint.Z + 2.f;

			// Only adjust if difference is significant (more than 1 unit)
			if (FMath::Abs(ActorLocation.Z - NewZ) > 1.f)
			{
				FVector NewLocation(ActorLocation.X, ActorLocation.Y, NewZ);
				Actor->SetActorLocation(NewLocation);
				bAdjusted = true;

				UE_LOG(LogTemp, Log, TEXT("AdjustActorToGround: Moved %s from Z=%.1f to Z=%.1f"),
					*Actor->GetName(), ActorLocation.Z, NewZ);
			}
		}
	}

	return bAdjusted;
}
