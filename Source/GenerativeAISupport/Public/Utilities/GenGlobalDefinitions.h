// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

//in accordance with https://unrealcommunity.wiki/logging-lgpidy6i
DECLARE_LOG_CATEGORY_EXTERN(LogGenAI, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogGenPerformance, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogGenAIVerbose, Log, All);

#define ENABLE_LOG(Category) UE_SET_LOG_VERBOSITY(Category, Log)
#define DISABLE_LOG(Category) UE_SET_LOG_VERBOSITY(Category, NoLogging)

#if WITH_EDITOR || UE_BUILD_DEVELOPMENT
#define LOG_TIME_START(StartTimePerf) \
const double StartTimePerf = FPlatformTime::Seconds();
#else
#define LOG_TIME_START(StartTimePerf) do {} while(0)
#endif

#if WITH_EDITOR || UE_BUILD_DEVELOPMENT
#define LOG_TIME_ELAPSED(StartTimePerf, Description) \
{ \
const double ElapsedTime = (FPlatformTime::Seconds() - StartTimePerf) * 1000.0; \
UE_LOG(LogGenPerformance, Display, TEXT("%s took: %f ms"), Description, ElapsedTime); \
}
#else
#define LOG_TIME_ELAPSED(StartTimePerf, Description) do {} while(0)
#endif

#include "CoreMinimal.h"
#include "GenGlobalDefinitions.generated.h" // Include at the end

// Disable logs by default
USTRUCT()
struct FLogInitializer
{
	GENERATED_BODY()
	
	FLogInitializer()
	{
		//DISABLE_LOG(LogGenAI);
		//DISABLE_LOG(LogGenPerformance);
		DISABLE_LOG(LogGenAIVerbose);

		ENABLE_LOG(LogGenAI);
		ENABLE_LOG(LogGenPerformance);
		//ENABLE_LOG(LogGenAIVerbose);
	}
};

static const FLogInitializer LogInitializer;