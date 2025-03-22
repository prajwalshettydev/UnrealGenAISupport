// GenerativeAISupportSettings.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GenerativeAISupportSettings.generated.h"

/**
 * Settings for the GenerativeAISupport plugin
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Generative AI Support"))
class GENERATIVEAISUPPORT_API UGenerativeAISupportSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UGenerativeAISupportSettings()
	{
		// Default values
		AutoStartSocketServer = false;
	}

	// Name that will appear in the settings menu
	virtual FName GetCategoryName() const override { return FName("Plugins"); }

	/** Whether to automatically start the socket server when the editor launches */
	UPROPERTY(config, EditAnywhere, Category = "Socket Server", meta = (DisplayName = "Auto Start Socket Server"))
	bool AutoStartSocketServer;
};