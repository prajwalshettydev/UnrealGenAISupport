// Weather System Types - Data structures for dynamic weather
// Part of GenerativeAISupport Plugin

#pragma once

#include "CoreMinimal.h"
#include "WeatherTypes.generated.h"

/**
 * Weather condition type
 */
UENUM(BlueprintType)
enum class EWeatherType : uint8
{
	Clear UMETA(DisplayName = "Klar"),
	PartlyCloudy UMETA(DisplayName = "Teilweise Bewölkt"),
	Cloudy UMETA(DisplayName = "Bewölkt"),
	Overcast UMETA(DisplayName = "Bedeckt"),
	LightRain UMETA(DisplayName = "Leichter Regen"),
	Rain UMETA(DisplayName = "Regen"),
	HeavyRain UMETA(DisplayName = "Starkregen"),
	Thunderstorm UMETA(DisplayName = "Gewitter"),
	LightSnow UMETA(DisplayName = "Leichter Schnee"),
	Snow UMETA(DisplayName = "Schnee"),
	Blizzard UMETA(DisplayName = "Schneesturm"),
	Fog UMETA(DisplayName = "Nebel"),
	DenseFog UMETA(DisplayName = "Dichter Nebel"),
	Sandstorm UMETA(DisplayName = "Sandsturm"),
	Hail UMETA(DisplayName = "Hagel"),
	AuroraBorealis UMETA(DisplayName = "Nordlichter"),
	MagicStorm UMETA(DisplayName = "Magischer Sturm")
};

/**
 * Season type
 */
UENUM(BlueprintType)
enum class ESeason : uint8
{
	Spring UMETA(DisplayName = "Frühling"),
	Summer UMETA(DisplayName = "Sommer"),
	Autumn UMETA(DisplayName = "Herbst"),
	Winter UMETA(DisplayName = "Winter")
};

/**
 * Time of day
 */
UENUM(BlueprintType)
enum class ETimeOfDay : uint8
{
	Dawn UMETA(DisplayName = "Morgendämmerung"),
	Morning UMETA(DisplayName = "Morgen"),
	Noon UMETA(DisplayName = "Mittag"),
	Afternoon UMETA(DisplayName = "Nachmittag"),
	Evening UMETA(DisplayName = "Abend"),
	Dusk UMETA(DisplayName = "Abenddämmerung"),
	Night UMETA(DisplayName = "Nacht"),
	Midnight UMETA(DisplayName = "Mitternacht")
};

/**
 * Biome type for regional weather
 */
UENUM(BlueprintType)
enum class EBiomeType : uint8
{
	Temperate UMETA(DisplayName = "Gemäßigt"),
	Forest UMETA(DisplayName = "Wald"),
	Desert UMETA(DisplayName = "Wüste"),
	Arctic UMETA(DisplayName = "Arktis"),
	Tropical UMETA(DisplayName = "Tropisch"),
	Mountain UMETA(DisplayName = "Gebirge"),
	Swamp UMETA(DisplayName = "Sumpf"),
	Volcanic UMETA(DisplayName = "Vulkanisch"),
	Coastal UMETA(DisplayName = "Küste"),
	Magical UMETA(DisplayName = "Magisch")
};

/**
 * Wind direction
 */
UENUM(BlueprintType)
enum class EWindDirection : uint8
{
	North UMETA(DisplayName = "Nord"),
	NorthEast UMETA(DisplayName = "Nordost"),
	East UMETA(DisplayName = "Ost"),
	SouthEast UMETA(DisplayName = "Südost"),
	South UMETA(DisplayName = "Süd"),
	SouthWest UMETA(DisplayName = "Südwest"),
	West UMETA(DisplayName = "West"),
	NorthWest UMETA(DisplayName = "Nordwest"),
	Variable UMETA(DisplayName = "Wechselnd")
};

/**
 * Moon phase
 */
UENUM(BlueprintType)
enum class EMoonPhase : uint8
{
	NewMoon UMETA(DisplayName = "Neumond"),
	WaxingCrescent UMETA(DisplayName = "Zunehmende Sichel"),
	FirstQuarter UMETA(DisplayName = "Erstes Viertel"),
	WaxingGibbous UMETA(DisplayName = "Zunehmender Mond"),
	FullMoon UMETA(DisplayName = "Vollmond"),
	WaningGibbous UMETA(DisplayName = "Abnehmender Mond"),
	LastQuarter UMETA(DisplayName = "Letztes Viertel"),
	WaningCrescent UMETA(DisplayName = "Abnehmende Sichel")
};

/**
 * Weather effect modifiers for gameplay
 */
USTRUCT(BlueprintType)
struct FWeatherEffects
{
	GENERATED_BODY()

	/** Visibility multiplier (1.0 = normal) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float VisibilityMultiplier = 1.0f;

	/** Movement speed multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float MovementSpeedMultiplier = 1.0f;

	/** Stamina drain multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float StaminaDrainMultiplier = 1.0f;

	/** Fire damage multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float FireDamageMultiplier = 1.0f;

	/** Ice damage multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float IceDamageMultiplier = 1.0f;

	/** Lightning damage multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float LightningDamageMultiplier = 1.0f;

	/** Stealth detection range multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float StealthDetectionMultiplier = 1.0f;

	/** Sound detection range multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float SoundDetectionMultiplier = 1.0f;

	/** Accuracy penalty for ranged attacks */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float RangedAccuracyPenalty = 0.0f;

	/** Periodic damage to player (exposure) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float ExposureDamagePerSecond = 0.0f;

	/** Required warmth level to avoid exposure */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float RequiredWarmth = 0.0f;
};

/**
 * Wind data
 */
USTRUCT(BlueprintType)
struct FTitanWindData
{
	GENERATED_BODY()

	/** Wind direction */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	EWindDirection Direction = EWindDirection::North;

	/** Wind speed in km/h */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float Speed = 0.0f;

	/** Gust strength (0-1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float GustStrength = 0.0f;

	/** Get wind as vector */
	FVector GetWindVector() const
	{
		FVector Dir;
		switch (Direction)
		{
		case EWindDirection::North: Dir = FVector(1, 0, 0); break;
		case EWindDirection::NorthEast: Dir = FVector(0.707f, 0.707f, 0); break;
		case EWindDirection::East: Dir = FVector(0, 1, 0); break;
		case EWindDirection::SouthEast: Dir = FVector(-0.707f, 0.707f, 0); break;
		case EWindDirection::South: Dir = FVector(-1, 0, 0); break;
		case EWindDirection::SouthWest: Dir = FVector(-0.707f, -0.707f, 0); break;
		case EWindDirection::West: Dir = FVector(0, -1, 0); break;
		case EWindDirection::NorthWest: Dir = FVector(0.707f, -0.707f, 0); break;
		default: Dir = FVector::ZeroVector; break;
		}
		return Dir * Speed;
	}
};

/**
 * Atmospheric conditions
 */
USTRUCT(BlueprintType)
struct FAtmosphericConditions
{
	GENERATED_BODY()

	/** Temperature in Celsius */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float Temperature = 20.0f;

	/** Humidity (0-100%) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float Humidity = 50.0f;

	/** Air pressure in hPa */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float Pressure = 1013.0f;

	/** Cloud coverage (0-1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float CloudCoverage = 0.0f;

	/** Precipitation intensity (0-1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float PrecipitationIntensity = 0.0f;

	/** Fog density (0-1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float FogDensity = 0.0f;
};

/**
 * Complete weather state
 */
USTRUCT(BlueprintType)
struct FWeatherState
{
	GENERATED_BODY()

	/** Current weather type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	EWeatherType WeatherType = EWeatherType::Clear;

	/** Current season */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	ESeason Season = ESeason::Summer;

	/** Time of day */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	ETimeOfDay TimeOfDay = ETimeOfDay::Noon;

	/** Current biome */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	EBiomeType Biome = EBiomeType::Temperate;

	/** Wind data */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	FTitanWindData Wind;

	/** Atmospheric conditions */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	FAtmosphericConditions Atmosphere;

	/** Gameplay effects */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	FWeatherEffects Effects;

	/** Current moon phase */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	EMoonPhase MoonPhase = EMoonPhase::FullMoon;

	/** Is it currently transitioning? */
	UPROPERTY(BlueprintReadOnly, Category = "Weather")
	bool bIsTransitioning = false;

	/** Next weather type (during transition) */
	UPROPERTY(BlueprintReadOnly, Category = "Weather")
	EWeatherType NextWeatherType = EWeatherType::Clear;

	/** Transition progress (0-1) */
	UPROPERTY(BlueprintReadOnly, Category = "Weather")
	float TransitionProgress = 0.0f;
};

/**
 * Weather transition settings
 */
USTRUCT(BlueprintType)
struct FWeatherTransition
{
	GENERATED_BODY()

	/** Source weather type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	EWeatherType FromWeather = EWeatherType::Clear;

	/** Target weather type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	EWeatherType ToWeather = EWeatherType::Clear;

	/** Transition duration in seconds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float Duration = 60.0f;

	/** Transition curve type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	bool bUseEaseInOut = true;
};

/**
 * Weather forecast entry
 */
USTRUCT(BlueprintType)
struct FWeatherForecast
{
	GENERATED_BODY()

	/** Predicted weather type */
	UPROPERTY(BlueprintReadOnly, Category = "Weather")
	EWeatherType PredictedWeather = EWeatherType::Clear;

	/** Confidence level (0-1) */
	UPROPERTY(BlueprintReadOnly, Category = "Weather")
	float Confidence = 1.0f;

	/** Time until this weather (in game hours) */
	UPROPERTY(BlueprintReadOnly, Category = "Weather")
	float HoursFromNow = 0.0f;

	/** Expected duration in game hours */
	UPROPERTY(BlueprintReadOnly, Category = "Weather")
	float ExpectedDuration = 1.0f;
};

/**
 * Biome weather probabilities
 */
USTRUCT(BlueprintType)
struct FBiomeWeatherProbabilities
{
	GENERATED_BODY()

	/** Biome type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	EBiomeType Biome = EBiomeType::Temperate;

	/** Season */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	ESeason Season = ESeason::Summer;

	/** Weather probabilities (must sum to 1.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	TMap<EWeatherType, float> WeatherProbabilities;

	/** Base temperature range */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float MinTemperature = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float MaxTemperature = 30.0f;
};

/**
 * Time settings for day/night cycle
 */
USTRUCT(BlueprintType)
struct FTimeSettings
{
	GENERATED_BODY()

	/** Current game hour (0-24) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float CurrentHour = 12.0f;

	/** Current day of year (1-365) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	int32 DayOfYear = 1;

	/** Current year */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	int32 Year = 1;

	/** Real seconds per game hour */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float SecondsPerGameHour = 60.0f;

	/** Sunrise hour */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float SunriseHour = 6.0f;

	/** Sunset hour */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float SunsetHour = 20.0f;

	/** Is time frozen? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	bool bTimeFrozen = false;
};

/**
 * Weather event (special weather conditions)
 */
USTRUCT(BlueprintType)
struct FWeatherEvent
{
	GENERATED_BODY()

	/** Event ID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	FString EventID;

	/** Event display name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	FText DisplayName;

	/** Weather type during event */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	EWeatherType WeatherType = EWeatherType::MagicStorm;

	/** Duration in game hours */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float Duration = 1.0f;

	/** Special effects */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	FWeatherEffects SpecialEffects;

	/** Spawns special enemies? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	bool bSpawnsSpecialEnemies = false;

	/** Bonus loot during event */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float LootBonusMultiplier = 1.0f;
};

/**
 * Lighting state for rendering
 */
USTRUCT(BlueprintType)
struct FWeatherLighting
{
	GENERATED_BODY()

	/** Sun intensity (0-1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float SunIntensity = 1.0f;

	/** Sun color */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	FLinearColor SunColor = FLinearColor(1.0f, 0.95f, 0.85f);

	/** Ambient light intensity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float AmbientIntensity = 0.3f;

	/** Ambient color */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	FLinearColor AmbientColor = FLinearColor(0.4f, 0.5f, 0.7f);

	/** Moon intensity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	float MoonIntensity = 0.1f;

	/** Moon color */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	FLinearColor MoonColor = FLinearColor(0.8f, 0.85f, 1.0f);

	/** Fog color */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	FLinearColor FogColor = FLinearColor(0.5f, 0.6f, 0.7f);

	/** Sky color */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	FLinearColor SkyColor = FLinearColor(0.3f, 0.5f, 0.9f);
};
