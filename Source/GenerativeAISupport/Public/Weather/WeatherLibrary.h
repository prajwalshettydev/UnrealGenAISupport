// Weather Library - Blueprint functions for dynamic weather system
// Part of GenerativeAISupport Plugin

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WeatherTypes.h"
#include "WeatherLibrary.generated.h"

// Delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeatherChanged, EWeatherType, OldWeather, EWeatherType, NewWeather);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimeOfDayChanged, ETimeOfDay, NewTimeOfDay);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSeasonChanged, ESeason, NewSeason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeatherEventStarted, const FWeatherEvent&, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeatherEventEnded, const FWeatherEvent&, Event);

/**
 * Blueprint Function Library for Weather System
 */
UCLASS()
class GENERATIVEAISUPPORT_API UWeatherLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ============================================
	// WEATHER STATE
	// ============================================

	/**
	 * Get current weather state
	 */
	UFUNCTION(BlueprintPure, Category = "Weather|State")
	static FWeatherState GetCurrentWeather();

	/**
	 * Get current weather type
	 */
	UFUNCTION(BlueprintPure, Category = "Weather|State")
	static EWeatherType GetWeatherType();

	/**
	 * Get current season
	 */
	UFUNCTION(BlueprintPure, Category = "Weather|State")
	static ESeason GetCurrentSeason();

	/**
	 * Get current time of day
	 */
	UFUNCTION(BlueprintPure, Category = "Weather|State")
	static ETimeOfDay GetTimeOfDay();

	/**
	 * Get current biome
	 */
	UFUNCTION(BlueprintPure, Category = "Weather|State")
	static EBiomeType GetCurrentBiome();

	/**
	 * Set current biome (when player moves regions)
	 */
	UFUNCTION(BlueprintCallable, Category = "Weather|State")
	static void SetCurrentBiome(EBiomeType NewBiome);

	/**
	 * Is it currently raining/snowing?
	 */
	UFUNCTION(BlueprintPure, Category = "Weather|State")
	static bool IsPrecipitating();

	/**
	 * Is it currently nighttime?
	 */
	UFUNCTION(BlueprintPure, Category = "Weather|State")
	static bool IsNightTime();

	/**
	 * Get current temperature
	 */
	UFUNCTION(BlueprintPure, Category = "Weather|State")
	static float GetTemperature();

	// ============================================
	// WEATHER CONTROL
	// ============================================

	/**
	 * Force weather change (instant)
	 */
	UFUNCTION(BlueprintCallable, Category = "Weather|Control")
	static void SetWeather(EWeatherType NewWeather);

	/**
	 * Transition to new weather over time
	 */
	UFUNCTION(BlueprintCallable, Category = "Weather|Control")
	static void TransitionToWeather(EWeatherType NewWeather, float TransitionDuration = 60.0f);

	/**
	 * Force season change
	 */
	UFUNCTION(BlueprintCallable, Category = "Weather|Control")
	static void SetSeason(ESeason NewSeason);

	/**
	 * Reset to random weather based on biome/season
	 */
	UFUNCTION(BlueprintCallable, Category = "Weather|Control")
	static void RandomizeWeather();

	/**
	 * Force weather event
	 */
	UFUNCTION(BlueprintCallable, Category = "Weather|Control")
	static void TriggerWeatherEvent(const FString& EventID);

	/**
	 * Stop current weather event
	 */
	UFUNCTION(BlueprintCallable, Category = "Weather|Control")
	static void StopWeatherEvent();

	// ============================================
	// TIME SYSTEM
	// ============================================

	/**
	 * Get time settings
	 */
	UFUNCTION(BlueprintPure, Category = "Weather|Time")
	static FTimeSettings GetTimeSettings();

	/**
	 * Set current game hour
	 */
	UFUNCTION(BlueprintCallable, Category = "Weather|Time")
	static void SetGameHour(float Hour);

	/**
	 * Advance time by hours
	 */
	UFUNCTION(BlueprintCallable, Category = "Weather|Time")
	static void AdvanceTime(float Hours);

	/**
	 * Set time scale (real seconds per game hour)
	 */
	UFUNCTION(BlueprintCallable, Category = "Weather|Time")
	static void SetTimeScale(float SecondsPerHour);

	/**
	 * Freeze/unfreeze time
	 */
	UFUNCTION(BlueprintCallable, Category = "Weather|Time")
	static void SetTimeFrozen(bool bFrozen);

	/**
	 * Get sun position angle (0-360)
	 */
	UFUNCTION(BlueprintPure, Category = "Weather|Time")
	static float GetSunAngle();

	/**
	 * Get moon phase
	 */
	UFUNCTION(BlueprintPure, Category = "Weather|Time")
	static EMoonPhase GetMoonPhase();

	/**
	 * Get formatted time string
	 */
	UFUNCTION(BlueprintPure, Category = "Weather|Time")
	static FString GetTimeString();

	/**
	 * Get formatted date string
	 */
	UFUNCTION(BlueprintPure, Category = "Weather|Time")
	static FString GetDateString();

	// ============================================
	// WEATHER EFFECTS
	// ============================================

	/**
	 * Get current weather effects
	 */
	UFUNCTION(BlueprintPure, Category = "Weather|Effects")
	static FWeatherEffects GetWeatherEffects();

	/**
	 * Get wind data
	 */
	UFUNCTION(BlueprintPure, Category = "Weather|Effects")
	static FTitanWindData GetWindData();

	/**
	 * Get wind direction as vector
	 */
	UFUNCTION(BlueprintPure, Category = "Weather|Effects")
	static FVector GetWindVector();

	/**
	 * Get lighting state for current weather/time
	 */
	UFUNCTION(BlueprintPure, Category = "Weather|Effects")
	static FWeatherLighting GetWeatherLighting();

	/**
	 * Get fog visibility distance
	 */
	UFUNCTION(BlueprintPure, Category = "Weather|Effects")
	static float GetVisibilityDistance();

	/**
	 * Apply weather effects to character stats
	 */
	UFUNCTION(BlueprintCallable, Category = "Weather|Effects")
	static TMap<FString, float> ApplyWeatherModifiers(const TMap<FString, float>& BaseStats);

	// ============================================
	// FORECAST & PREDICTION
	// ============================================

	/**
	 * Get weather forecast for next N hours
	 */
	UFUNCTION(BlueprintCallable, Category = "Weather|Forecast")
	static TArray<FWeatherForecast> GetForecast(int32 HoursAhead = 24);

	/**
	 * Get probability of weather type in biome/season
	 */
	UFUNCTION(BlueprintPure, Category = "Weather|Forecast")
	static float GetWeatherProbability(EWeatherType Weather, EBiomeType Biome, ESeason Season);

	/**
	 * Predict next weather change
	 */
	UFUNCTION(BlueprintPure, Category = "Weather|Forecast")
	static FWeatherForecast PredictNextChange();

	// ============================================
	// BIOME CONFIGURATION
	// ============================================

	/**
	 * Load biome weather configuration from JSON
	 */
	UFUNCTION(BlueprintCallable, Category = "Weather|Config")
	static bool LoadBiomeConfigFromJSON(const FString& FilePath);

	/**
	 * Set biome weather probabilities
	 */
	UFUNCTION(BlueprintCallable, Category = "Weather|Config")
	static void SetBiomeWeatherProbabilities(const FBiomeWeatherProbabilities& Config);

	/**
	 * Get biome temperature range
	 */
	UFUNCTION(BlueprintPure, Category = "Weather|Config")
	static void GetBiomeTemperatureRange(EBiomeType Biome, ESeason Season, float& OutMin, float& OutMax);

	// ============================================
	// WEATHER EVENTS
	// ============================================

	/**
	 * Register weather event
	 */
	UFUNCTION(BlueprintCallable, Category = "Weather|Events")
	static void RegisterWeatherEvent(const FWeatherEvent& Event);

	/**
	 * Get active weather event
	 */
	UFUNCTION(BlueprintPure, Category = "Weather|Events")
	static bool GetActiveEvent(FWeatherEvent& OutEvent);

	/**
	 * Is weather event active?
	 */
	UFUNCTION(BlueprintPure, Category = "Weather|Events")
	static bool IsWeatherEventActive();

	/**
	 * Get all registered weather events
	 */
	UFUNCTION(BlueprintCallable, Category = "Weather|Events")
	static TArray<FWeatherEvent> GetAllWeatherEvents();

	// ============================================
	// DISPLAY HELPERS
	// ============================================

	/**
	 * Get weather type display name
	 */
	UFUNCTION(BlueprintPure, Category = "Weather|Display")
	static FText GetWeatherDisplayName(EWeatherType Weather);

	/**
	 * Get season display name
	 */
	UFUNCTION(BlueprintPure, Category = "Weather|Display")
	static FText GetSeasonDisplayName(ESeason Season);

	/**
	 * Get time of day display name
	 */
	UFUNCTION(BlueprintPure, Category = "Weather|Display")
	static FText GetTimeOfDayDisplayName(ETimeOfDay TimeOfDay);

	/**
	 * Get biome display name
	 */
	UFUNCTION(BlueprintPure, Category = "Weather|Display")
	static FText GetBiomeDisplayName(EBiomeType Biome);

	/**
	 * Get weather icon path
	 */
	UFUNCTION(BlueprintPure, Category = "Weather|Display")
	static FString GetWeatherIconPath(EWeatherType Weather);

	/**
	 * Get weather description
	 */
	UFUNCTION(BlueprintPure, Category = "Weather|Display")
	static FText GetWeatherDescription(EWeatherType Weather);

	// ============================================
	// SIMULATION UPDATE
	// ============================================

	/**
	 * Update weather simulation (call every frame or tick)
	 */
	UFUNCTION(BlueprintCallable, Category = "Weather|Simulation")
	static void UpdateWeatherSimulation(float DeltaTime);

	/**
	 * Initialize weather system
	 */
	UFUNCTION(BlueprintCallable, Category = "Weather|Simulation")
	static void InitializeWeatherSystem();

	/**
	 * Save weather state
	 */
	UFUNCTION(BlueprintCallable, Category = "Weather|Save")
	static FString SaveWeatherStateToJSON();

	/**
	 * Load weather state
	 */
	UFUNCTION(BlueprintCallable, Category = "Weather|Save")
	static bool LoadWeatherStateFromJSON(const FString& JSONString);

private:
	/** Current weather state */
	static FWeatherState CurrentWeather;

	/** Time settings */
	static FTimeSettings TimeSettings;

	/** Biome configurations */
	static TMap<EBiomeType, TMap<ESeason, FBiomeWeatherProbabilities>> BiomeConfigs;

	/** Registered weather events */
	static TMap<FString, FWeatherEvent> WeatherEvents;

	/** Active weather event (if any) */
	static FString ActiveEventID;

	/** Time until next weather change */
	static float NextWeatherChangeTimer;

	/** Weather effects cache */
	static TMap<EWeatherType, FWeatherEffects> WeatherEffectsCache;

	/** Initialize default weather effects */
	static void InitializeDefaultWeatherEffects();

	/** Initialize default biome configs */
	static void InitializeDefaultBiomeConfigs();

	/** Calculate weather effects for type */
	static FWeatherEffects CalculateWeatherEffects(EWeatherType Weather);

	/** Calculate lighting for current state */
	static FWeatherLighting CalculateLighting();

	/** Pick random weather for biome/season */
	static EWeatherType PickRandomWeather(EBiomeType Biome, ESeason Season);

	/** Update time of day from hour */
	static ETimeOfDay CalculateTimeOfDay(float Hour);

	/** Calculate moon phase from day */
	static EMoonPhase CalculateMoonPhase(int32 DayOfYear);

	/** Calculate season from day of year */
	static ESeason CalculateSeason(int32 DayOfYear);
};
