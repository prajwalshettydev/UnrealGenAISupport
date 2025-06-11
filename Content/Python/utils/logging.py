import traceback
from typing import Any

import unreal


def log_info(message: str) -> None:
    """
    Log an informational message to the Unreal log

    Args:
        message: The message to log
    """
    unreal.log(f"[AI Plugin] {message}")


def log_warning(message: str) -> None:
    """
    Log a warning message to the Unreal log

    Args:
        message: The message to log
    """
    unreal.log_warning(f"[AI Plugin] {message}")


def log_error(message: str, include_traceback: bool = False) -> None:
    """
    Log an error message to the Unreal log

    Args:
        message: The message to log
        include_traceback: Whether to include the traceback in the log
    """
    error_message = f"[AI Plugin] ERROR: {message}"
    unreal.log_error(error_message)

    if include_traceback:
        tb = traceback.format_exc()
        unreal.log_error(f"[AI Plugin] Traceback:\n{tb}")


def log_command(command_type: str, details: Any = None) -> None:
    """
    Log a command being processed

    Args:
        command_type: The type of command being processed
        details: Optional details about the command
    """
    if details:
        unreal.log(f"[AI Plugin] Processing {command_type} command: {details}")
    else:
        unreal.log(f"[AI Plugin] Processing {command_type} command")


def log_result(command_type: str, success: bool, details: Any = None) -> None:
    """
    Log the result of a command

    Args:
        command_type: The type of command that was processed
        success: Whether the command was successful
        details: Optional details about the result
    """
    status = "successful" if success else "failed"

    if details:
        unreal.log(f"[AI Plugin] {command_type} command {status}: {details}")
    else:
        unreal.log(f"[AI Plugin] {command_type} command {status}")
