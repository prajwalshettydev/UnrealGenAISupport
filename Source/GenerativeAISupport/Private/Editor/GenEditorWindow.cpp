#include "Editor/GenEditorWindow.h"
#include "GenerativeAISupportSettings.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Styling/SlateStyleRegistry.h"
#include "EditorStyleSet.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "LevelEditor.h"
#include "Modules/ModuleManager.h"
#include "HAL/PlatformProcess.h"
#include "HAL/FileManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "Widgets/Images/SImage.h"
#include "Styling/CoreStyle.h"
#include "ISourceControlModule.h"
#include "ISourceControlProvider.h"
#include "SourceControlHelpers.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Misc/FileHelper.h"
#include "Misc/MessageDialog.h"
#include "DesktopPlatformModule.h"
#include "ISettingsModule.h"
#include "Data/GenAIOrgs.h"
#include "Interfaces/IPluginManager.h"
#include "Secure/GenSecureKey.h"

// Static member initialization
FGenEditorWindowManager* FGenEditorWindowManager::Singleton = nullptr;
const FName FGenEditorWindowManager::TabId = FName("GenEditorWindow");

FGenEditorWindowManager& FGenEditorWindowManager::Get()
{
    if (!Singleton)
    {
        Singleton = new FGenEditorWindowManager();
    }
    return *Singleton;
}

void FGenEditorWindowManager::RegisterTabSpawner(const TSharedPtr<FTabManager>& TabManager)
{
    TabManager->RegisterTabSpawner(TabId, FOnSpawnTab::CreateRaw(this, &FGenEditorWindowManager::SpawnEditorWindowTab))
        .SetDisplayName(NSLOCTEXT("GenerativeAISupport", "TabTitle", "Gen AI Support"))
        .SetTooltipText(NSLOCTEXT("GenerativeAISupport", "TabTooltip", "Open the Generative AI Support window"))
        .SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Details"));
}

void FGenEditorWindowManager::UnregisterTabSpawner(const TSharedPtr<FTabManager>& TabManager)
{
    TabManager->UnregisterTabSpawner(TabId);
}

TSharedRef<SDockTab> FGenEditorWindowManager::SpawnEditorWindowTab(const FSpawnTabArgs& Args)
{
    TSharedRef<SDockTab> Tab = SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            SNew(SGenEditorWindow)
        ];

    return Tab;
}

void SGenEditorWindow::Construct(const FArguments& InArgs)
{
    // Main layout
    ChildSlot
    [
        SNew(SVerticalBox)
        // Title
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(10)
        [
            SNew(STextBlock)
            .Text(NSLOCTEXT("GenerativeAISupport", "PluginTitle", "Generative AI Support"))
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
        ]
        // Status sections
        + SVerticalBox::Slot()
        .FillHeight(1.0f)
        [
            SNew(SScrollBox)
            // MCP Status Section
            + SScrollBox::Slot()
            .Padding(10)
            [
                SNew(SBox)
                .WidthOverride(800) // Increased width to accommodate more columns
                [
                    CreateMCPStatusSection()
                ]
            ]
            // API Status Section
            + SScrollBox::Slot()
            .Padding(10)
            [
                SNew(SBox)
                .WidthOverride(800) // Increased width to accommodate more columns
                [
                    CreateAPIStatusSection()
                ]
            ]
            // Action buttons section
            + SScrollBox::Slot()
            .Padding(10)
            [
                SNew(SBox)
                .WidthOverride(800) // Increased width to accommodate more columns
                [
                    CreateActionButtonsSection()
                ]
            ]
        ]
        // Bottom buttons
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(10)
        .HAlign(HAlign_Right)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(5, 0)
            [
                SNew(SButton)
                .Text(NSLOCTEXT("GenerativeAISupport", "RefreshButton", "Refresh Status"))
                .OnClicked_Lambda([this]() {
                    RefreshStatus();
                    return FReply::Handled();
                })
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(5, 0)
            [
                SNew(SButton)
                .Text(NSLOCTEXT("GenerativeAISupport", "ConfigButton", "Open Settings"))
                .OnClicked_Lambda([]() {
                    FModuleManager::LoadModuleChecked<ISettingsModule>("Settings").ShowViewer("Project", "Plugins", "GenerativeAISupport");
                    return FReply::Handled();
                })
            ]
        ]
    ];

    // Set up timer to refresh the status periodically
    if (GEditor && GEditor->GetEditorWorldContext().World())
    {
        GEditor->GetEditorWorldContext().World()->GetTimerManager().SetTimer(
            StatusRefreshTimerHandle,
            FTimerDelegate::CreateSP(this, &SGenEditorWindow::RefreshStatus),
            5.0f,  // Refresh every 5 seconds
            true);
    }
    
    // Initial refresh
    RefreshStatus();
}

TSharedRef<SWidget> SGenEditorWindow::CreateMCPStatusSection()
{
    return SNew(SBorder)
    .BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
    .Padding(8)
    [
        SNew(SVerticalBox)
        // Section Title
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0, 0, 0, 10)
        [
            SNew(STextBlock)
            .Text(NSLOCTEXT("GenerativeAISupport", "MCPStatusTitle", "MCP Status"))
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
        ]
        // Column headers
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0, 0, 0, 5)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(0, 0, 10, 0)
            [
                SNew(STextBlock)
                .Text(NSLOCTEXT("GenerativeAISupport", "ComponentHeader", "Component"))
                .MinDesiredWidth(150)
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(0, 0, 10, 0)
            [
                SNew(STextBlock)
                .Text(NSLOCTEXT("GenerativeAISupport", "StatusHeader", "Status"))
                .MinDesiredWidth(100)
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
            ]
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .VAlign(VAlign_Center)
            .Padding(0, 0, 10, 0)
            [
                SNew(STextBlock)
                .Text(NSLOCTEXT("GenerativeAISupport", "DetailsHeader", "Details"))
                .MinDesiredWidth(150)
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Text(NSLOCTEXT("GenerativeAISupport", "ActionHeader", "Action"))
                .MinDesiredWidth(100)
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
            ]
        ]
        // Separator line
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0, 0, 0, 10)
        [
            SNew(SBox)
            .HeightOverride(1.0f)
            [
                SNew(SBorder)
                .BorderImage(FEditorStyle::GetBrush("Menu.Separator"))
                .Padding(FMargin(0.0f))
            ]
        ]
        // Unreal Socket Server Status
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0, 2)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(0, 0, 10, 0)
            [
                SNew(STextBlock)
                .Text(NSLOCTEXT("GenerativeAISupport", "UnrealSocketLabel", "Unreal Socket Server"))
                .MinDesiredWidth(150)
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(0, 0, 10, 0)
            [
                SAssignNew(UnrealSocketStatusText, STextBlock)
                .Text(NSLOCTEXT("GenerativeAISupport", "CheckingStatus", "Checking..."))
                .MinDesiredWidth(100)
            ]
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .VAlign(VAlign_Center)
            .Padding(0, 0, 10, 0)
            [
                SAssignNew(UnrealSocketDetailsText, STextBlock)
                .Text(NSLOCTEXT("GenerativeAISupport", "NoDetails", ""))
                .MinDesiredWidth(150)
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .HAlign(HAlign_Right)
            [
                SNew(SBox)
                .WidthOverride(100)
                .Visibility_Lambda([this]() -> EVisibility { 
                    return IsUnrealSocketServerRunning() ? EVisibility::Collapsed : EVisibility::Visible; 
                })
                [
                    SNew(SButton)
                    .Text(NSLOCTEXT("GenerativeAISupport", "StartButton", "Start"))
                    .OnClicked_Lambda([this]() {
                        // Enable auto-start in settings and start the server
                        UGenerativeAISupportSettings* Settings = GetMutableDefault<UGenerativeAISupportSettings>();
                        if (Settings)
                        {
                            Settings->bAutoStartSocketServer = true;
                            Settings->SaveConfig();
                            
                            // Attempt to actually start the server
                            FNotificationInfo Info(NSLOCTEXT("GenerativeAISupport", "SocketServerStarted", "Socket server auto-start enabled. The server will start with the editor."));
                            Info.ExpireDuration = 5.0f;
                            FSlateNotificationManager::Get().AddNotification(Info);
                            
                            RefreshStatus();
                        }
                        return FReply::Handled();
                    })
                    .ToolTipText(NSLOCTEXT("GenerativeAISupport", "StartSocketTooltip", "Enable auto-start for the Unreal Socket Server"))
                ]
            ]
        ]
        // MCP Server Status
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0, 2)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(0, 0, 10, 0)
            [
                SNew(STextBlock)
                .Text(NSLOCTEXT("GenerativeAISupport", "MCPServerLabel", "MCP Server"))
                .MinDesiredWidth(150)
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(0, 0, 10, 0)
            [
                SAssignNew(MCPServerStatusText, STextBlock)
                .Text(NSLOCTEXT("GenerativeAISupport", "CheckingStatus", "Checking..."))
                .MinDesiredWidth(100)
            ]
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .VAlign(VAlign_Center)
            .Padding(0, 0, 10, 0)
            [
                SAssignNew(MCPServerDetailsText, STextBlock)
                .Text(NSLOCTEXT("GenerativeAISupport", "NoDetails", ""))
                .MinDesiredWidth(150)
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .HAlign(HAlign_Right)
            [
                SNew(SBox)
                .WidthOverride(100)
                .Visibility_Lambda([this]() -> EVisibility { 
                    return IsMCPServerRunning() ? EVisibility::Collapsed : EVisibility::Visible; 
                })
                [
                    SNew(SButton)
                    .Text(NSLOCTEXT("GenerativeAISupport", "SetupButton", "Setup"))
                    .OnClicked_Lambda([this]() {
                        const FText Title = NSLOCTEXT("GenerativeAISupport", "MCPServerSetupTitle", "Setup MCP Server");
                        const FText Message = NSLOCTEXT("GenerativeAISupport", "MCPServerSetupMessage", 
                            "To use MCP Server with AI tools, you need to set up at least one of the following:\n\n"
                            "- Set up Claude configuration\n"
                            "- Set up Cursor configuration\n\n"
                            "After setup, you'll need to restart Claude or Cursor to activate the MCP Server.");
                            
                        FMessageDialog::Open(EAppMsgType::Ok, Message, &Title);
                        return FReply::Handled();
                    })
                    .ToolTipText(NSLOCTEXT("GenerativeAISupport", "SetupMCPTooltip", "Set up MCP Server configuration"))
                ]
            ]
        ]
        // Claude Status
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0, 2)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(0, 0, 10, 0)
            [
                SNew(STextBlock)
                .Text(NSLOCTEXT("GenerativeAISupport", "ClaudeLabel", "Claude"))
                .MinDesiredWidth(150)
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(0, 0, 10, 0)
            [
                SAssignNew(ClaudeStatusText, STextBlock)
                .Text(NSLOCTEXT("GenerativeAISupport", "CheckingStatus", "Checking..."))
                .MinDesiredWidth(100)
            ]
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .VAlign(VAlign_Center)
            .Padding(0, 0, 10, 0)
            [
                SAssignNew(ClaudeDetailsText, STextBlock)
                .Text(NSLOCTEXT("GenerativeAISupport", "NoDetails", ""))
                .MinDesiredWidth(150)
                .ToolTipText_Lambda([this]() {
                    return FText::FromString(GetClaudeConfigPath());
                })
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .HAlign(HAlign_Right)
            [
                SNew(SButton)
                .Text(NSLOCTEXT("GenerativeAISupport", "OpenButton", "Open"))
                .OnClicked_Lambda([this]() {
                    OpenClaudeConfig();
                    return FReply::Handled();
                })
                .IsEnabled_Lambda([this]() {
                    return FPaths::FileExists(GetClaudeConfigPath());
                })
            ]
        ]
        // Cursor Status
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0, 2)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(0, 0, 10, 0)
            [
                SNew(STextBlock)
                .Text(NSLOCTEXT("GenerativeAISupport", "CursorLabel", "Cursor"))
                .MinDesiredWidth(150)
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(0, 0, 10, 0)
            [
                SAssignNew(CursorStatusText, STextBlock)
                .Text(NSLOCTEXT("GenerativeAISupport", "CheckingStatus", "Checking..."))
                .MinDesiredWidth(100)
            ]
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .VAlign(VAlign_Center)
            .Padding(0, 0, 10, 0)
            [
                SAssignNew(CursorDetailsText, STextBlock)
                .Text(NSLOCTEXT("GenerativeAISupport", "NoDetails", ""))
                .MinDesiredWidth(150)
                .ToolTipText_Lambda([this]() {
                    return FText::FromString(GetCursorConfigPath());
                })
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .HAlign(HAlign_Right)
            [
                SNew(SButton)
                .Text(NSLOCTEXT("GenerativeAISupport", "OpenButton", "Open"))
                .OnClicked_Lambda([this]() {
                    OpenCursorConfig();
                    return FReply::Handled();
                })
                .IsEnabled_Lambda([this]() {
                    return FPaths::FileExists(GetCursorConfigPath());
                })
            ]
        ]
    ];
}

TSharedRef<SWidget> SGenEditorWindow::CreateAPIStatusSection()
{
    return SNew(SBorder)
    .BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
    .Padding(8)
    [
        SNew(SVerticalBox)
        // Section Title
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0, 0, 0, 10)
        [
            SNew(STextBlock)
            .Text(NSLOCTEXT("GenerativeAISupport", "APIStatusTitle", "API Keys Status"))
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
        ]
        // Column headers
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0, 0, 0, 5)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(0, 0, 10, 0)
            [
                SNew(STextBlock)
                .Text(NSLOCTEXT("GenerativeAISupport", "ProviderHeader", "Provider"))
                .MinDesiredWidth(150)
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(0, 0, 10, 0)
            [
                SNew(STextBlock)
                .Text(NSLOCTEXT("GenerativeAISupport", "StatusHeader", "Status"))
                .MinDesiredWidth(100)
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
            ]
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Text(NSLOCTEXT("GenerativeAISupport", "APIKeyPreviewHeader", "API Key Preview"))
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
            ]
        ]
        // Separator line
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0, 0, 0, 10)
        [
            SNew(SBox)
            .HeightOverride(1.0f)
            [
                SNew(SBorder)
                .BorderImage(FEditorStyle::GetBrush("Menu.Separator"))
                .Padding(FMargin(0.0f))
            ]
        ]
        // API Keys Status Container
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SAssignNew(APIStatusContainer, SVerticalBox)
        ]
    ];
}

TSharedRef<SWidget> SGenEditorWindow::CreateActionButtonsSection()
{
    return SNew(SBorder)
    .BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
    .Padding(8)
    [
        SNew(SVerticalBox)
        // Section Title
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0, 0, 0, 10)
        [
            SNew(STextBlock)
            .Text(NSLOCTEXT("GenerativeAISupport", "SetupActionsTitle", "Setup Actions"))
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
        ]
        // Setup Claude Config Button
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0, 4)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            [
                SNew(STextBlock)
                .Text(NSLOCTEXT("GenerativeAISupport", "ClaudeConfigLabel", "Create/Update Claude Configuration"))
                .ToolTipText(NSLOCTEXT("GenerativeAISupport", "ClaudeConfigTooltip", "Creates or updates Claude's configuration file to work with this plugin"))
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(10, 0, 0, 0)
            [
                SNew(SButton)
                .Text(NSLOCTEXT("GenerativeAISupport", "SetupClaudeButton", "Setup Claude"))
                .OnClicked(this, &SGenEditorWindow::SetupClaudeConfig)
            ]
        ]
        // Setup Cursor Config Button
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0, 4)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            [
                SNew(STextBlock)
                .Text(NSLOCTEXT("GenerativeAISupport", "CursorConfigLabel", "Create/Update Cursor Configuration"))
                .ToolTipText(NSLOCTEXT("GenerativeAISupport", "CursorConfigTooltip", "Creates or updates Cursor's MCP configuration file to work with this plugin"))
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(10, 0, 0, 0)
            [
                SNew(SButton)
                .Text(NSLOCTEXT("GenerativeAISupport", "SetupCursorButton", "Setup Cursor"))
                .OnClicked(this, &SGenEditorWindow::SetupCursorConfig)
            ]
        ]
    ];
}

void SGenEditorWindow::RefreshStatus()
{
    // Check Unreal Socket Server status
    bool bUnrealSocketRunning = IsUnrealSocketServerRunning();
    UnrealSocketStatusText->SetText(
        bUnrealSocketRunning 
        ? NSLOCTEXT("GenerativeAISupport", "StatusRunning", "Running ✓") 
        : NSLOCTEXT("GenerativeAISupport", "StatusNotRunning", "Not Running ❌")
    );
    UnrealSocketStatusText->SetColorAndOpacity(
        bUnrealSocketRunning 
        ? FLinearColor(0.0f, 0.8f, 0.0f) 
        : NotConfiguredColor
    );
    
    // Set details about the socket server
    FString SocketDetails = bUnrealSocketRunning ? TEXT("localhost:9877") : TEXT("");
    UnrealSocketDetailsText->SetText(FText::FromString(SocketDetails));

    // Check MCP Server status
    bool bMCPServerRunning = IsMCPServerRunning();
    MCPServerStatusText->SetText(
        bMCPServerRunning 
        ? NSLOCTEXT("GenerativeAISupport", "StatusRunning", "Running ✓") 
        : NSLOCTEXT("GenerativeAISupport", "StatusNotRunning", "Not Running ❌")
    );
    MCPServerStatusText->SetColorAndOpacity(
        bMCPServerRunning 
        ? FLinearColor(0.0f, 0.8f, 0.0f) 
        : NotConfiguredColor
    );
    
    // Set details about the MCP server
    FString MCPDetails = bMCPServerRunning ? TEXT("UnrealHandshake") : TEXT("");
    MCPServerDetailsText->SetText(FText::FromString(MCPDetails));

    // Check Claude configuration
    bool bClaudeConfigured = IsClaudeConfigured();
    ClaudeStatusText->SetText(
        bClaudeConfigured 
        ? NSLOCTEXT("GenerativeAISupport", "StatusConfigured", "Configured ✓") 
        : NSLOCTEXT("GenerativeAISupport", "StatusNotConfigured", "Not Configured ❌")
    );
    ClaudeStatusText->SetColorAndOpacity(
        bClaudeConfigured 
        ? FLinearColor(0.0f, 0.8f, 0.0f) 
        : NotConfiguredColor
    );
    
    // Set details about the Claude config
    FString ClaudeConfigPath = GetClaudeConfigPath();
    FString ClaudeDetails = bClaudeConfigured ? ShortenPath(ClaudeConfigPath) : TEXT("");
    ClaudeDetailsText->SetText(FText::FromString(ClaudeDetails));

    // Check Cursor configuration
    bool bCursorConfigured = IsCursorConfigured();
    CursorStatusText->SetText(
        bCursorConfigured 
        ? NSLOCTEXT("GenerativeAISupport", "StatusConfigured", "Configured ✓") 
        : NSLOCTEXT("GenerativeAISupport", "StatusNotConfigured", "Not Configured ❌")
    );
    CursorStatusText->SetColorAndOpacity(
        bCursorConfigured 
        ? FLinearColor(0.0f, 0.8f, 0.0f) 
        : NotConfiguredColor
    );
    
    // Set details about the Cursor config
    FString CursorConfigPath = GetCursorConfigPath();
    FString CursorDetails = bCursorConfigured ? ShortenPath(CursorConfigPath) : TEXT("");
    CursorDetailsText->SetText(FText::FromString(CursorDetails));

    // Refresh API keys status
    APIStatusContainer->ClearChildren();
    
    // Create a map for EGenAIOrgs enum values and their display names
    TMap<EGenAIOrgs, FString> OrgDisplayNames;
    OrgDisplayNames.Add(EGenAIOrgs::OpenAI, "OpenAI");
    OrgDisplayNames.Add(EGenAIOrgs::Anthropic, "Anthropic");
    OrgDisplayNames.Add(EGenAIOrgs::Google, "Google");
    OrgDisplayNames.Add(EGenAIOrgs::Meta, "Meta");
    OrgDisplayNames.Add(EGenAIOrgs::DeepSeek, "DeepSeek");
    OrgDisplayNames.Add(EGenAIOrgs::XAI, "XAI");

    // Add API key status rows using the GenSecureKey system
    for (const auto& OrgPair : OrgDisplayNames)
    {
        EGenAIOrgs Org = OrgPair.Key;
        FString OrgName = OrgPair.Value;
        
        bool bKeySet = false;
        FString ApiKeyPreview = GetAPIKeyPreview(Org, bKeySet);
        
        APIStatusContainer->AddSlot()
        .AutoHeight()
        .Padding(0, 2)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(0, 0, 10, 0)
            [
                SNew(STextBlock)
                .Text(FText::FromString(OrgName))
                .MinDesiredWidth(150)
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(0, 0, 10, 0)
            [
                SNew(STextBlock)
                .Text(
                    bKeySet 
                    ? NSLOCTEXT("GenerativeAISupport", "StatusConfigured", "Configured ✓") 
                    : NSLOCTEXT("GenerativeAISupport", "StatusNotConfigured", "Not Configured ❌")
                )
                .ColorAndOpacity(
                    bKeySet 
                    ? FLinearColor(0.0f, 0.8f, 0.0f) 
                    : NotConfiguredColor
                )
                .MinDesiredWidth(100)
            ]
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Text(FText::FromString(ApiKeyPreview))
                .Font(FCoreStyle::GetDefaultFontStyle("Mono", 9))
            ]
        ];
    }
}

bool SGenEditorWindow::IsUnrealSocketServerRunning() const
{
    UGenerativeAISupportSettings* Settings = GetMutableDefault<UGenerativeAISupportSettings>();
    if (!Settings || !Settings->bAutoStartSocketServer)
    {
        return false;
    }
    
    // TODO: Add actual socket connectivity check here
    return true;
}

bool SGenEditorWindow::IsMCPServerRunning() const
{
    return FPlatformProcess::IsApplicationRunning(TEXT("python")) &&
           (IsClaudeConfigured() || IsCursorConfigured());
}

bool SGenEditorWindow::IsClaudeConfigured() const
{
    FString ClaudeConfigPath = GetClaudeConfigPath();
    if (FPaths::FileExists(ClaudeConfigPath))
    {
        FString ConfigContent;
        if (FFileHelper::LoadFileToString(ConfigContent, *ClaudeConfigPath))
        {
            return ConfigContent.Contains(TEXT("unreal-handshake"));
        }
    }
    return false;
}

FString SGenEditorWindow::GetClaudeConfigPath() const
{
    return FPaths::Combine(FPlatformProcess::UserSettingsDir(), TEXT("Claude"), TEXT("claude_desktop_config.json"));
}

void SGenEditorWindow::OpenClaudeConfig() const
{
    FString ClaudeConfigPath = GetClaudeConfigPath();
    if (FPaths::FileExists(ClaudeConfigPath))
    {
        FPlatformProcess::LaunchFileInDefaultExternalApplication(*ClaudeConfigPath);
    }
    else
    {
        FString DirectoryPath = FPaths::GetPath(ClaudeConfigPath);
        FMessageDialog::Open(EAppMsgType::Ok, FText::Format(
            NSLOCTEXT("GenerativeAISupport", "FileNotFoundDetail", "Claude config file not found:\n{0}\n\nWe'll create it in this location when you click 'Setup Claude'"),
            FText::FromString(ClaudeConfigPath)
        ));
    }
}

FReply SGenEditorWindow::SetupClaudeConfig()
{
    FString ClaudeConfigPath = GetClaudeConfigPath();
    FString ClaudeConfigDir = FPaths::GetPath(ClaudeConfigPath);
    
    if (!FPaths::DirectoryExists(ClaudeConfigDir))
    {
        IFileManager::Get().MakeDirectory(*ClaudeConfigDir, true);
    }
    
    FString PluginPythonPath;
    TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("GenerativeAISupport"));
    if (Plugin.IsValid())
    {
        PluginPythonPath = FPaths::ConvertRelativePathToFull(
            FPaths::Combine(Plugin->GetBaseDir(), TEXT("Content"), TEXT("Python"), TEXT("mcp_server.py"))
        );
        PluginPythonPath = PluginPythonPath.Replace(TEXT("\\"), TEXT("/"));
    }
    else
    {
        FMessageDialog::Open(EAppMsgType::Ok, NSLOCTEXT("GenerativeAISupport", "PluginNotFound", "GenerativeAISupport plugin not found!"));
        return FReply::Handled();
    }
    
    FString ConfigJson = FString::Printf(TEXT("{\n")
        TEXT("  \"mcpServers\": {\n")
        TEXT("    \"unreal-handshake\": {\n")
        TEXT("      \"command\": \"python\",\n")
        TEXT("      \"args\": [\"%s\"],\n")
        TEXT("      \"env\": {\n")
        TEXT("        \"UNREAL_HOST\": \"localhost\",\n")
        TEXT("        \"UNREAL_PORT\": \"9877\"\n")
        TEXT("      }\n")
        TEXT("    }\n")
        TEXT("  }\n")
        TEXT("}"), *PluginPythonPath);
    
    bool bSuccess = FFileHelper::SaveStringToFile(ConfigJson, *ClaudeConfigPath);
    
    if (bSuccess)
    {
        FNotificationInfo Info(FText::Format(
            NSLOCTEXT("GenerativeAISupport", "ClaudeConfigSuccessDetail", "Claude configuration updated successfully at:\n{0}"),
            FText::FromString(ClaudeConfigPath)
        ));
        Info.ExpireDuration = 7.0f;
        FSlateNotificationManager::Get().AddNotification(Info);
        OpenClaudeConfig();
    }
    else
    {
        FNotificationInfo Info(FText::Format(
            NSLOCTEXT("GenerativeAISupport", "ClaudeConfigFailedDetail", "Failed to update Claude configuration at:\n{0}\nPlease check folder permissions."),
            FText::FromString(ClaudeConfigPath)
        ));
        Info.ExpireDuration = 7.0f;
        FSlateNotificationManager::Get().AddNotification(Info);
    }
    
    RefreshStatus();
    return FReply::Handled();
}

bool SGenEditorWindow::IsCursorConfigured() const
{
    FString CursorConfigPath = GetCursorConfigPath();
    if (FPaths::FileExists(CursorConfigPath))
    {
        FString ConfigContent;
        if (FFileHelper::LoadFileToString(ConfigContent, *CursorConfigPath))
        {
            return ConfigContent.Contains(TEXT("unreal-handshake"));
        }
    }
    return false;
}

FString SGenEditorWindow::GetCursorConfigPath() const
{
    return FPaths::Combine(FPlatformProcess::UserHomeDir(), TEXT(".cursor"), TEXT("mcp.json"));
}

void SGenEditorWindow::OpenCursorConfig() const
{
    FString CursorConfigPath = GetCursorConfigPath();
    if (FPaths::FileExists(CursorConfigPath))
    {
        FPlatformProcess::LaunchFileInDefaultExternalApplication(*CursorConfigPath);
    }
    else
    {
        FString DirectoryPath = FPaths::GetPath(CursorConfigPath);
        FMessageDialog::Open(EAppMsgType::Ok, FText::Format(
            NSLOCTEXT("GenerativeAISupport", "FileNotFoundDetail", "Cursor config file not found:\n{0}\n\nWe'll create it in this location when you click 'Setup Cursor'"),
            FText::FromString(CursorConfigPath)
        ));
    }
}

FReply SGenEditorWindow::SetupCursorConfig()
{
    FString CursorConfigPath = GetCursorConfigPath();
    FString CursorConfigDir = FPaths::GetPath(CursorConfigPath);
    
    if (!FPaths::DirectoryExists(CursorConfigDir))
    {
        IFileManager::Get().MakeDirectory(*CursorConfigDir, true);
    }
    
    FString PluginPythonPath;
    TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("GenerativeAISupport"));
    if (Plugin.IsValid())
    {
        PluginPythonPath = FPaths::ConvertRelativePathToFull(
            FPaths::Combine(Plugin->GetBaseDir(), TEXT("Content"), TEXT("Python"), TEXT("mcp_server.py"))
        );
        PluginPythonPath = PluginPythonPath.Replace(TEXT("\\"), TEXT("/"));
    }
    else
    {
        FMessageDialog::Open(EAppMsgType::Ok, NSLOCTEXT("GenerativeAISupport", "PluginNotFound", "GenerativeAISupport plugin not found!"));
        return FReply::Handled();
    }
    
    FString ConfigJson = FString::Printf(TEXT("{\n")
        TEXT("  \"mcpServers\": {\n")
        TEXT("    \"unreal-handshake\": {\n")
        TEXT("      \"command\": \"python\",\n")
        TEXT("      \"args\": [\"%s\"],\n")
        TEXT("      \"env\": {\n")
        TEXT("        \"UNREAL_HOST\": \"localhost\",\n")
        TEXT("        \"UNREAL_PORT\": \"9877\"\n")
        TEXT("      }\n")
        TEXT("    }\n")
        TEXT("  }\n")
        TEXT("}"), *PluginPythonPath);
    
    bool bSuccess = FFileHelper::SaveStringToFile(ConfigJson, *CursorConfigPath);
    
    if (bSuccess)
    {
        FNotificationInfo Info(FText::Format(
            NSLOCTEXT("GenerativeAISupport", "CursorConfigSuccessDetail", "Cursor configuration updated successfully at:\n{0}"),
            FText::FromString(CursorConfigPath)
        ));
        Info.ExpireDuration = 7.0f;
        FSlateNotificationManager::Get().AddNotification(Info);
        OpenCursorConfig();
    }
    else
    {
        FNotificationInfo Info(FText::Format(
            NSLOCTEXT("GenerativeAISupport", "CursorConfigFailedDetail", "Failed to update Cursor configuration at:\n{0}\nPlease check folder permissions."),
            FText::FromString(CursorConfigPath)
        ));
        Info.ExpireDuration = 7.0f;
        FSlateNotificationManager::Get().AddNotification(Info);
    }
    
    RefreshStatus();
    return FReply::Handled();
}

FString SGenEditorWindow::GetAPIKeyPreview(EGenAIOrgs Org, bool& bKeySet) const
{
    UGenSecureKey* SecureKey = GetMutableDefault<UGenSecureKey>();
    if (SecureKey)
    {
        FString ApiKey = SecureKey->GetGenerativeAIApiKey(Org);
        bKeySet = !ApiKey.IsEmpty();
        
        if (bKeySet)
        {
            int32 KeyLength = ApiKey.Len();
            int32 PreviewLength = FMath::Min(4, KeyLength);
            
            FString KeyPreview = ApiKey.Left(PreviewLength);
            for (int32 i = 0; i < FMath::Min(8, KeyLength - PreviewLength); ++i)
            {
                KeyPreview.AppendChar(TEXT('*'));
            }
            
            return KeyPreview;
        }
    }
    
    bKeySet = false;
    return TEXT("");
}

FString SGenEditorWindow::ShortenPath(const FString& Path) const
{
    static const int32 MaxPathLength = 40;
    
    if (Path.Len() <= MaxPathLength)
    {
        return Path;
    }
    
    int32 Count = 0;
    int32 SlashPos = Path.Len();
    
    for (int32 i = 0; i < 3; ++i)
    {
        int32 Temp = SlashPos - 1;
        bool bFound = Path.FindLastChar(TEXT('/'), Temp);
        if (!bFound)
        {
            SlashPos = 0;
            break;
        }
        SlashPos = Temp;
    }
    
    if (SlashPos <= 0)
    {
        return TEXT("...") + Path.Right(MaxPathLength - 3);
    }
    
    return TEXT("...") + Path.Mid(SlashPos);
}
