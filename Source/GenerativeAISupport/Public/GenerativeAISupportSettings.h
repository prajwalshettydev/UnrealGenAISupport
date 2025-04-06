// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GenerativeAISupportSettings.generated.h"

/**
 * Settings for the Generative AI Support Plugin
 */
UCLASS(config=Game, defaultconfig)
class GENERATIVEAISUPPORT_API UGenerativeAISupportSettings : public UObject
{
    GENERATED_BODY()

public:
    UGenerativeAISupportSettings();

    /** Whether to automatically start the socket server when Unreal Engine launches */
    UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Socket Server", meta = (DisplayName = "Auto Start Socket Server"))
    bool bAutoStartSocketServer;
};
