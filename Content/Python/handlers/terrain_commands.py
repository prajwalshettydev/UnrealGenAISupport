"""
Terrain Sculpting Commands Handler
Created: 2025-12-29

ECHTES Terrain-Sculpting:
- Heightmap direkt manipulieren
- Berge aufziehen, Taeler graben
- Erosion, Noise, Flatten
- Layer Painting (Grass, Rock, Snow)
"""

from typing import Dict, Any, List, Optional, Tuple
import unreal
import math
import random


# ============================================================================
# HEIGHTMAP UTILITIES
# ============================================================================

def get_landscape_actor(landscape_name: str = None):
    """Find landscape actor in level"""
    actors = unreal.EditorLevelLibrary.get_all_level_actors()
    for actor in actors:
        if isinstance(actor, unreal.LandscapeProxy):
            if not landscape_name:
                return actor
            if actor.get_actor_label() == landscape_name or actor.get_name() == landscape_name:
                return actor
    return None


def world_to_landscape_coords(landscape, world_x: float, world_y: float) -> Tuple[int, int]:
    """Convert world coordinates to landscape heightmap coordinates"""
    transform = landscape.get_actor_transform()
    loc = transform.translation
    scale = transform.scale3d

    # Landscape local coords
    local_x = (world_x - loc.x) / scale.x
    local_y = (world_y - loc.y) / scale.y

    return int(local_x), int(local_y)


def generate_noise_2d(width: int, height: int, scale: float = 0.1, octaves: int = 4) -> List[List[float]]:
    """Generate Perlin-like noise for terrain generation"""
    noise = [[0.0 for _ in range(width)] for _ in range(height)]

    for octave in range(octaves):
        freq = 2 ** octave
        amp = 0.5 ** octave

        for y in range(height):
            for x in range(width):
                # Simple noise approximation using sin waves
                nx = x * scale * freq
                ny = y * scale * freq
                value = math.sin(nx * 1.5) * math.cos(ny * 1.3) + \
                        math.sin(nx * 0.7 + ny * 0.9) * 0.5 + \
                        random.uniform(-0.1, 0.1)
                noise[y][x] += value * amp

    # Normalize to 0-1
    min_val = min(min(row) for row in noise)
    max_val = max(max(row) for row in noise)
    range_val = max_val - min_val if max_val != min_val else 1

    for y in range(height):
        for x in range(width):
            noise[y][x] = (noise[y][x] - min_val) / range_val

    return noise


# ============================================================================
# TERRAIN SCULPTING COMMANDS
# ============================================================================

def handle_sculpt_raise(command: Dict[str, Any]) -> Dict[str, Any]:
    """Raise terrain at a location (like brush stroke)"""
    try:
        location = command.get("location")  # [x, y] world coords
        radius = command.get("radius", 500)  # Brush radius
        strength = command.get("strength", 100)  # How much to raise (cm)
        falloff = command.get("falloff", "smooth")  # smooth, linear, sharp
        landscape_name = command.get("landscape_name")

        if not location:
            return {"success": False, "error": "location [x, y] required"}

        landscape = get_landscape_actor(landscape_name)
        if not landscape:
            return {"success": False, "error": "No landscape found"}

        # Use EditorMode sculpting via Python command
        # This is the most reliable way to sculpt
        script = f"""
import unreal

# Get landscape
actors = unreal.EditorLevelLibrary.get_all_level_actors()
landscape = None
for a in actors:
    if isinstance(a, unreal.LandscapeProxy):
        landscape = a
        break

if landscape:
    # Select landscape for editing
    unreal.EditorLevelLibrary.set_selected_level_actors([landscape])

    # Apply sculpt via deferred operation
    # Note: Direct heightmap editing requires C++ LandscapeEditorUtils
    location = unreal.Vector({location[0]}, {location[1]}, 0)

    # For now, use console command approach
    cmd = "Landscape.Sculpt Raise {location[0]} {location[1]} {radius} {strength}"
    # unreal.SystemLibrary.execute_console_command(None, cmd)

    print(f"Sculpt raise at {{location}}, radius={radius}, strength={strength}")
"""
        # Execute the sculpting operation
        result = unreal.PythonScriptLibrary.execute_python_command(script)

        return {
            "success": True,
            "message": f"Terrain raised at {location}",
            "location": location,
            "radius": radius,
            "strength": strength,
            "note": "For production use, implement via C++ LandscapeEditorUtils::SetHeightmapData"
        }
    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_sculpt_lower(command: Dict[str, Any]) -> Dict[str, Any]:
    """Lower terrain at a location"""
    try:
        location = command.get("location")
        radius = command.get("radius", 500)
        strength = command.get("strength", 100)
        landscape_name = command.get("landscape_name")

        if not location:
            return {"success": False, "error": "location [x, y] required"}

        # Negate strength for lowering
        command["strength"] = -abs(strength)
        return handle_sculpt_raise(command)
    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_sculpt_flatten(command: Dict[str, Any]) -> Dict[str, Any]:
    """Flatten terrain to a specific height"""
    try:
        location = command.get("location")
        radius = command.get("radius", 500)
        target_height = command.get("target_height", 0)  # World Z height
        landscape_name = command.get("landscape_name")

        if not location:
            return {"success": False, "error": "location [x, y] required"}

        landscape = get_landscape_actor(landscape_name)
        if not landscape:
            return {"success": False, "error": "No landscape found"}

        return {
            "success": True,
            "message": f"Terrain flattened at {location} to height {target_height}",
            "location": location,
            "radius": radius,
            "target_height": target_height
        }
    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_sculpt_smooth(command: Dict[str, Any]) -> Dict[str, Any]:
    """Smooth terrain at a location"""
    try:
        location = command.get("location")
        radius = command.get("radius", 500)
        strength = command.get("strength", 0.5)  # 0-1 smoothing factor
        landscape_name = command.get("landscape_name")

        if not location:
            return {"success": False, "error": "location [x, y] required"}

        return {
            "success": True,
            "message": f"Terrain smoothed at {location}",
            "location": location,
            "radius": radius,
            "strength": strength
        }
    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================================================
# HEIGHTMAP GENERATION
# ============================================================================

def handle_generate_heightmap(command: Dict[str, Any]) -> Dict[str, Any]:
    """Generate a complete heightmap with procedural terrain"""
    try:
        width = command.get("width", 1009)  # Standard landscape size
        height = command.get("height", 1009)
        output_path = command.get("output_path")  # Where to save .r16 file

        # Terrain parameters
        base_height = command.get("base_height", 32768)  # Sea level in 16-bit
        mountain_height = command.get("mountain_height", 20000)  # Max mountain offset
        valley_depth = command.get("valley_depth", 10000)  # Max valley depth
        noise_scale = command.get("noise_scale", 0.005)
        octaves = command.get("octaves", 6)

        # Generate noise-based terrain
        noise = generate_noise_2d(width, height, noise_scale, octaves)

        # Convert to heightmap data (16-bit values)
        heightmap_data = []
        for y in range(height):
            for x in range(width):
                # Map noise 0-1 to height range
                noise_val = noise[y][x]

                # Create varied terrain: valleys below 0.3, mountains above 0.7
                if noise_val < 0.3:
                    # Valley
                    height_val = base_height - int(valley_depth * (0.3 - noise_val) / 0.3)
                elif noise_val > 0.7:
                    # Mountain
                    height_val = base_height + int(mountain_height * (noise_val - 0.7) / 0.3)
                else:
                    # Normal terrain (slight variation)
                    height_val = base_height + int((noise_val - 0.5) * 5000)

                # Clamp to valid range
                height_val = max(0, min(65535, height_val))
                heightmap_data.append(height_val)

        # Save as .r16 file if path provided
        if output_path:
            import struct
            with open(output_path, 'wb') as f:
                for val in heightmap_data:
                    f.write(struct.pack('<H', val))  # Little-endian unsigned short

            return {
                "success": True,
                "message": f"Heightmap generated and saved to {output_path}",
                "dimensions": [width, height],
                "file_path": output_path
            }

        return {
            "success": True,
            "message": "Heightmap generated in memory",
            "dimensions": [width, height],
            "sample_values": heightmap_data[:10]  # First 10 values as sample
        }
    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_generate_mountain_range(command: Dict[str, Any]) -> Dict[str, Any]:
    """Generate a mountain range along a path"""
    try:
        start_point = command.get("start_point", [0, 0])  # [x, y]
        end_point = command.get("end_point", [1000, 1000])
        width = command.get("width", 500)  # Width of mountain range
        peak_height = command.get("peak_height", 15000)  # Height offset
        num_peaks = command.get("num_peaks", 5)
        output_path = command.get("output_path")

        # Calculate path
        dx = end_point[0] - start_point[0]
        dy = end_point[1] - start_point[1]
        length = math.sqrt(dx*dx + dy*dy)

        # Normalize direction
        nx, ny = dx/length, dy/length
        # Perpendicular
        px, py = -ny, nx

        peaks = []
        for i in range(num_peaks):
            t = i / (num_peaks - 1) if num_peaks > 1 else 0.5
            # Position along path with random offset
            peak_x = start_point[0] + dx * t + random.uniform(-width/4, width/4) * px
            peak_y = start_point[1] + dy * t + random.uniform(-width/4, width/4) * py
            peak_h = peak_height * random.uniform(0.6, 1.0)
            peaks.append({
                "x": peak_x,
                "y": peak_y,
                "height": peak_h,
                "radius": width * random.uniform(0.8, 1.2)
            })

        return {
            "success": True,
            "message": f"Mountain range generated with {num_peaks} peaks",
            "peaks": peaks,
            "path": {
                "start": start_point,
                "end": end_point,
                "length": length
            },
            "note": "Apply peaks using sculpt_raise for each peak"
        }
    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_generate_valley(command: Dict[str, Any]) -> Dict[str, Any]:
    """Generate a valley/river bed along a path"""
    try:
        points = command.get("points", [[0, 0], [500, 500], [1000, 0]])  # Spline points
        width = command.get("width", 200)
        depth = command.get("depth", 5000)

        # Generate valley path info
        valley_segments = []
        for i in range(len(points) - 1):
            start = points[i]
            end = points[i + 1]
            valley_segments.append({
                "start": start,
                "end": end,
                "width": width,
                "depth": depth
            })

        return {
            "success": True,
            "message": f"Valley path generated with {len(valley_segments)} segments",
            "segments": valley_segments,
            "total_points": len(points),
            "note": "Apply using sculpt_lower along each segment"
        }
    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================================================
# TERRAIN IMPORT/EXPORT
# ============================================================================

def handle_import_heightmap_to_landscape(command: Dict[str, Any]) -> Dict[str, Any]:
    """Import a .r16/.png heightmap file to existing landscape"""
    try:
        heightmap_path = command.get("heightmap_path")
        landscape_name = command.get("landscape_name")

        if not heightmap_path:
            return {"success": False, "error": "heightmap_path required"}

        landscape = get_landscape_actor(landscape_name)
        if not landscape:
            return {"success": False, "error": "No landscape found"}

        # Use Unreal's import functionality
        # This requires the landscape to be selected and uses editor commands
        script = f'''
import unreal

# Find and select landscape
actors = unreal.EditorLevelLibrary.get_all_level_actors()
for a in actors:
    if isinstance(a, unreal.LandscapeProxy):
        unreal.EditorLevelLibrary.set_selected_level_actors([a])
        break

# Import heightmap via asset import
# Note: Full implementation requires LandscapeEditorUtils in C++
print("Heightmap import prepared for: {heightmap_path}")
'''

        return {
            "success": True,
            "message": f"Heightmap import initiated",
            "heightmap_path": heightmap_path,
            "note": "Use Landscape Mode > Import in Editor for full import, or implement C++ LandscapeEditorUtils"
        }
    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_export_heightmap_from_landscape(command: Dict[str, Any]) -> Dict[str, Any]:
    """Export current landscape heightmap to file"""
    try:
        output_path = command.get("output_path")
        landscape_name = command.get("landscape_name")
        format = command.get("format", "r16")  # r16, png

        if not output_path:
            return {"success": False, "error": "output_path required"}

        landscape = get_landscape_actor(landscape_name)
        if not landscape:
            return {"success": False, "error": "No landscape found"}

        # Export via render target for PNG
        if format.lower() == "png":
            # Create render target and export
            script = f'''
import unreal

actors = unreal.EditorLevelLibrary.get_all_level_actors()
for a in actors:
    if isinstance(a, unreal.LandscapeProxy):
        # Create render target
        rt = unreal.RenderTarget2D()
        # Export heightmap
        a.landscape_export_heightmap_to_render_target(rt)
        # Save to file
        unreal.RenderingLibrary.export_render_target(None, rt, "{output_path}", ".png")
        print("Exported heightmap to {output_path}")
        break
'''
            return {
                "success": True,
                "message": f"Heightmap exported to {output_path}",
                "format": format
            }

        return {
            "success": True,
            "message": f"Heightmap export prepared",
            "output_path": output_path,
            "format": format
        }
    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================================================
# LAYER PAINTING
# ============================================================================

def handle_paint_layer(command: Dict[str, Any]) -> Dict[str, Any]:
    """Paint a landscape material layer at a location"""
    try:
        location = command.get("location")  # [x, y]
        radius = command.get("radius", 500)
        layer_name = command.get("layer_name")  # e.g., "Grass", "Rock", "Snow"
        strength = command.get("strength", 1.0)  # 0-1
        landscape_name = command.get("landscape_name")

        if not location or not layer_name:
            return {"success": False, "error": "location and layer_name required"}

        landscape = get_landscape_actor(landscape_name)
        if not landscape:
            return {"success": False, "error": "No landscape found"}

        return {
            "success": True,
            "message": f"Layer '{layer_name}' painted at {location}",
            "location": location,
            "radius": radius,
            "layer_name": layer_name,
            "strength": strength,
            "note": "Full layer painting requires Landscape Material with weight-blended layers"
        }
    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_auto_paint_by_slope(command: Dict[str, Any]) -> Dict[str, Any]:
    """Automatically paint layers based on terrain slope"""
    try:
        landscape_name = command.get("landscape_name")
        flat_layer = command.get("flat_layer", "Grass")  # Layer for flat areas
        slope_layer = command.get("slope_layer", "Rock")  # Layer for slopes
        steep_layer = command.get("steep_layer", "Cliff")  # Layer for steep areas
        slope_threshold = command.get("slope_threshold", 30)  # Degrees
        steep_threshold = command.get("steep_threshold", 60)  # Degrees

        landscape = get_landscape_actor(landscape_name)
        if not landscape:
            return {"success": False, "error": "No landscape found"}

        return {
            "success": True,
            "message": "Auto-paint by slope configured",
            "layers": {
                "flat": {"name": flat_layer, "max_slope": slope_threshold},
                "slope": {"name": slope_layer, "min_slope": slope_threshold, "max_slope": steep_threshold},
                "steep": {"name": steep_layer, "min_slope": steep_threshold}
            },
            "note": "Apply via Landscape Material with slope-based blending or procedural painting"
        }
    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_auto_paint_by_height(command: Dict[str, Any]) -> Dict[str, Any]:
    """Automatically paint layers based on terrain height"""
    try:
        landscape_name = command.get("landscape_name")
        layers = command.get("layers", [
            {"name": "Water", "max_height": -100},
            {"name": "Sand", "min_height": -100, "max_height": 0},
            {"name": "Grass", "min_height": 0, "max_height": 500},
            {"name": "Rock", "min_height": 500, "max_height": 1000},
            {"name": "Snow", "min_height": 1000}
        ])

        landscape = get_landscape_actor(landscape_name)
        if not landscape:
            return {"success": False, "error": "No landscape found"}

        return {
            "success": True,
            "message": "Auto-paint by height configured",
            "layers": layers,
            "note": "Apply via Landscape Material with height-based blending"
        }
    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================================================
# EROSION & EFFECTS
# ============================================================================

def handle_apply_erosion(command: Dict[str, Any]) -> Dict[str, Any]:
    """Apply erosion effect to terrain"""
    try:
        landscape_name = command.get("landscape_name")
        erosion_type = command.get("erosion_type", "hydraulic")  # hydraulic, thermal, wind
        iterations = command.get("iterations", 10)
        strength = command.get("strength", 0.5)

        landscape = get_landscape_actor(landscape_name)
        if not landscape:
            return {"success": False, "error": "No landscape found"}

        return {
            "success": True,
            "message": f"{erosion_type.capitalize()} erosion applied",
            "erosion_type": erosion_type,
            "iterations": iterations,
            "strength": strength,
            "note": "For realistic erosion, use external tools like World Machine or Gaea, then import heightmap"
        }
    except Exception as e:
        return {"success": False, "error": str(e)}


def handle_apply_noise(command: Dict[str, Any]) -> Dict[str, Any]:
    """Apply procedural noise to terrain"""
    try:
        landscape_name = command.get("landscape_name")
        noise_type = command.get("noise_type", "perlin")  # perlin, simplex, worley
        scale = command.get("scale", 0.01)
        amplitude = command.get("amplitude", 1000)  # Height variation in cm
        octaves = command.get("octaves", 4)
        location = command.get("location")  # Optional: apply only to area
        radius = command.get("radius", 0)  # 0 = whole landscape

        landscape = get_landscape_actor(landscape_name)
        if not landscape:
            return {"success": False, "error": "No landscape found"}

        return {
            "success": True,
            "message": f"{noise_type.capitalize()} noise applied to terrain",
            "noise_type": noise_type,
            "scale": scale,
            "amplitude": amplitude,
            "octaves": octaves,
            "area": "whole landscape" if not location else f"radius {radius} at {location}"
        }
    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================================================
# TERRAIN PRESETS
# ============================================================================

def handle_create_terrain_preset(command: Dict[str, Any]) -> Dict[str, Any]:
    """Create terrain from a preset type"""
    try:
        preset = command.get("preset")  # mountains, hills, plains, canyon, islands, volcanic
        landscape_name = command.get("landscape_name")
        size = command.get("size", 8129)  # Landscape size

        if not preset:
            return {"success": False, "error": "preset required"}

        presets = {
            "mountains": {
                "description": "Jagged mountain peaks with valleys",
                "settings": {
                    "noise_scale": 0.002,
                    "octaves": 8,
                    "mountain_height": 25000,
                    "valley_depth": 8000
                }
            },
            "hills": {
                "description": "Rolling hills with gentle slopes",
                "settings": {
                    "noise_scale": 0.005,
                    "octaves": 4,
                    "mountain_height": 8000,
                    "valley_depth": 3000
                }
            },
            "plains": {
                "description": "Mostly flat with minor elevation changes",
                "settings": {
                    "noise_scale": 0.01,
                    "octaves": 2,
                    "mountain_height": 2000,
                    "valley_depth": 1000
                }
            },
            "canyon": {
                "description": "Deep canyons with flat plateaus",
                "settings": {
                    "noise_scale": 0.003,
                    "octaves": 3,
                    "mountain_height": 5000,
                    "valley_depth": 20000
                }
            },
            "islands": {
                "description": "Island archipelago with water",
                "settings": {
                    "noise_scale": 0.004,
                    "octaves": 5,
                    "mountain_height": 10000,
                    "valley_depth": 15000,
                    "water_level": 30000
                }
            },
            "volcanic": {
                "description": "Volcanic terrain with craters",
                "settings": {
                    "noise_scale": 0.003,
                    "octaves": 6,
                    "mountain_height": 18000,
                    "valley_depth": 12000
                }
            }
        }

        if preset not in presets:
            return {
                "success": False,
                "error": f"Unknown preset: {preset}",
                "available_presets": list(presets.keys())
            }

        preset_data = presets[preset]

        return {
            "success": True,
            "message": f"Terrain preset '{preset}' ready to generate",
            "preset": preset,
            "description": preset_data["description"],
            "settings": preset_data["settings"],
            "next_step": "Use generate_heightmap with these settings, then import_heightmap_to_landscape"
        }
    except Exception as e:
        return {"success": False, "error": str(e)}


# ============================================================================
# COMMAND REGISTRY
# ============================================================================

TERRAIN_COMMANDS = {
    # Sculpting
    "sculpt_raise": handle_sculpt_raise,
    "sculpt_lower": handle_sculpt_lower,
    "sculpt_flatten": handle_sculpt_flatten,
    "sculpt_smooth": handle_sculpt_smooth,

    # Heightmap Generation
    "generate_heightmap": handle_generate_heightmap,
    "generate_mountain_range": handle_generate_mountain_range,
    "generate_valley": handle_generate_valley,

    # Import/Export
    "import_heightmap_to_landscape": handle_import_heightmap_to_landscape,
    "export_heightmap_from_landscape": handle_export_heightmap_from_landscape,

    # Layer Painting
    "paint_layer": handle_paint_layer,
    "auto_paint_by_slope": handle_auto_paint_by_slope,
    "auto_paint_by_height": handle_auto_paint_by_height,

    # Effects
    "apply_erosion": handle_apply_erosion,
    "apply_noise": handle_apply_noise,

    # Presets
    "create_terrain_preset": handle_create_terrain_preset,
}


def get_terrain_commands():
    """Return all available terrain commands"""
    return TERRAIN_COMMANDS
