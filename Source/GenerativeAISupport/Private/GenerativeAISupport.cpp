// Copyright Epic Games, Inc. All Rights Reserved.

#include "GenerativeAISupport.h"

#define LOCTEXT_NAMESPACE "FGenerativeAISupportModule"

FGenerativeAISupportModule::FGenerativeAISupportModule()
{
	// Constructor initialization ensures proper state
}

void FGenerativeAISupportModule::StartupModule()
{
	// Log to debug module loading
	UE_LOG(LogTemp, Log, TEXT("FGenerativeAISupportModule::StartupModule called"));
}

void FGenerativeAISupportModule::ShutdownModule()
{
	// Runtime module cleanup if needed
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGenerativeAISupportModule, GenerativeAISupport)