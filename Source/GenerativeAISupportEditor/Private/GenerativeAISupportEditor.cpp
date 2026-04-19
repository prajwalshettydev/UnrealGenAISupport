// Copyright (c) 2025 Prajwal Shetty. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the root directory of this
// source tree or http://opensource.org/licenses/MIT.



#include "GenerativeAISupportEditor.h"
#include "ISettingsModule.h"
#include "Styling/AppStyle.h"
#include "GenerativeAISupportSettings.h"
#include "ISettingsSection.h"
#include "LevelEditor.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "Editor/GenEditorCommands.h"
#include "Editor/GenEditorWindow.h"
#include "Interfaces/IPluginManager.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#define LOCTEXT_NAMESPACE "FGenerativeAISupportEditorModule"

// Define the tab ID for our editor window
static const FName GenEditorTabId("GenEditorWindow");

namespace
{
const TCHAR* UnrealHandshakeServerName = TEXT("unreal-handshake");
const TCHAR* DefaultUnrealHost = TEXT("localhost");
constexpr int32 DefaultUnrealPort = 9877;

bool ResolvePluginPaths(FString& OutPluginBaseDir, FString& OutPluginPythonPath)
{
    TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("GenerativeAISupport"));
    if (!Plugin.IsValid())
    {
        return false;
    }

    OutPluginBaseDir = FPaths::ConvertRelativePathToFull(Plugin->GetBaseDir());
    OutPluginPythonPath = FPaths::Combine(OutPluginBaseDir, TEXT("Content"), TEXT("Python"), TEXT("mcp_server.py"));
    FPaths::NormalizeFilename(OutPluginBaseDir);
    FPaths::NormalizeFilename(OutPluginPythonPath);
    return true;
}

FString GetProjectCodexConfigPath()
{
    return FPaths::Combine(FPaths::ProjectDir(), TEXT(".codex"), TEXT("config.toml"));
}

FString GetGlobalCodexConfigPath()
{
    return FPaths::Combine(FPlatformProcess::UserHomeDir(), TEXT(".codex"), TEXT("config.toml"));
}

FString GetPreferredCodexConfigPathForPlugin(const FString& /*PluginBaseDir*/)
{
    return GetGlobalCodexConfigPath();
}

FString BuildCodexMcpConfigBlock(const FString& PluginPythonPath)
{
    return FString::Printf(
        TEXT("[mcp_servers.%s]\n")
        TEXT("command = \"python\"\n")
        TEXT("args = [\"%s\"]\n")
        TEXT("env = { UNREAL_HOST = \"%s\", UNREAL_PORT = \"%d\" }\n"),
        UnrealHandshakeServerName,
        *PluginPythonPath,
        DefaultUnrealHost,
        DefaultUnrealPort
    );
}

bool UpsertCodexMcpConfig(const FString& ConfigPath, const FString& NewBlock)
{
    FString ExistingConfig;
    if (FPaths::FileExists(ConfigPath) && !FFileHelper::LoadFileToString(ExistingConfig, *ConfigPath))
    {
        return false;
    }

    const FString Header = FString::Printf(TEXT("[mcp_servers.%s]"), UnrealHandshakeServerName);
    const int32 HeaderIndex = ExistingConfig.Find(Header, ESearchCase::CaseSensitive, ESearchDir::FromStart);
    FString UpdatedConfig = ExistingConfig;

    if (HeaderIndex != INDEX_NONE)
    {
        int32 BlockEndIndex = ExistingConfig.Len();
        for (int32 Index = HeaderIndex + Header.Len(); Index < ExistingConfig.Len() - 1; ++Index)
        {
            if (ExistingConfig[Index] == TEXT('\n') && ExistingConfig[Index + 1] == TEXT('['))
            {
                BlockEndIndex = Index + 1;
                break;
            }
        }

        UpdatedConfig = ExistingConfig.Left(HeaderIndex);
        if (!UpdatedConfig.IsEmpty() && !UpdatedConfig.EndsWith(TEXT("\n")))
        {
            UpdatedConfig.Append(TEXT("\n"));
        }
        if (!UpdatedConfig.IsEmpty() && !UpdatedConfig.EndsWith(TEXT("\n\n")))
        {
            UpdatedConfig.Append(TEXT("\n"));
        }

        UpdatedConfig.Append(NewBlock);

        if (BlockEndIndex < ExistingConfig.Len())
        {
            FString Suffix = ExistingConfig.Mid(BlockEndIndex);
            if (!Suffix.IsEmpty() && !Suffix.StartsWith(TEXT("\n")) && !UpdatedConfig.EndsWith(TEXT("\n")))
            {
                UpdatedConfig.Append(TEXT("\n"));
            }
            UpdatedConfig.Append(Suffix);
        }
    }
    else
    {
        if (!UpdatedConfig.IsEmpty() && !UpdatedConfig.EndsWith(TEXT("\n")))
        {
            UpdatedConfig.Append(TEXT("\n"));
        }
        if (!UpdatedConfig.IsEmpty() && !UpdatedConfig.EndsWith(TEXT("\n\n")))
        {
            UpdatedConfig.Append(TEXT("\n"));
        }
        UpdatedConfig.Append(NewBlock);
    }

    return FFileHelper::SaveStringToFile(UpdatedConfig, *ConfigPath);
}
}

FGenerativeAISupportEditorModule::FGenerativeAISupportEditorModule()
    : bSettingsRegistered(false)
{
    // Constructor initialization ensures proper state
}

void FGenerativeAISupportEditorModule::StartupModule()
{
    // Log to debug module loading
    UE_LOG(LogTemp, Log, TEXT("FGenerativeAISupportEditorModule::StartupModule called"));

    // Register project settings
    RegisterSettings();

    // Bootstrap defaults for new projects and Codex MCP integration.
    EnsureInitialProjectSetup();

    // Register menu extension
    RegisterMenuExtension();

    // Register tab spawner
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
                            GenEditorTabId,
                            FOnSpawnTab::CreateRaw(&FGenEditorWindowManager::Get(),
                                                   &FGenEditorWindowManager::SpawnEditorWindowTab))
                            .SetDisplayName(LOCTEXT("GenEditorWindowTitle", "Gen AI Support"))
                            .SetTooltipText(LOCTEXT("GenEditorWindowTooltip", "Open the Generative AI Support window"))
                            .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"))
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
            GetMutableDefault<UGenerativeAISupportSettings>()
        );

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

void FGenerativeAISupportEditorModule::EnsureInitialProjectSetup()
{
    EnsureDefaultSocketServerAutostart();
    EnsureCodexConfiguration();
}

void FGenerativeAISupportEditorModule::EnsureDefaultSocketServerAutostart()
{
    FString ConfigValue;
    const bool bHasExplicitConfig = GConfig->GetString(
        TEXT("/Script/GenerativeAISupportEditor.GenerativeAISupportSettings"),
        TEXT("bAutoStartSocketServer"),
        ConfigValue,
        GGameIni
    );

    if (bHasExplicitConfig)
    {
        return;
    }

    if (UGenerativeAISupportSettings* Settings = GetMutableDefault<UGenerativeAISupportSettings>())
    {
        Settings->bAutoStartSocketServer = true;
        Settings->SaveConfig();
        UE_LOG(LogTemp, Log, TEXT("Enabled GenerativeAISupport socket server auto-start for this project."));
    }
}

void FGenerativeAISupportEditorModule::EnsureCodexConfiguration()
{
    FString PluginBaseDir;
    FString PluginPythonPath;
    if (!ResolvePluginPaths(PluginBaseDir, PluginPythonPath))
    {
        UE_LOG(LogTemp, Warning, TEXT("GenerativeAISupport plugin path could not be resolved for Codex MCP setup."));
        return;
    }

    const FString PreferredConfigPath = GetPreferredCodexConfigPathForPlugin(PluginBaseDir);

    const FString ConfigDir = FPaths::GetPath(PreferredConfigPath);
    if (!FPaths::DirectoryExists(ConfigDir))
    {
        IFileManager::Get().MakeDirectory(*ConfigDir, true);
    }

    if (UpsertCodexMcpConfig(PreferredConfigPath, BuildCodexMcpConfigBlock(PluginPythonPath)))
    {
        UE_LOG(LogTemp, Log, TEXT("Updated global Codex MCP config at: %s"), *PreferredConfigPath);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to update global Codex MCP config at: %s"), *PreferredConfigPath);
    }
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
        "WindowLayout",
        EExtensionHook::After,
        PluginCommands,
        FMenuExtensionDelegate::CreateLambda([](FMenuBuilder& Builder) {
            Builder.AddMenuEntry(FGenEditorCommands::Get().OpenGenEditorWindow);
        }));
    
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
