// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditorStyleSet.h"
#include "Framework/Commands/Commands.h"

/**
 * 
 */
class FGenEditorCommands : public TCommands<FGenEditorCommands>
{
public:
 FGenEditorCommands()
     : TCommands<FGenEditorCommands>(
         TEXT("GenerativeAISupport"), // Context name for fast lookup
         NSLOCTEXT("Contexts", "GenerativeAISupport", "Generative AI Support Plugin"), // Localized context name for displaying
         NAME_None, // Parent context name
         FEditorStyle::GetStyleSetName() // Style set name
     )
 {
 }

 // TCommands<> interface
 virtual void RegisterCommands() override;

 // Command to open the editor window
 TSharedPtr<FUICommandInfo> OpenGenEditorWindow;
};
