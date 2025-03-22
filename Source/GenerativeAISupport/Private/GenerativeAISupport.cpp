// Copyright Epic Games, Inc. All Rights Reserved.

#include "GenerativeAISupport.h"
#if WITH_EDITOR
#include "ISettingsModule.h"
#endif
#include "GenerativeAISupportSettings.h"
#include "ISettingsSection.h"

#define LOCTEXT_NAMESPACE "FGenerativeAISupportModule"

void FGenerativeAISupportModule::StartupModule()
{
    // Log to debug module loading
    UE_LOG(LogTemp, Log, TEXT("FGenerativeAISupportModule::StartupModule called"));

    // Register project settings
    RegisterSettings();
}

void FGenerativeAISupportModule::ShutdownModule()
{
    // Unregister settings
    UnregisterSettings();
}

void FGenerativeAISupportModule::RegisterSettings()
{
#if WITH_EDITOR
    UnregisterSettings();
    // Prevent duplicate registration
    if (bSettingsRegistered)
    {
        UE_LOG(LogTemp, Warning, TEXT("GenerativeAISupport settings already registered, skipping."));
        return;
    }

    if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
    {
        ISettingsSectionPtr SettingsSection = SettingsModule->RegisterSettings("Project", "Plugins", "GenerativeAISupport",
            FText::FromString("Generative AI Support"),
            FText::FromString("Configuration for the GenerativeAISupport plugin"),
            GetMutableDefault<UGenerativeAISupportSettings>()
        );

        if (SettingsSection.IsValid())
        {
            bSettingsRegistered = true;
            UE_LOG(LogTemp, Log, TEXT("GenerativeAISupport settings registered successfully."));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to register GenerativeAISupport settings."));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Settings module not found."));
    }
#endif
}

void FGenerativeAISupportModule::UnregisterSettings()
{
#if WITH_EDITOR
    if (bSettingsRegistered)
    {
        if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
        {
            SettingsModule->UnregisterSettings("Project", "Plugins", "GenerativeAISupport");
            bSettingsRegistered = false;
            UE_LOG(LogTemp, Log, TEXT("GenerativeAISupport settings unregistered successfully."));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Settings module not found during unregistration."));
        }
    }
#endif
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGenerativeAISupportModule, GenerativeAISupport)