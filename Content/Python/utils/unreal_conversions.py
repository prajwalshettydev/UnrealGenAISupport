from typing import List, Optional, Tuple, Union

import unreal


def to_unreal_vector(
    vector_data: Union[List[float], Tuple[float, float, float]],
) -> unreal.Vector:
    """
    Convert a list or tuple of 3 floats to an Unreal Vector

    Args:
        vector_data: A list or tuple containing 3 floats [x, y, z]

    Returns:
        An unreal.Vector object
    """
    if not isinstance(vector_data, (list, tuple)) or len(vector_data) != 3:
        raise ValueError("Vector data must be a list or tuple of 3 floats")

    return unreal.Vector(vector_data[0], vector_data[1], vector_data[2])


def to_unreal_rotator(
    rotation_data: Union[List[float], Tuple[float, float, float]],
) -> unreal.Rotator:
    """
    Convert a list or tuple of 3 floats to an Unreal Rotator

    Args:
        rotation_data: A list or tuple containing 3 floats [pitch, yaw, roll]

    Returns:
        An unreal.Rotator object
    """
    if not isinstance(rotation_data, (list, tuple)) or len(rotation_data) != 3:
        raise ValueError("Rotation data must be a list or tuple of 3 floats")

    return unreal.Rotator(rotation_data[0], rotation_data[1], rotation_data[2])


def to_unreal_color(
    color_data: Union[List[float], Tuple[float, float, float]],
) -> unreal.LinearColor:
    """
    Convert a list or tuple of 3 floats to an Unreal LinearColor

    Args:
        color_data: A list or tuple containing 3 floats [r, g, b]

    Returns:
        An unreal.LinearColor object with alpha=1.0
    """
    if not isinstance(color_data, (list, tuple)) or len(color_data) < 3:
        raise ValueError("Color data must be a list or tuple of at least 3 floats")

    alpha = 1.0
    if len(color_data) > 3:
        alpha = color_data[3]

    return unreal.LinearColor(color_data[0], color_data[1], color_data[2], alpha)
