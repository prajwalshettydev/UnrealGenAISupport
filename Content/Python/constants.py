"""Shared constants for the GenerativeAISupport MCP pipeline.

Single source of truth for values that were previously duplicated across
mcp_server.py, unreal_socket_server.py, and handler modules.
"""

DEFAULT_HOST = "localhost"
DEFAULT_PORT = 9877

# Destructive script patterns (regex) — used by execute_python_script safety checks.
# Previously duplicated in mcp_server.py and python_commands.py.
DESTRUCTIVE_SCRIPT_PATTERNS = [
    r"unreal\.EditorAssetLibrary\.delete_asset",
    r"unreal\.EditorLevelLibrary\.destroy_actor",
    r"os\.remove",
    r"shutil\.rmtree",
]

# Destructive console command keywords — used by execute_unreal_command safety checks.
DESTRUCTIVE_COMMAND_KEYWORDS = ["delete", "save", "quit", "exit", "restart"]
