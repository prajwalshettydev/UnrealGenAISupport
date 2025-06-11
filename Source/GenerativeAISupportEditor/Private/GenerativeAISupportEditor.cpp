// Copyright (c) 2025 Prajwal Shetty. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the root directory of this
// source tree or http://opensource.org/licenses/MIT.

#include "GenerativeAISupportEditor.h"
#include "Editor/GenEditorCommands.h"
#include "Editor/GenEditorWindow.h"
#include "EditorStyleSet.h"
#include "GenerativeAISupportSettings.h"
#include "ISettingsModule.h"
#include "ISettingsSection.h"
#include "LevelEditor.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

#define LOCTEXT_NAMESPACE "FGenerativeAISupportEditorModule"

// Define the tab ID for our editor window
static const FName GenEditorTabId("GenEditorWindow");

FGenerativeAISupportEditorModule::FGenerativeAISupportEditorModule() : bSettingsRegistered(false)
{
	// Constructor initialization ensures proper state
}

void FGenerativeAISupportEditorModule::StartupModule()
{
	// Log to debug module loading
	UE_LOG(LogTemp, Log, TEXT("FGenerativeAISupportEditorModule::StartupModule called"));

	// Register project settings
	RegisterSettings();

	// Register menu extension
	RegisterMenuExtension();

	// Register tab spawner
	FGlobalTabmanager::Get()
		->RegisterNomadTabSpawner(
			GenEditorTabId,
			FOnSpawnTab::CreateRaw(&FGenEditorWindowManager::Get(), &FGenEditorWindowManager::SpawnEditorWindowTab))
		.SetDisplayName(LOCTEXT("GenEditorWindowTitle", "Gen AI Support"))
		.SetTooltipText(LOCTEXT("GenEditorWindowTooltip", "Open the Generative AI Support window"))
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Details"))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory());
}

void FGenerativeAISupportEditorModule::ShutdownModule()
{
	// Unregister settings
	UnregisterSettings();

	// Unregister menu extension
	UnregisterMenuExtension();

	// Unregister tab spawner
	if (FSlateApplication::IsInitialized())
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(GenEditorTabId);
	}
}

void FGenerativeAISupportEditorModule::RegisterSettings()
{
	// Prevent duplicate registration
	if (bSettingsRegistered)
	{
		UE_LOG(LogTemp, Warning, TEXT("GenerativeAISupport settings already registered, skipping."));
		return;
	}

	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		ISettingsSectionPtr SettingsSection = SettingsModule->RegisterSettings(
			"Project", "Plugins", "GenerativeAISupport",
			LOCTEXT("GenerativeAISupportSettingsName", "Generative AI Support"),
			LOCTEXT("GenerativeAISupportDescription", "Configuration for the GenerativeAISupport plugin"),
			GetMutableDefault<UGenerativeAISupportSettings>());

		if (SettingsSection.IsValid())
		{
			SettingsSection->OnModified().BindRaw(this, &FGenerativeAISupportEditorModule::HandleSettingsSaved);
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
}

void FGenerativeAISupportEditorModule::UnregisterSettings()
{
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
}

bool FGenerativeAISupportEditorModule::HandleSettingsSaved()
{
	UGenerativeAISupportSettings* Settings = GetMutableDefault<UGenerativeAISupportSettings>();
	Settings->SaveConfig();

	return true;
}

void FGenerativeAISupportEditorModule::RegisterMenuExtension()
{
	// Register commands
	FGenEditorCommands::Register();
	PluginCommands = MakeShareable(new FUICommandList);

	// Map commands to actions
	PluginCommands->MapAction(
		FGenEditorCommands::Get().OpenGenEditorWindow,
		FExecuteAction::CreateRaw(this, &FGenerativeAISupportEditorModule::OnEditorWindowMenuClicked),
		FCanExecuteAction());

	// Register menu extension - add to the Window menu
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());

	MenuExtender->AddMenuExtension(
		"WindowLayout", EExtensionHook::After, PluginCommands,
		FMenuExtensionDelegate::CreateLambda([](FMenuBuilder& Builder)
											 { Builder.AddMenuEntry(FGenEditorCommands::Get().OpenGenEditorWindow); }));

	LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
}

void FGenerativeAISupportEditorModule::UnregisterMenuExtension()
{
	// Unregister commands
	FGenEditorCommands::Unregister();
}

void FGenerativeAISupportEditorModule::OnEditorWindowMenuClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(GenEditorTabId);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGenerativeAISupportEditorModule, GenerativeAISupportEditor)
