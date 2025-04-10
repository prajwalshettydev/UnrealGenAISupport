// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class GENERATIVEAISUPPORT_API FGenerativeAISupportModule : public IModuleInterface
{
public:
    FGenerativeAISupportModule();

    // IModuleInterface implementation
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    void RegisterSettings();
    void UnregisterSettings();
    
    bool HandleSettingsSaved();
    void RegisterMenuExtension();
    void UnregisterMenuExtension();
    void OnEditorWindowMenuClicked();

    bool bSettingsRegistered;
    
    TSharedPtr<FUICommandList> PluginCommands;
};
