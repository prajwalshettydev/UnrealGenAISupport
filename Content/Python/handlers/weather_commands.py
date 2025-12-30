# Weather Commands - Python Handler for WeatherLibrary
# Auto-generated for GenerativeAISupport Plugin

from typing import Dict, Any
import unreal
from utils import logging as log

# ============================================
# WEATHER STATE
# ============================================

def handle_get_weather(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get current weather state"""
    try:
        weather_type = unreal.WeatherLibrary.get_weather_type()
        season = unreal.WeatherLibrary.get_current_season()
        time_of_day = unreal.WeatherLibrary.get_time_of_day()
        biome = unreal.WeatherLibrary.get_current_biome()
        temperature = unreal.WeatherLibrary.get_temperature()

        return {
            "success": True,
            "weather_type": str(weather_type),
            "season": str(season),
            "time_of_day": str(time_of_day),
            "biome": str(biome),
            "temperature": temperature,
            "is_raining": unreal.WeatherLibrary.is_precipitating(),
            "is_night": unreal.WeatherLibrary.is_night_time()
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_set_weather(command: Dict[str, Any]) -> Dict[str, Any]:
    """Set weather type instantly"""
    try:
        weather_type = command.get("weather_type", "Clear")

        # Convert string to enum
        weather_enum = getattr(unreal.EWeatherType, weather_type.upper(), None)
        if weather_enum is None:
            return {"success": False, "error": f"Unknown weather type: {weather_type}"}

        unreal.WeatherLibrary.set_weather(weather_enum)
        log.log_info(f"Weather set to: {weather_type}")

        return {"success": True, "weather": weather_type}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_transition_weather(command: Dict[str, Any]) -> Dict[str, Any]:
    """Transition to new weather over time"""
    try:
        weather_type = command.get("weather_type", "Clear")
        duration = command.get("duration", 60.0)

        weather_enum = getattr(unreal.EWeatherType, weather_type.upper(), None)
        if weather_enum is None:
            return {"success": False, "error": f"Unknown weather type: {weather_type}"}

        unreal.WeatherLibrary.transition_to_weather(weather_enum, duration)

        return {"success": True, "weather": weather_type, "duration": duration}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_set_season(command: Dict[str, Any]) -> Dict[str, Any]:
    """Set current season"""
    try:
        season = command.get("season", "Summer")

        season_enum = getattr(unreal.ESeason, season.upper(), None)
        if season_enum is None:
            return {"success": False, "error": f"Unknown season: {season}"}

        unreal.WeatherLibrary.set_season(season_enum)

        return {"success": True, "season": season}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_set_biome(command: Dict[str, Any]) -> Dict[str, Any]:
    """Set current biome"""
    try:
        biome = command.get("biome", "Forest")

        biome_enum = getattr(unreal.EBiomeType, biome.upper(), None)
        if biome_enum is None:
            return {"success": False, "error": f"Unknown biome: {biome}"}

        unreal.WeatherLibrary.set_current_biome(biome_enum)

        return {"success": True, "biome": biome}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_randomize_weather(command: Dict[str, Any]) -> Dict[str, Any]:
    """Reset to random weather based on biome/season"""
    try:
        unreal.WeatherLibrary.randomize_weather()

        # Get new weather
        weather_type = unreal.WeatherLibrary.get_weather_type()

        return {"success": True, "new_weather": str(weather_type)}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# TIME SYSTEM
# ============================================

def handle_get_time(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get current game time"""
    try:
        time_str = unreal.WeatherLibrary.get_time_string()
        date_str = unreal.WeatherLibrary.get_date_string()
        sun_angle = unreal.WeatherLibrary.get_sun_angle()
        moon_phase = unreal.WeatherLibrary.get_moon_phase()

        return {
            "success": True,
            "time": time_str,
            "date": date_str,
            "sun_angle": sun_angle,
            "moon_phase": str(moon_phase)
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_set_time(command: Dict[str, Any]) -> Dict[str, Any]:
    """Set game hour (0-24)"""
    try:
        hour = command.get("hour", 12.0)

        unreal.WeatherLibrary.set_game_hour(hour)

        return {"success": True, "hour": hour}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_advance_time(command: Dict[str, Any]) -> Dict[str, Any]:
    """Advance time by hours"""
    try:
        hours = command.get("hours", 1.0)

        unreal.WeatherLibrary.advance_time(hours)
        time_str = unreal.WeatherLibrary.get_time_string()

        return {"success": True, "advanced_hours": hours, "new_time": time_str}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_set_time_scale(command: Dict[str, Any]) -> Dict[str, Any]:
    """Set time scale (real seconds per game hour)"""
    try:
        seconds_per_hour = command.get("seconds_per_hour", 60.0)

        unreal.WeatherLibrary.set_time_scale(seconds_per_hour)

        return {"success": True, "seconds_per_hour": seconds_per_hour}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_freeze_time(command: Dict[str, Any]) -> Dict[str, Any]:
    """Freeze or unfreeze time"""
    try:
        frozen = command.get("frozen", True)

        unreal.WeatherLibrary.set_time_frozen(frozen)

        return {"success": True, "frozen": frozen}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# WEATHER EFFECTS
# ============================================

def handle_get_weather_effects(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get current weather effects"""
    try:
        wind = unreal.WeatherLibrary.get_wind_vector()
        visibility = unreal.WeatherLibrary.get_visibility_distance()

        return {
            "success": True,
            "wind": {"x": wind.x, "y": wind.y, "z": wind.z},
            "visibility": visibility
        }
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# FORECAST
# ============================================

def handle_get_forecast(command: Dict[str, Any]) -> Dict[str, Any]:
    """Get weather forecast"""
    try:
        hours_ahead = command.get("hours_ahead", 24)

        forecast = unreal.WeatherLibrary.get_forecast(hours_ahead)

        forecast_list = []
        for f in forecast:
            forecast_list.append({
                "hour": f.hour,
                "weather": str(f.weather_type)
            })

        return {"success": True, "forecast": forecast_list}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# WEATHER EVENTS
# ============================================

def handle_trigger_weather_event(command: Dict[str, Any]) -> Dict[str, Any]:
    """Trigger a weather event by ID"""
    try:
        event_id = command.get("event_id", "")

        if not event_id:
            return {"success": False, "error": "event_id required"}

        unreal.WeatherLibrary.trigger_weather_event(event_id)

        return {"success": True, "event_id": event_id}
    except Exception as e:
        return {"success": False, "error": str(e)}

def handle_stop_weather_event(command: Dict[str, Any]) -> Dict[str, Any]:
    """Stop current weather event"""
    try:
        unreal.WeatherLibrary.stop_weather_event()

        return {"success": True}
    except Exception as e:
        return {"success": False, "error": str(e)}

# ============================================
# COMMAND REGISTRY
# ============================================

WEATHER_COMMANDS = {
    # State
    "weather_get": handle_get_weather,
    "weather_set": handle_set_weather,
    "weather_transition": handle_transition_weather,
    "weather_set_season": handle_set_season,
    "weather_set_biome": handle_set_biome,
    "weather_randomize": handle_randomize_weather,

    # Time
    "weather_get_time": handle_get_time,
    "weather_set_time": handle_set_time,
    "weather_advance_time": handle_advance_time,
    "weather_set_time_scale": handle_set_time_scale,
    "weather_freeze_time": handle_freeze_time,

    # Effects
    "weather_get_effects": handle_get_weather_effects,

    # Forecast
    "weather_get_forecast": handle_get_forecast,

    # Events
    "weather_trigger_event": handle_trigger_weather_event,
    "weather_stop_event": handle_stop_weather_event,
}
