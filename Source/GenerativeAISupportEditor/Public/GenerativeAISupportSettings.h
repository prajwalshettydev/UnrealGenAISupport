// Copyright (c) 2025 Prajwal Shetty. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the root directory of this
// source tree or http://opensource.org/licenses/MIT.

#pragma once

#include "CoreMinimal.h"
#include "GenerativeAISupportSettings.generated.h"
#include "UObject/NoExportTypes.h"

/**
 * Settings for the Generative AI Support Plugin
 */
UCLASS(config = Game, defaultconfig)
class GENERATIVEAISUPPORTEDITOR_API UGenerativeAISupportSettings : public UObject
{
	GENERATED_BODY()

  public:
	UGenerativeAISupportSettings();

	/** Whether to automatically start the socket server when Unreal Engine launches */
	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Socket Server",
			  meta = (DisplayName = "Auto Start Socket Server"))
	bool bAutoStartSocketServer;
};
