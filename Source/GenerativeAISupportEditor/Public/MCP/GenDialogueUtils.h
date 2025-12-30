// Copyright (c) 2025 Prajwal Shetty. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the root directory of this
// source tree or http://opensource.org/licenses/MIT.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NPC/NPCDialogTypes.h"
#include "GenDialogueUtils.generated.h"

/**
 * Editor utilities for NPC Dialog System
 * Allows configuring dialog components via Python/MCP
 */
UCLASS()
class GENERATIVEAISUPPORTEDITOR_API UGenDialogueUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ============================================
	// COMPONENT MANAGEMENT
	// ============================================

	/**
	 * Find all actors with NPCDialogComponent in the level.
	 * @return JSON array of actors with dialog components
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Dialogue Utils")
	static FString ListDialogActors();

	/**
	 * Get dialog component configuration for an actor.
	 * @param ActorName - Actor label to search for
	 * @return JSON with dialog component settings
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Dialogue Utils")
	static FString GetDialogConfig(const FString& ActorName);

	/**
	 * Set dialog component configuration on an actor.
	 * @param ActorName - Actor label
	 * @param NPCID - NPC unique ID
	 * @param DisplayName - Display name
	 * @param Title - Title/Role
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Dialogue Utils")
	static FString SetDialogConfig(const FString& ActorName, const FString& NPCID,
								   const FString& DisplayName, const FString& Title);

	/**
	 * Add NPCDialogComponent to an actor.
	 * @param ActorName - Actor label
	 * @param NPCID - NPC unique ID
	 * @param DisplayName - Display name
	 * @return JSON result with component info
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Dialogue Utils")
	static FString AddDialogComponent(const FString& ActorName, const FString& NPCID, const FString& DisplayName);

	/**
	 * Remove NPCDialogComponent from an actor.
	 * @param ActorName - Actor label
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Dialogue Utils")
	static FString RemoveDialogComponent(const FString& ActorName);

	// ============================================
	// GREETING & FAREWELL
	// ============================================

	/**
	 * Set greeting message for NPC.
	 * @param ActorName - Actor label
	 * @param GreetingText - The greeting message
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Dialogue Utils")
	static FString SetGreeting(const FString& ActorName, const FString& GreetingText);

	/**
	 * Set farewell message for NPC.
	 * @param ActorName - Actor label
	 * @param FarewellText - The farewell message
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Dialogue Utils")
	static FString SetFarewell(const FString& ActorName, const FString& FarewellText);

	// ============================================
	// QUICK OPTIONS
	// ============================================

	/**
	 * Get quick dialog options for an NPC.
	 * @param ActorName - Actor label
	 * @return JSON array of quick options
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Dialogue Utils")
	static FString GetQuickOptions(const FString& ActorName);

	/**
	 * Add a quick dialog option to an NPC.
	 * @param ActorName - Actor label
	 * @param DisplayText - Text shown to player
	 * @param Message - Message sent to NPC
	 * @param Hotkey - Hotkey number (1-9)
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Dialogue Utils")
	static FString AddQuickOption(const FString& ActorName, const FString& DisplayText,
								  const FString& Message, int32 Hotkey);

	/**
	 * Clear all quick options for an NPC.
	 * @param ActorName - Actor label
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Dialogue Utils")
	static FString ClearQuickOptions(const FString& ActorName);

	// ============================================
	// TRADE & SHOP
	// ============================================

	/**
	 * Configure trade settings for NPC.
	 * @param ActorName - Actor label
	 * @param bCanTrade - Enable/disable trading
	 * @param ShopID - Shop ID reference
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Dialogue Utils")
	static FString SetTradeConfig(const FString& ActorName, bool bCanTrade, const FString& ShopID);

	// ============================================
	// MOOD & REPUTATION
	// ============================================

	/**
	 * Set NPC mood.
	 * @param ActorName - Actor label
	 * @param MoodName - Mood name (Neutral, Happy, Sad, Angry, etc.)
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Dialogue Utils")
	static FString SetMood(const FString& ActorName, const FString& MoodName);

	/**
	 * Set NPC reputation.
	 * @param ActorName - Actor label
	 * @param Reputation - Reputation value (-1000 to 1000)
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Dialogue Utils")
	static FString SetReputation(const FString& ActorName, int32 Reputation);

	// ============================================
	// PORTRAIT & VISUAL
	// ============================================

	/**
	 * Set portrait image path.
	 * @param ActorName - Actor label
	 * @param PortraitPath - Path to portrait texture
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Dialogue Utils")
	static FString SetPortrait(const FString& ActorName, const FString& PortraitPath);

	/**
	 * Set faction ID.
	 * @param ActorName - Actor label
	 * @param FactionID - Faction identifier
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Dialogue Utils")
	static FString SetFaction(const FString& ActorName, const FString& FactionID);

	// ============================================
	// RUNTIME CONTROL (PIE)
	// ============================================

	/**
	 * Start dialog with NPC (in PIE mode).
	 * @param ActorName - Actor label
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Dialogue Utils")
	static FString StartDialog(const FString& ActorName);

	/**
	 * End dialog with NPC (in PIE mode).
	 * @param ActorName - Actor label
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Dialogue Utils")
	static FString EndDialog(const FString& ActorName);

	/**
	 * Send message to NPC (in PIE mode).
	 * @param ActorName - Actor label
	 * @param Message - Message to send
	 * @return JSON result
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Dialogue Utils")
	static FString SendMessage(const FString& ActorName, const FString& Message);

	// ============================================
	// EXPORT/IMPORT
	// ============================================

	/**
	 * Export all dialog configurations to JSON.
	 * @return JSON with all NPC dialog configs
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Dialogue Utils")
	static FString ExportAllDialogConfigs();

	/**
	 * List available moods.
	 * @return JSON array of mood names
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Dialogue Utils")
	static FString ListMoods();

	/**
	 * List choice types.
	 * @return JSON array of choice type names
	 */
	UFUNCTION(BlueprintCallable, Category = "Generative AI|Dialogue Utils")
	static FString ListChoiceTypes();

private:
	/** Helper to find actor by name */
	static AActor* FindActorByName(const FString& ActorName);

	/** Helper to find dialog component */
	static class UNPCDialogComponent* FindDialogComponent(const FString& ActorName);

	/** Convert mood string to enum */
	static ENPCMood StringToMood(const FString& MoodString);

	/** Convert mood enum to string */
	static FString MoodToString(ENPCMood Mood);
};
