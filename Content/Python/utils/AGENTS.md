<!-- Parent: ../AGENTS.md -->
<!-- Generated: 2026-04-08 | Updated: 2026-04-08 -->

# Utils Package

## Purpose
Utility modules for logging and Unreal Engine type conversions. These are used throughout the MCP server and handlers to ensure consistent logging and cross-language data translation.

## Key Files
| File | Description |
|------|-------------|
| `__init__.py` | Package marker (empty) |
| `logging.py` | Structured logging with `[AI Plugin]` prefix for Unreal console |
| `unreal_conversions.py` | Python ↔ Unreal type conversion: tuples ↔ vectors/rotators/transforms |

## Logging Module (`logging.py`)

### Purpose
Consistent logging interface with `[AI Plugin]` prefix for visibility in Unreal's `Output.log`. All messages are routed through Unreal's logging system.

### API

#### `log_info(message: str) -> None`
Log informational message to Unreal console.
```python
log_info("Blueprint node created: K2Node_CallFunction_1")
```

#### `log_warning(message: str) -> None`
Log warning message to Unreal console (yellow in editor output).
```python
log_warning("Actor not found: player_1")
```

#### `log_error(message: str, include_traceback: bool = False) -> None`
Log error message to Unreal console (red in editor output). Optionally include Python traceback.
```python
log_error("Failed to apply patch", include_traceback=True)
```

#### `log_command(command_type: str, details: Any = None) -> None`
Log command dispatch for debugging MCP traffic.
```python
log_command("spawn", {"actor_class": "BP_NPC", "location": [0, 0, 100]})
```

#### `log_result(command_type: str, success: bool, details: Any = None) -> None`
Log command result (success or failure).
```python
log_result("add_node", success=True, details={"node_guid": "..."})
```

### Output Format
```
[AI Plugin] <message>
[AI Plugin] ERROR: <error>
[AI Plugin] Processing spawn command: {...}
```

## Unreal Conversions Module (`unreal_conversions.py`)

### Purpose
Translate between Python native types (tuples, lists, dicts) and Unreal Python API types (FVector, FRotator, FTransform, etc.).

### API

#### Vector Conversion
```python
def to_unreal_vector(python_tuple: Tuple[float, float, float]) -> unreal.Vector:
    """Convert (x, y, z) to FVector."""
    
def from_unreal_vector(vec: unreal.Vector) -> Tuple[float, float, float]:
    """Convert FVector to (x, y, z)."""
```

#### Rotator Conversion
```python
def to_unreal_rotator(python_tuple: Tuple[float, float, float]) -> unreal.Rotator:
    """Convert (pitch, yaw, roll) degrees to FRotator."""
    
def from_unreal_rotator(rot: unreal.Rotator) -> Tuple[float, float, float]:
    """Convert FRotator to (pitch, yaw, roll) degrees."""
```

#### Transform Conversion
```python
def to_unreal_transform(location, rotation, scale) -> unreal.Transform:
    """Combine location (vector), rotation (rotator), scale (vector) into FTransform."""
    
def from_unreal_transform(transform: unreal.Transform) -> dict:
    """Extract location, rotation, scale from FTransform."""
```

#### Matrix Operations
```python
def vector_from_matrix_row(matrix: unreal.Matrix, row: int) -> Tuple[float, float, float]:
    """Extract row vector from transformation matrix."""
```

### Usage Examples

```python
from utils import unreal_conversions as uc

# Spawn actor at (100, 200, 300) with yaw rotation of 45 degrees
location = uc.to_unreal_vector((100, 200, 300))
rotation = uc.to_unreal_rotator((0, 45, 0))  # pitch, yaw, roll

actor = unreal.spawn_actor_from_class(
    unreal.StaticMeshActor,
    location=location,
    rotation=rotation
)

# Read back and convert to Python
pos = uc.from_unreal_vector(actor.get_actor_location())
print(f"Actor at: {pos}")  # (100.0, 200.0, 300.0)
```

### Coordinate System Notes
- **Unreal**: right-handed, Z-up (X forward, Y right, Z up)
- **Python**: standard Unreal coordinate conventions apply
- **Angles**: Unreal rotators use degrees (not radians)
  - Pitch: rotation around Y axis (nose up/down)
  - Yaw: rotation around Z axis (turning left/right)
  - Roll: rotation around X axis (barrel roll)

## For AI Agents

### Working In This Directory
- Logging is read-only — add new functions if new log types needed, but do not modify logging prefix.
- Conversions must preserve precision — use float, not int, for all numeric types.
- Always test round-trip conversions: `to_*` then `from_*` should preserve data.

### Testing Requirements
- Unit test all conversions with edge cases:
  - Zero vectors/rotators
  - Large values (> 1000000)
  - Negative values
  - Floating-point precision edge cases
- Integration test logging output appears in Unreal `Output.log`.
- Verify locale-specific logging (e.g., Chinese-locale UE5) formats correctly.

### Common Patterns
- **Use conversions for ALL inter-type transfers** — do not build Unreal objects manually in Python.
- **Log every command dispatch and result** — this creates an audit trail for debugging MCP issues.
- **Always log errors with traceback** — aids in post-mortem analysis of failures.

### Dependencies
- `unreal` module (auto-available in UE Editor Python runtime)
- No external packages (standard library only)

<!-- MANUAL: -->
