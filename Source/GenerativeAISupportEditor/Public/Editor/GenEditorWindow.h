// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Docking/SDockTab.h"

// Forward declarations
enum class EGenAIOrgs : uint8;

class SGenEditorWindow : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SGenEditorWindow) {}
    SLATE_END_ARGS()

    /** Widget constructor */
    void Construct(const FArguments& InArgs);

    /** Refreshes the status display */
    void RefreshStatus();

private:
    /** Creates the MCP status section */
    TSharedRef<SWidget> CreateMCPStatusSection();

    /** Creates the API status section */
    TSharedRef<SWidget> CreateAPIStatusSection();

    /** Creates action buttons section */
    TSharedRef<SWidget> CreateActionButtonsSection();

    /** Checks if Unreal socket server is running */
    bool IsUnrealSocketServerRunning() const;

    /** Checks if MCP server is running */
    bool IsMCPServerRunning() const;

    /** Checks if Claude is configured */
    bool IsClaudeConfigured() const;

    /** Gets the Claude config path */
    FString GetClaudeConfigPath() const;
    
    /** Opens the Claude config file */
    void OpenClaudeConfig() const;

    /** Attempts to create/update Claude configuration */
    FReply SetupClaudeConfig();

    /** Checks if Cursor is configured */
    bool IsCursorConfigured() const;
    
    /** Gets the Cursor config path */
    FString GetCursorConfigPath() const;
    
    /** Opens the Cursor config file */
    void OpenCursorConfig() const;

    /** Attempts to create/update Cursor configuration */
    FReply SetupCursorConfig();

    /** Checks if a specific API key is set using the GenSecureKey system */
    bool IsAPIKeySet(EGenAIOrgs Org) const;
    
    /** Gets a preview of the API key (first few chars + asterisks) */
    FString GetAPIKeyPreview(EGenAIOrgs Org, bool& bKeySet) const;
    
    /** Shortens a file path for display */
    FString ShortenPath(const FString& Path) const;

    /** Status widgets that need to be updated */
    TSharedPtr<STextBlock> UnrealSocketStatusText;
    TSharedPtr<STextBlock> UnrealSocketDetailsText;
    TSharedPtr<STextBlock> MCPServerStatusText;
    TSharedPtr<STextBlock> MCPServerDetailsText;
    TSharedPtr<STextBlock> ClaudeStatusText;
    TSharedPtr<STextBlock> ClaudeDetailsText;
    TSharedPtr<STextBlock> CursorStatusText;
    TSharedPtr<STextBlock> CursorDetailsText;
    TSharedPtr<SVerticalBox> APIStatusContainer;

    /** Timer handle for periodically refreshing status */
    FTimerHandle StatusRefreshTimerHandle;
    
    /** Color for "not configured" status - using a milder color instead of red */
    FLinearColor NotConfiguredColor = FLinearColor(0.7f, 0.7f, 0.0f); // Amber/Yellow
};

/**
 * Manages the Generative AI Support editor window
 */
class FGenEditorWindowManager
{
public:
    /** Registers the editor window tab */
    void RegisterTabSpawner(const TSharedPtr<FTabManager>& TabManager);
    
    /** Unregisters the editor window tab */
    void UnregisterTabSpawner(const TSharedPtr<FTabManager>& TabManager);
    
    /** Creates the editor window tab */
    TSharedRef<SDockTab> SpawnEditorWindowTab(const FSpawnTabArgs& Args);
    
    /** Gets the singleton instance */
    static FGenEditorWindowManager& Get();
    
private:
    /** Singleton instance */
    static FGenEditorWindowManager* Singleton;
    
    /** Tab ID for the editor window */
    static const FName TabId;
};