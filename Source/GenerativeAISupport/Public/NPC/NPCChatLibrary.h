// NPC Chat Library - Blueprint callable functions for NPC AI Chat
// Part of GenerativeAISupport Plugin

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NPCChatLibrary.generated.h"

/**
 * Blueprint Function Library for NPC Chat
 * Connects to the NPC AI Server (Port 9879) to get AI-powered responses
 */
UCLASS()
class GENERATIVEAISUPPORT_API UNPCChatLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Send a chat message to an NPC and get a response
	 * @param Message - The player's message to the NPC
	 * @param NpcId - The NPC's personality ID (e.g., "lucy", "blacksmith", "merchant")
	 * @param OutResponse - The NPC's response
	 * @return True if successful, false if connection failed
	 */
	UFUNCTION(BlueprintCallable, Category = "NPC Chat", meta = (DisplayName = "Send NPC Chat"))
	static bool SendNPCChat(const FString& Message, const FString& NpcId, FString& OutResponse);

	/**
	 * Reset an NPC's conversation history
	 * @param NpcId - The NPC's ID to reset
	 * @return True if successful
	 */
	UFUNCTION(BlueprintCallable, Category = "NPC Chat", meta = (DisplayName = "Reset NPC Conversation"))
	static bool ResetNPCConversation(const FString& NpcId);

	/**
	 * Check if the NPC AI Server is running
	 * @return True if server is reachable
	 */
	UFUNCTION(BlueprintCallable, Category = "NPC Chat", meta = (DisplayName = "Is NPC Server Running"))
	static bool IsNPCServerRunning();

	/**
	 * Spawn an actor properly aligned on the ground (no floating, sinking, or tipping over)
	 * @param WorldContextObject - World context
	 * @param ActorClass - The class to spawn
	 * @param Location - Desired spawn location (Z will be adjusted to ground)
	 * @param Rotation - Desired rotation (Pitch and Roll will be zeroed for upright)
	 * @param SpawnedActor - The spawned actor reference
	 * @return True if spawn was successful
	 */
	UFUNCTION(BlueprintCallable, Category = "NPC Spawn", meta = (DisplayName = "Spawn Actor On Ground", WorldContext = "WorldContextObject"))
	static bool SpawnActorOnGround(UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, FVector Location, FRotator Rotation, AActor*& SpawnedActor);

	/**
	 * Find the ground position below a given location
	 * @param WorldContextObject - World context
	 * @param Location - Starting location to trace from
	 * @param GroundLocation - The found ground position
	 * @param MaxTraceDistance - How far down to trace (default 10000)
	 * @return True if ground was found
	 */
	UFUNCTION(BlueprintCallable, Category = "NPC Spawn", meta = (DisplayName = "Find Ground Position", WorldContext = "WorldContextObject"))
	static bool FindGroundPosition(UObject* WorldContextObject, FVector Location, FVector& GroundLocation, float MaxTraceDistance = 10000.f);

	/**
	 * Adjust an actor to stand properly on ground (fixes floating/sinking actors)
	 * @param Actor - The actor to adjust
	 * @param bSnapToGround - If true, moves actor to ground level
	 * @param bFixRotation - If true, sets pitch/roll to 0 (upright)
	 * @return True if adjustment was made
	 */
	UFUNCTION(BlueprintCallable, Category = "NPC Spawn", meta = (DisplayName = "Adjust Actor To Ground"))
	static bool AdjustActorToGround(AActor* Actor, bool bSnapToGround = true, bool bFixRotation = true);

private:
	static bool SendToNPCServer(const FString& JsonPayload, FString& OutResponse);
	static const int32 NPC_SERVER_PORT = 9879;
};
