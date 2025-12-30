// Weather Library Implementation
// Part of GenerativeAISupport Plugin

#include "Weather/WeatherLibrary.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

// Static member initialization
FWeatherState UWeatherLibrary::CurrentWeather;
FTimeSettings UWeatherLibrary::TimeSettings;
TMap<EBiomeType, TMap<ESeason, FBiomeWeatherProbabilities>> UWeatherLibrary::BiomeConfigs;
TMap<FString, FWeatherEvent> UWeatherLibrary::WeatherEvents;
FString UWeatherLibrary::ActiveEventID;
float UWeatherLibrary::NextWeatherChangeTimer = 3600.0f;
TMap<EWeatherType, FWeatherEffects> UWeatherLibrary::WeatherEffectsCache;

// ============================================
// WEATHER STATE
// ============================================

FWeatherState UWeatherLibrary::GetCurrentWeather()
{
	return CurrentWeather;
}

EWeatherType UWeatherLibrary::GetWeatherType()
{
	return CurrentWeather.WeatherType;
}

ESeason UWeatherLibrary::GetCurrentSeason()
{
	return CurrentWeather.Season;
}

ETimeOfDay UWeatherLibrary::GetTimeOfDay()
{
	return CurrentWeather.TimeOfDay;
}

EBiomeType UWeatherLibrary::GetCurrentBiome()
{
	return CurrentWeather.Biome;
}

void UWeatherLibrary::SetCurrentBiome(EBiomeType NewBiome)
{
	if (CurrentWeather.Biome != NewBiome)
	{
		CurrentWeather.Biome = NewBiome;
		// Potentially trigger weather change for new biome
		RandomizeWeather();
	}
}

bool UWeatherLibrary::IsPrecipitating()
{
	switch (CurrentWeather.WeatherType)
	{
	case EWeatherType::LightRain:
	case EWeatherType::Rain:
	case EWeatherType::HeavyRain:
	case EWeatherType::Thunderstorm:
	case EWeatherType::LightSnow:
	case EWeatherType::Snow:
	case EWeatherType::Blizzard:
	case EWeatherType::Hail:
		return true;
	default:
		return false;
	}
}

bool UWeatherLibrary::IsNightTime()
{
	return TimeSettings.CurrentHour < TimeSettings.SunriseHour ||
	       TimeSettings.CurrentHour >= TimeSettings.SunsetHour;
}

float UWeatherLibrary::GetTemperature()
{
	return CurrentWeather.Atmosphere.Temperature;
}

// ============================================
// WEATHER CONTROL
// ============================================

void UWeatherLibrary::SetWeather(EWeatherType NewWeather)
{
	CurrentWeather.WeatherType = NewWeather;
	CurrentWeather.Effects = CalculateWeatherEffects(NewWeather);
	CurrentWeather.bIsTransitioning = false;
	CurrentWeather.TransitionProgress = 0.0f;

	// Update atmosphere based on weather
	switch (NewWeather)
	{
	case EWeatherType::Clear:
		CurrentWeather.Atmosphere.CloudCoverage = 0.0f;
		CurrentWeather.Atmosphere.PrecipitationIntensity = 0.0f;
		CurrentWeather.Atmosphere.FogDensity = 0.0f;
		break;
	case EWeatherType::PartlyCloudy:
		CurrentWeather.Atmosphere.CloudCoverage = 0.3f;
		break;
	case EWeatherType::Cloudy:
		CurrentWeather.Atmosphere.CloudCoverage = 0.6f;
		break;
	case EWeatherType::Overcast:
		CurrentWeather.Atmosphere.CloudCoverage = 0.9f;
		break;
	case EWeatherType::LightRain:
		CurrentWeather.Atmosphere.CloudCoverage = 0.7f;
		CurrentWeather.Atmosphere.PrecipitationIntensity = 0.3f;
		break;
	case EWeatherType::Rain:
		CurrentWeather.Atmosphere.CloudCoverage = 0.85f;
		CurrentWeather.Atmosphere.PrecipitationIntensity = 0.6f;
		break;
	case EWeatherType::HeavyRain:
		CurrentWeather.Atmosphere.CloudCoverage = 1.0f;
		CurrentWeather.Atmosphere.PrecipitationIntensity = 0.9f;
		break;
	case EWeatherType::Thunderstorm:
		CurrentWeather.Atmosphere.CloudCoverage = 1.0f;
		CurrentWeather.Atmosphere.PrecipitationIntensity = 0.8f;
		break;
	case EWeatherType::Fog:
		CurrentWeather.Atmosphere.FogDensity = 0.6f;
		break;
	case EWeatherType::DenseFog:
		CurrentWeather.Atmosphere.FogDensity = 0.9f;
		break;
	case EWeatherType::LightSnow:
		CurrentWeather.Atmosphere.CloudCoverage = 0.8f;
		CurrentWeather.Atmosphere.PrecipitationIntensity = 0.3f;
		break;
	case EWeatherType::Snow:
		CurrentWeather.Atmosphere.CloudCoverage = 0.9f;
		CurrentWeather.Atmosphere.PrecipitationIntensity = 0.6f;
		break;
	case EWeatherType::Blizzard:
		CurrentWeather.Atmosphere.CloudCoverage = 1.0f;
		CurrentWeather.Atmosphere.PrecipitationIntensity = 1.0f;
		CurrentWeather.Atmosphere.FogDensity = 0.7f;
		break;
	default:
		break;
	}
}

void UWeatherLibrary::TransitionToWeather(EWeatherType NewWeather, float TransitionDuration)
{
	if (CurrentWeather.WeatherType == NewWeather) return;

	CurrentWeather.bIsTransitioning = true;
	CurrentWeather.NextWeatherType = NewWeather;
	CurrentWeather.TransitionProgress = 0.0f;

	// Store transition duration in timer
	NextWeatherChangeTimer = TransitionDuration;
}

void UWeatherLibrary::SetSeason(ESeason NewSeason)
{
	CurrentWeather.Season = NewSeason;

	// Update temperature range for season
	float MinTemp, MaxTemp;
	GetBiomeTemperatureRange(CurrentWeather.Biome, NewSeason, MinTemp, MaxTemp);
	CurrentWeather.Atmosphere.Temperature = FMath::RandRange(MinTemp, MaxTemp);
}

void UWeatherLibrary::RandomizeWeather()
{
	EWeatherType NewWeather = PickRandomWeather(CurrentWeather.Biome, CurrentWeather.Season);
	SetWeather(NewWeather);

	// Randomize wind
	CurrentWeather.Wind.Direction = static_cast<EWindDirection>(FMath::RandRange(0, 8));
	CurrentWeather.Wind.Speed = FMath::RandRange(0.0f, 50.0f);
	CurrentWeather.Wind.GustStrength = FMath::RandRange(0.0f, 0.5f);

	// Randomize temperature within biome range
	float MinTemp, MaxTemp;
	GetBiomeTemperatureRange(CurrentWeather.Biome, CurrentWeather.Season, MinTemp, MaxTemp);
	CurrentWeather.Atmosphere.Temperature = FMath::RandRange(MinTemp, MaxTemp);

	// Set next weather change timer
	NextWeatherChangeTimer = FMath::RandRange(1800.0f, 7200.0f); // 30 min to 2 hours
}

void UWeatherLibrary::TriggerWeatherEvent(const FString& EventID)
{
	if (const FWeatherEvent* Event = WeatherEvents.Find(EventID))
	{
		ActiveEventID = EventID;
		SetWeather(Event->WeatherType);
		CurrentWeather.Effects = Event->SpecialEffects;
	}
}

void UWeatherLibrary::StopWeatherEvent()
{
	ActiveEventID.Empty();
	RandomizeWeather();
}

// ============================================
// TIME SYSTEM
// ============================================

FTimeSettings UWeatherLibrary::GetTimeSettings()
{
	return TimeSettings;
}

void UWeatherLibrary::SetGameHour(float Hour)
{
	TimeSettings.CurrentHour = FMath::Fmod(Hour, 24.0f);
	if (TimeSettings.CurrentHour < 0) TimeSettings.CurrentHour += 24.0f;

	CurrentWeather.TimeOfDay = CalculateTimeOfDay(TimeSettings.CurrentHour);
}

void UWeatherLibrary::AdvanceTime(float Hours)
{
	float NewHour = TimeSettings.CurrentHour + Hours;

	// Handle day rollover
	while (NewHour >= 24.0f)
	{
		NewHour -= 24.0f;
		TimeSettings.DayOfYear++;

		if (TimeSettings.DayOfYear > 365)
		{
			TimeSettings.DayOfYear = 1;
			TimeSettings.Year++;
		}

		// Update season and moon phase
		CurrentWeather.Season = CalculateSeason(TimeSettings.DayOfYear);
		CurrentWeather.MoonPhase = CalculateMoonPhase(TimeSettings.DayOfYear);
	}

	SetGameHour(NewHour);
}

void UWeatherLibrary::SetTimeScale(float SecondsPerHour)
{
	TimeSettings.SecondsPerGameHour = FMath::Max(1.0f, SecondsPerHour);
}

void UWeatherLibrary::SetTimeFrozen(bool bFrozen)
{
	TimeSettings.bTimeFrozen = bFrozen;
}

float UWeatherLibrary::GetSunAngle()
{
	// Sun angle: 0 at midnight, 180 at noon
	return (TimeSettings.CurrentHour / 24.0f) * 360.0f;
}

EMoonPhase UWeatherLibrary::GetMoonPhase()
{
	return CurrentWeather.MoonPhase;
}

FString UWeatherLibrary::GetTimeString()
{
	int32 Hour = FMath::FloorToInt(TimeSettings.CurrentHour);
	int32 Minute = FMath::FloorToInt((TimeSettings.CurrentHour - Hour) * 60.0f);
	return FString::Printf(TEXT("%02d:%02d"), Hour, Minute);
}

FString UWeatherLibrary::GetDateString()
{
	return FString::Printf(TEXT("Tag %d, Jahr %d"), TimeSettings.DayOfYear, TimeSettings.Year);
}

// ============================================
// WEATHER EFFECTS
// ============================================

FWeatherEffects UWeatherLibrary::GetWeatherEffects()
{
	return CurrentWeather.Effects;
}

FTitanWindData UWeatherLibrary::GetWindData()
{
	return CurrentWeather.Wind;
}

FVector UWeatherLibrary::GetWindVector()
{
	return CurrentWeather.Wind.GetWindVector();
}

FWeatherLighting UWeatherLibrary::GetWeatherLighting()
{
	return CalculateLighting();
}

float UWeatherLibrary::GetVisibilityDistance()
{
	float BaseVisibility = 10000.0f; // 10km base visibility
	return BaseVisibility * CurrentWeather.Effects.VisibilityMultiplier;
}

TMap<FString, float> UWeatherLibrary::ApplyWeatherModifiers(const TMap<FString, float>& BaseStats)
{
	TMap<FString, float> ModifiedStats = BaseStats;
	const FWeatherEffects& Effects = CurrentWeather.Effects;

	// Apply movement speed modifier
	if (float* MoveSpeed = ModifiedStats.Find(TEXT("MovementSpeed")))
	{
		*MoveSpeed *= Effects.MovementSpeedMultiplier;
	}

	// Apply stamina drain modifier
	if (float* StaminaDrain = ModifiedStats.Find(TEXT("StaminaDrain")))
	{
		*StaminaDrain *= Effects.StaminaDrainMultiplier;
	}

	// Apply damage modifiers
	if (float* FireDamage = ModifiedStats.Find(TEXT("FireDamage")))
	{
		*FireDamage *= Effects.FireDamageMultiplier;
	}
	if (float* IceDamage = ModifiedStats.Find(TEXT("IceDamage")))
	{
		*IceDamage *= Effects.IceDamageMultiplier;
	}
	if (float* LightningDamage = ModifiedStats.Find(TEXT("LightningDamage")))
	{
		*LightningDamage *= Effects.LightningDamageMultiplier;
	}

	// Apply accuracy penalty
	if (float* Accuracy = ModifiedStats.Find(TEXT("RangedAccuracy")))
	{
		*Accuracy -= Effects.RangedAccuracyPenalty;
	}

	return ModifiedStats;
}

// ============================================
// FORECAST & PREDICTION
// ============================================

TArray<FWeatherForecast> UWeatherLibrary::GetForecast(int32 HoursAhead)
{
	TArray<FWeatherForecast> Forecast;

	float HoursAccumulated = 0.0f;
	EWeatherType PreviousWeather = CurrentWeather.WeatherType;

	while (HoursAccumulated < HoursAhead)
	{
		FWeatherForecast Entry;
		Entry.HoursFromNow = HoursAccumulated;
		Entry.PredictedWeather = PickRandomWeather(CurrentWeather.Biome, CurrentWeather.Season);
		Entry.Confidence = FMath::Max(0.3f, 1.0f - (HoursAccumulated / static_cast<float>(HoursAhead)));
		Entry.ExpectedDuration = FMath::RandRange(1.0f, 4.0f);

		Forecast.Add(Entry);
		HoursAccumulated += Entry.ExpectedDuration;
	}

	return Forecast;
}

float UWeatherLibrary::GetWeatherProbability(EWeatherType Weather, EBiomeType Biome, ESeason Season)
{
	if (const TMap<ESeason, FBiomeWeatherProbabilities>* SeasonMap = BiomeConfigs.Find(Biome))
	{
		if (const FBiomeWeatherProbabilities* Config = SeasonMap->Find(Season))
		{
			if (const float* Prob = Config->WeatherProbabilities.Find(Weather))
			{
				return *Prob;
			}
		}
	}
	return 0.1f; // Default low probability
}

FWeatherForecast UWeatherLibrary::PredictNextChange()
{
	FWeatherForecast Prediction;
	Prediction.HoursFromNow = NextWeatherChangeTimer / 3600.0f;
	Prediction.PredictedWeather = PickRandomWeather(CurrentWeather.Biome, CurrentWeather.Season);
	Prediction.Confidence = 0.7f;
	Prediction.ExpectedDuration = FMath::RandRange(1.0f, 4.0f);
	return Prediction;
}

// ============================================
// BIOME CONFIGURATION
// ============================================

bool UWeatherLibrary::LoadBiomeConfigFromJSON(const FString& FilePath)
{
	FString JSONString;
	if (!FFileHelper::LoadFileToString(JSONString, *FilePath))
	{
		return false;
	}

	TSharedPtr<FJsonObject> RootObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JSONString);

	if (!FJsonSerializer::Deserialize(Reader, RootObject))
	{
		return false;
	}

	// Parse biome configurations
	// ... (Implementation for JSON parsing)

	return true;
}

void UWeatherLibrary::SetBiomeWeatherProbabilities(const FBiomeWeatherProbabilities& Config)
{
	if (!BiomeConfigs.Contains(Config.Biome))
	{
		BiomeConfigs.Add(Config.Biome, TMap<ESeason, FBiomeWeatherProbabilities>());
	}
	BiomeConfigs[Config.Biome].Add(Config.Season, Config);
}

void UWeatherLibrary::GetBiomeTemperatureRange(EBiomeType Biome, ESeason Season, float& OutMin, float& OutMax)
{
	// Default temperature ranges
	switch (Biome)
	{
	case EBiomeType::Arctic:
		OutMin = -40.0f; OutMax = -10.0f;
		if (Season == ESeason::Summer) { OutMin = -20.0f; OutMax = 5.0f; }
		break;
	case EBiomeType::Desert:
		OutMin = 20.0f; OutMax = 45.0f;
		if (Season == ESeason::Winter) { OutMin = 5.0f; OutMax = 25.0f; }
		break;
	case EBiomeType::Tropical:
		OutMin = 25.0f; OutMax = 35.0f;
		break;
	case EBiomeType::Mountain:
		OutMin = -10.0f; OutMax = 15.0f;
		if (Season == ESeason::Summer) { OutMin = 5.0f; OutMax = 25.0f; }
		break;
	case EBiomeType::Swamp:
		OutMin = 15.0f; OutMax = 30.0f;
		break;
	case EBiomeType::Volcanic:
		OutMin = 30.0f; OutMax = 50.0f;
		break;
	default: // Temperate, Forest, Coastal
		switch (Season)
		{
		case ESeason::Spring: OutMin = 8.0f; OutMax = 20.0f; break;
		case ESeason::Summer: OutMin = 18.0f; OutMax = 32.0f; break;
		case ESeason::Autumn: OutMin = 5.0f; OutMax = 18.0f; break;
		case ESeason::Winter: OutMin = -5.0f; OutMax = 8.0f; break;
		}
		break;
	}
}

// ============================================
// WEATHER EVENTS
// ============================================

void UWeatherLibrary::RegisterWeatherEvent(const FWeatherEvent& Event)
{
	WeatherEvents.Add(Event.EventID, Event);
}

bool UWeatherLibrary::GetActiveEvent(FWeatherEvent& OutEvent)
{
	if (ActiveEventID.IsEmpty()) return false;

	if (const FWeatherEvent* Event = WeatherEvents.Find(ActiveEventID))
	{
		OutEvent = *Event;
		return true;
	}
	return false;
}

bool UWeatherLibrary::IsWeatherEventActive()
{
	return !ActiveEventID.IsEmpty();
}

TArray<FWeatherEvent> UWeatherLibrary::GetAllWeatherEvents()
{
	TArray<FWeatherEvent> Events;
	WeatherEvents.GenerateValueArray(Events);
	return Events;
}

// ============================================
// DISPLAY HELPERS
// ============================================

FText UWeatherLibrary::GetWeatherDisplayName(EWeatherType Weather)
{
	switch (Weather)
	{
	case EWeatherType::Clear: return FText::FromString(TEXT("Klar"));
	case EWeatherType::PartlyCloudy: return FText::FromString(TEXT("Teilweise Bewölkt"));
	case EWeatherType::Cloudy: return FText::FromString(TEXT("Bewölkt"));
	case EWeatherType::Overcast: return FText::FromString(TEXT("Bedeckt"));
	case EWeatherType::LightRain: return FText::FromString(TEXT("Leichter Regen"));
	case EWeatherType::Rain: return FText::FromString(TEXT("Regen"));
	case EWeatherType::HeavyRain: return FText::FromString(TEXT("Starkregen"));
	case EWeatherType::Thunderstorm: return FText::FromString(TEXT("Gewitter"));
	case EWeatherType::LightSnow: return FText::FromString(TEXT("Leichter Schnee"));
	case EWeatherType::Snow: return FText::FromString(TEXT("Schnee"));
	case EWeatherType::Blizzard: return FText::FromString(TEXT("Schneesturm"));
	case EWeatherType::Fog: return FText::FromString(TEXT("Nebel"));
	case EWeatherType::DenseFog: return FText::FromString(TEXT("Dichter Nebel"));
	case EWeatherType::Sandstorm: return FText::FromString(TEXT("Sandsturm"));
	case EWeatherType::Hail: return FText::FromString(TEXT("Hagel"));
	case EWeatherType::AuroraBorealis: return FText::FromString(TEXT("Nordlichter"));
	case EWeatherType::MagicStorm: return FText::FromString(TEXT("Magischer Sturm"));
	default: return FText::FromString(TEXT("Unbekannt"));
	}
}

FText UWeatherLibrary::GetSeasonDisplayName(ESeason Season)
{
	switch (Season)
	{
	case ESeason::Spring: return FText::FromString(TEXT("Frühling"));
	case ESeason::Summer: return FText::FromString(TEXT("Sommer"));
	case ESeason::Autumn: return FText::FromString(TEXT("Herbst"));
	case ESeason::Winter: return FText::FromString(TEXT("Winter"));
	default: return FText::FromString(TEXT("Unbekannt"));
	}
}

FText UWeatherLibrary::GetTimeOfDayDisplayName(ETimeOfDay TimeOfDay)
{
	switch (TimeOfDay)
	{
	case ETimeOfDay::Dawn: return FText::FromString(TEXT("Morgendämmerung"));
	case ETimeOfDay::Morning: return FText::FromString(TEXT("Morgen"));
	case ETimeOfDay::Noon: return FText::FromString(TEXT("Mittag"));
	case ETimeOfDay::Afternoon: return FText::FromString(TEXT("Nachmittag"));
	case ETimeOfDay::Evening: return FText::FromString(TEXT("Abend"));
	case ETimeOfDay::Dusk: return FText::FromString(TEXT("Abenddämmerung"));
	case ETimeOfDay::Night: return FText::FromString(TEXT("Nacht"));
	case ETimeOfDay::Midnight: return FText::FromString(TEXT("Mitternacht"));
	default: return FText::FromString(TEXT("Unbekannt"));
	}
}

FText UWeatherLibrary::GetBiomeDisplayName(EBiomeType Biome)
{
	switch (Biome)
	{
	case EBiomeType::Temperate: return FText::FromString(TEXT("Gemäßigt"));
	case EBiomeType::Forest: return FText::FromString(TEXT("Wald"));
	case EBiomeType::Desert: return FText::FromString(TEXT("Wüste"));
	case EBiomeType::Arctic: return FText::FromString(TEXT("Arktis"));
	case EBiomeType::Tropical: return FText::FromString(TEXT("Tropisch"));
	case EBiomeType::Mountain: return FText::FromString(TEXT("Gebirge"));
	case EBiomeType::Swamp: return FText::FromString(TEXT("Sumpf"));
	case EBiomeType::Volcanic: return FText::FromString(TEXT("Vulkanisch"));
	case EBiomeType::Coastal: return FText::FromString(TEXT("Küste"));
	case EBiomeType::Magical: return FText::FromString(TEXT("Magisch"));
	default: return FText::FromString(TEXT("Unbekannt"));
	}
}

FString UWeatherLibrary::GetWeatherIconPath(EWeatherType Weather)
{
	switch (Weather)
	{
	case EWeatherType::Clear: return TEXT("/Game/UI/Icons/Weather/clear");
	case EWeatherType::PartlyCloudy: return TEXT("/Game/UI/Icons/Weather/partly_cloudy");
	case EWeatherType::Cloudy: return TEXT("/Game/UI/Icons/Weather/cloudy");
	case EWeatherType::Overcast: return TEXT("/Game/UI/Icons/Weather/overcast");
	case EWeatherType::LightRain: return TEXT("/Game/UI/Icons/Weather/light_rain");
	case EWeatherType::Rain: return TEXT("/Game/UI/Icons/Weather/rain");
	case EWeatherType::HeavyRain: return TEXT("/Game/UI/Icons/Weather/heavy_rain");
	case EWeatherType::Thunderstorm: return TEXT("/Game/UI/Icons/Weather/thunderstorm");
	case EWeatherType::LightSnow: return TEXT("/Game/UI/Icons/Weather/light_snow");
	case EWeatherType::Snow: return TEXT("/Game/UI/Icons/Weather/snow");
	case EWeatherType::Blizzard: return TEXT("/Game/UI/Icons/Weather/blizzard");
	case EWeatherType::Fog: return TEXT("/Game/UI/Icons/Weather/fog");
	case EWeatherType::DenseFog: return TEXT("/Game/UI/Icons/Weather/dense_fog");
	case EWeatherType::Sandstorm: return TEXT("/Game/UI/Icons/Weather/sandstorm");
	case EWeatherType::Hail: return TEXT("/Game/UI/Icons/Weather/hail");
	case EWeatherType::AuroraBorealis: return TEXT("/Game/UI/Icons/Weather/aurora");
	case EWeatherType::MagicStorm: return TEXT("/Game/UI/Icons/Weather/magic_storm");
	default: return TEXT("/Game/UI/Icons/Weather/unknown");
	}
}

FText UWeatherLibrary::GetWeatherDescription(EWeatherType Weather)
{
	switch (Weather)
	{
	case EWeatherType::Clear:
		return FText::FromString(TEXT("Der Himmel ist klar und die Sicht ist ausgezeichnet."));
	case EWeatherType::Thunderstorm:
		return FText::FromString(TEXT("Ein heftiges Gewitter tobt. Blitze zucken über den Himmel und Donner grollt."));
	case EWeatherType::Blizzard:
		return FText::FromString(TEXT("Ein tobender Schneesturm. Sicht stark eingeschränkt, Bewegung erschwert."));
	case EWeatherType::MagicStorm:
		return FText::FromString(TEXT("Arkane Energien durchziehen die Luft. Magische Kreaturen sind aktiver."));
	default:
		return GetWeatherDisplayName(Weather);
	}
}

// ============================================
// SIMULATION UPDATE
// ============================================

void UWeatherLibrary::UpdateWeatherSimulation(float DeltaTime)
{
	// Update time
	if (!TimeSettings.bTimeFrozen)
	{
		float HoursToAdd = DeltaTime / TimeSettings.SecondsPerGameHour;
		AdvanceTime(HoursToAdd);
	}

	// Handle weather transition
	if (CurrentWeather.bIsTransitioning)
	{
		CurrentWeather.TransitionProgress += DeltaTime / NextWeatherChangeTimer;

		if (CurrentWeather.TransitionProgress >= 1.0f)
		{
			SetWeather(CurrentWeather.NextWeatherType);
		}
		else
		{
			// Interpolate atmospheric values
			FWeatherEffects TargetEffects = CalculateWeatherEffects(CurrentWeather.NextWeatherType);
			CurrentWeather.Effects.VisibilityMultiplier = FMath::Lerp(
				CurrentWeather.Effects.VisibilityMultiplier,
				TargetEffects.VisibilityMultiplier,
				CurrentWeather.TransitionProgress
			);
			// ... similar for other effects
		}
	}
	else
	{
		// Count down to next weather change
		NextWeatherChangeTimer -= DeltaTime;
		if (NextWeatherChangeTimer <= 0)
		{
			EWeatherType NewWeather = PickRandomWeather(CurrentWeather.Biome, CurrentWeather.Season);
			TransitionToWeather(NewWeather, 120.0f); // 2 minute transition
		}
	}

	// Update wind with some variation
	CurrentWeather.Wind.Speed += FMath::RandRange(-0.5f, 0.5f);
	CurrentWeather.Wind.Speed = FMath::Clamp(CurrentWeather.Wind.Speed, 0.0f, 100.0f);
}

void UWeatherLibrary::InitializeWeatherSystem()
{
	InitializeDefaultWeatherEffects();
	InitializeDefaultBiomeConfigs();

	// Set initial state
	TimeSettings.CurrentHour = 12.0f;
	TimeSettings.DayOfYear = 1;
	TimeSettings.Year = 1;

	CurrentWeather.Biome = EBiomeType::Temperate;
	CurrentWeather.Season = ESeason::Summer;
	CurrentWeather.TimeOfDay = ETimeOfDay::Noon;
	CurrentWeather.MoonPhase = EMoonPhase::FullMoon;

	RandomizeWeather();
}

FString UWeatherLibrary::SaveWeatherStateToJSON()
{
	TSharedRef<FJsonObject> RootObject = MakeShared<FJsonObject>();

	RootObject->SetNumberField(TEXT("weather_type"), static_cast<int32>(CurrentWeather.WeatherType));
	RootObject->SetNumberField(TEXT("season"), static_cast<int32>(CurrentWeather.Season));
	RootObject->SetNumberField(TEXT("biome"), static_cast<int32>(CurrentWeather.Biome));
	RootObject->SetNumberField(TEXT("current_hour"), TimeSettings.CurrentHour);
	RootObject->SetNumberField(TEXT("day_of_year"), TimeSettings.DayOfYear);
	RootObject->SetNumberField(TEXT("year"), TimeSettings.Year);
	RootObject->SetNumberField(TEXT("temperature"), CurrentWeather.Atmosphere.Temperature);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(RootObject, Writer);

	return OutputString;
}

bool UWeatherLibrary::LoadWeatherStateFromJSON(const FString& JSONString)
{
	TSharedPtr<FJsonObject> RootObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JSONString);

	if (!FJsonSerializer::Deserialize(Reader, RootObject))
	{
		return false;
	}

	CurrentWeather.WeatherType = static_cast<EWeatherType>(RootObject->GetIntegerField(TEXT("weather_type")));
	CurrentWeather.Season = static_cast<ESeason>(RootObject->GetIntegerField(TEXT("season")));
	CurrentWeather.Biome = static_cast<EBiomeType>(RootObject->GetIntegerField(TEXT("biome")));
	TimeSettings.CurrentHour = RootObject->GetNumberField(TEXT("current_hour"));
	TimeSettings.DayOfYear = RootObject->GetIntegerField(TEXT("day_of_year"));
	TimeSettings.Year = RootObject->GetIntegerField(TEXT("year"));
	CurrentWeather.Atmosphere.Temperature = RootObject->GetNumberField(TEXT("temperature"));

	CurrentWeather.Effects = CalculateWeatherEffects(CurrentWeather.WeatherType);
	CurrentWeather.TimeOfDay = CalculateTimeOfDay(TimeSettings.CurrentHour);
	CurrentWeather.MoonPhase = CalculateMoonPhase(TimeSettings.DayOfYear);

	return true;
}

// ============================================
// PRIVATE HELPERS
// ============================================

void UWeatherLibrary::InitializeDefaultWeatherEffects()
{
	// Clear weather
	FWeatherEffects Clear;
	Clear.VisibilityMultiplier = 1.0f;
	Clear.MovementSpeedMultiplier = 1.0f;
	WeatherEffectsCache.Add(EWeatherType::Clear, Clear);

	// Rain
	FWeatherEffects Rain;
	Rain.VisibilityMultiplier = 0.7f;
	Rain.MovementSpeedMultiplier = 0.95f;
	Rain.FireDamageMultiplier = 0.8f;
	Rain.LightningDamageMultiplier = 1.2f;
	Rain.RangedAccuracyPenalty = 0.1f;
	Rain.SoundDetectionMultiplier = 0.7f;
	WeatherEffectsCache.Add(EWeatherType::Rain, Rain);

	// Heavy Rain
	FWeatherEffects HeavyRain;
	HeavyRain.VisibilityMultiplier = 0.4f;
	HeavyRain.MovementSpeedMultiplier = 0.85f;
	HeavyRain.StaminaDrainMultiplier = 1.2f;
	HeavyRain.FireDamageMultiplier = 0.5f;
	HeavyRain.LightningDamageMultiplier = 1.5f;
	HeavyRain.RangedAccuracyPenalty = 0.25f;
	HeavyRain.SoundDetectionMultiplier = 0.5f;
	WeatherEffectsCache.Add(EWeatherType::HeavyRain, HeavyRain);

	// Thunderstorm
	FWeatherEffects Storm;
	Storm.VisibilityMultiplier = 0.3f;
	Storm.MovementSpeedMultiplier = 0.8f;
	Storm.StaminaDrainMultiplier = 1.3f;
	Storm.FireDamageMultiplier = 0.3f;
	Storm.LightningDamageMultiplier = 2.0f;
	Storm.RangedAccuracyPenalty = 0.35f;
	Storm.SoundDetectionMultiplier = 0.3f;
	WeatherEffectsCache.Add(EWeatherType::Thunderstorm, Storm);

	// Blizzard
	FWeatherEffects Blizzard;
	Blizzard.VisibilityMultiplier = 0.2f;
	Blizzard.MovementSpeedMultiplier = 0.6f;
	Blizzard.StaminaDrainMultiplier = 1.5f;
	Blizzard.FireDamageMultiplier = 0.6f;
	Blizzard.IceDamageMultiplier = 1.5f;
	Blizzard.RangedAccuracyPenalty = 0.5f;
	Blizzard.ExposureDamagePerSecond = 2.0f;
	Blizzard.RequiredWarmth = 50.0f;
	WeatherEffectsCache.Add(EWeatherType::Blizzard, Blizzard);

	// Dense Fog
	FWeatherEffects DenseFog;
	DenseFog.VisibilityMultiplier = 0.15f;
	DenseFog.StealthDetectionMultiplier = 0.5f;
	DenseFog.RangedAccuracyPenalty = 0.4f;
	WeatherEffectsCache.Add(EWeatherType::DenseFog, DenseFog);

	// Magic Storm
	FWeatherEffects MagicStorm;
	MagicStorm.VisibilityMultiplier = 0.5f;
	MagicStorm.MovementSpeedMultiplier = 0.9f;
	MagicStorm.FireDamageMultiplier = 1.3f;
	MagicStorm.IceDamageMultiplier = 1.3f;
	MagicStorm.LightningDamageMultiplier = 1.3f;
	WeatherEffectsCache.Add(EWeatherType::MagicStorm, MagicStorm);
}

void UWeatherLibrary::InitializeDefaultBiomeConfigs()
{
	// Temperate Summer
	FBiomeWeatherProbabilities TemperateSummer;
	TemperateSummer.Biome = EBiomeType::Temperate;
	TemperateSummer.Season = ESeason::Summer;
	TemperateSummer.MinTemperature = 18.0f;
	TemperateSummer.MaxTemperature = 32.0f;
	TemperateSummer.WeatherProbabilities.Add(EWeatherType::Clear, 0.4f);
	TemperateSummer.WeatherProbabilities.Add(EWeatherType::PartlyCloudy, 0.25f);
	TemperateSummer.WeatherProbabilities.Add(EWeatherType::Cloudy, 0.15f);
	TemperateSummer.WeatherProbabilities.Add(EWeatherType::Rain, 0.1f);
	TemperateSummer.WeatherProbabilities.Add(EWeatherType::Thunderstorm, 0.1f);
	SetBiomeWeatherProbabilities(TemperateSummer);

	// Temperate Winter
	FBiomeWeatherProbabilities TemperateWinter;
	TemperateWinter.Biome = EBiomeType::Temperate;
	TemperateWinter.Season = ESeason::Winter;
	TemperateWinter.MinTemperature = -5.0f;
	TemperateWinter.MaxTemperature = 8.0f;
	TemperateWinter.WeatherProbabilities.Add(EWeatherType::Clear, 0.2f);
	TemperateWinter.WeatherProbabilities.Add(EWeatherType::Overcast, 0.3f);
	TemperateWinter.WeatherProbabilities.Add(EWeatherType::Snow, 0.25f);
	TemperateWinter.WeatherProbabilities.Add(EWeatherType::LightSnow, 0.15f);
	TemperateWinter.WeatherProbabilities.Add(EWeatherType::Fog, 0.1f);
	SetBiomeWeatherProbabilities(TemperateWinter);

	// Desert
	FBiomeWeatherProbabilities DesertConfig;
	DesertConfig.Biome = EBiomeType::Desert;
	DesertConfig.Season = ESeason::Summer;
	DesertConfig.MinTemperature = 25.0f;
	DesertConfig.MaxTemperature = 50.0f;
	DesertConfig.WeatherProbabilities.Add(EWeatherType::Clear, 0.7f);
	DesertConfig.WeatherProbabilities.Add(EWeatherType::PartlyCloudy, 0.15f);
	DesertConfig.WeatherProbabilities.Add(EWeatherType::Sandstorm, 0.15f);
	SetBiomeWeatherProbabilities(DesertConfig);

	// Arctic
	FBiomeWeatherProbabilities ArcticConfig;
	ArcticConfig.Biome = EBiomeType::Arctic;
	ArcticConfig.Season = ESeason::Winter;
	ArcticConfig.MinTemperature = -40.0f;
	ArcticConfig.MaxTemperature = -10.0f;
	ArcticConfig.WeatherProbabilities.Add(EWeatherType::Clear, 0.2f);
	ArcticConfig.WeatherProbabilities.Add(EWeatherType::Snow, 0.3f);
	ArcticConfig.WeatherProbabilities.Add(EWeatherType::Blizzard, 0.25f);
	ArcticConfig.WeatherProbabilities.Add(EWeatherType::AuroraBorealis, 0.25f);
	SetBiomeWeatherProbabilities(ArcticConfig);
}

FWeatherEffects UWeatherLibrary::CalculateWeatherEffects(EWeatherType Weather)
{
	if (const FWeatherEffects* Cached = WeatherEffectsCache.Find(Weather))
	{
		return *Cached;
	}

	// Return default effects
	FWeatherEffects Default;
	return Default;
}

FWeatherLighting UWeatherLibrary::CalculateLighting()
{
	FWeatherLighting Lighting;

	float Hour = TimeSettings.CurrentHour;
	bool bIsNight = IsNightTime();

	// Base sun intensity based on time of day
	if (bIsNight)
	{
		Lighting.SunIntensity = 0.0f;
		Lighting.AmbientIntensity = 0.1f;

		// Moon brightness based on phase
		switch (CurrentWeather.MoonPhase)
		{
		case EMoonPhase::FullMoon: Lighting.MoonIntensity = 0.3f; break;
		case EMoonPhase::WaxingGibbous:
		case EMoonPhase::WaningGibbous: Lighting.MoonIntensity = 0.2f; break;
		case EMoonPhase::FirstQuarter:
		case EMoonPhase::LastQuarter: Lighting.MoonIntensity = 0.1f; break;
		default: Lighting.MoonIntensity = 0.05f; break;
		}
	}
	else
	{
		// Sunrise/sunset color
		if (Hour < 8.0f || Hour > 18.0f)
		{
			Lighting.SunColor = FLinearColor(1.0f, 0.6f, 0.4f);
			Lighting.SunIntensity = 0.6f;
		}
		else if (Hour >= 11.0f && Hour <= 14.0f)
		{
			Lighting.SunIntensity = 1.0f;
		}
		else
		{
			Lighting.SunIntensity = 0.8f;
		}
	}

	// Modify based on weather
	switch (CurrentWeather.WeatherType)
	{
	case EWeatherType::Overcast:
	case EWeatherType::Cloudy:
		Lighting.SunIntensity *= 0.5f;
		Lighting.AmbientIntensity *= 1.3f;
		break;
	case EWeatherType::Rain:
	case EWeatherType::HeavyRain:
		Lighting.SunIntensity *= 0.3f;
		Lighting.AmbientIntensity *= 0.8f;
		Lighting.AmbientColor = FLinearColor(0.4f, 0.45f, 0.5f);
		break;
	case EWeatherType::Thunderstorm:
		Lighting.SunIntensity *= 0.2f;
		Lighting.AmbientColor = FLinearColor(0.3f, 0.35f, 0.45f);
		break;
	case EWeatherType::Fog:
	case EWeatherType::DenseFog:
		Lighting.SunIntensity *= 0.4f;
		Lighting.FogColor = FLinearColor(0.7f, 0.75f, 0.8f);
		break;
	case EWeatherType::MagicStorm:
		Lighting.AmbientColor = FLinearColor(0.5f, 0.3f, 0.7f);
		break;
	default:
		break;
	}

	return Lighting;
}

EWeatherType UWeatherLibrary::PickRandomWeather(EBiomeType Biome, ESeason Season)
{
	if (const TMap<ESeason, FBiomeWeatherProbabilities>* SeasonMap = BiomeConfigs.Find(Biome))
	{
		if (const FBiomeWeatherProbabilities* Config = SeasonMap->Find(Season))
		{
			float Random = FMath::FRand();
			float Accumulated = 0.0f;

			for (const auto& Pair : Config->WeatherProbabilities)
			{
				Accumulated += Pair.Value;
				if (Random <= Accumulated)
				{
					return Pair.Key;
				}
			}
		}
	}

	// Default fallback
	return EWeatherType::Clear;
}

ETimeOfDay UWeatherLibrary::CalculateTimeOfDay(float Hour)
{
	if (Hour >= 0.0f && Hour < 4.0f) return ETimeOfDay::Midnight;
	if (Hour >= 4.0f && Hour < 6.0f) return ETimeOfDay::Dawn;
	if (Hour >= 6.0f && Hour < 10.0f) return ETimeOfDay::Morning;
	if (Hour >= 10.0f && Hour < 14.0f) return ETimeOfDay::Noon;
	if (Hour >= 14.0f && Hour < 17.0f) return ETimeOfDay::Afternoon;
	if (Hour >= 17.0f && Hour < 19.0f) return ETimeOfDay::Evening;
	if (Hour >= 19.0f && Hour < 21.0f) return ETimeOfDay::Dusk;
	return ETimeOfDay::Night;
}

EMoonPhase UWeatherLibrary::CalculateMoonPhase(int32 DayOfYear)
{
	// Lunar cycle is approximately 29.5 days
	int32 Phase = (DayOfYear % 30) / 4;
	return static_cast<EMoonPhase>(Phase);
}

ESeason UWeatherLibrary::CalculateSeason(int32 DayOfYear)
{
	if (DayOfYear <= 91) return ESeason::Spring;
	if (DayOfYear <= 182) return ESeason::Summer;
	if (DayOfYear <= 273) return ESeason::Autumn;
	return ESeason::Winter;
}
