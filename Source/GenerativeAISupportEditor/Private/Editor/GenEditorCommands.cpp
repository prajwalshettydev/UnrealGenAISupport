// Fill out your copyright notice in the Description page of Project Settings.


#include "Editor/GenEditorCommands.h"

#define LOCTEXT_NAMESPACE "GenEditorCommands"

void FGenEditorCommands::RegisterCommands()
{
	UI_COMMAND(
		OpenGenEditorWindow,
		"Gen AI Support",
		"Open the Generative AI Support window",
		EUserInterfaceActionType::Button,
		FInputChord());
}

#undef LOCTEXT_NAMESPACE
