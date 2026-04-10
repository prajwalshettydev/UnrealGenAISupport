"""Command Registry — single source of truth for socket command definitions.

Each command is defined once via the @registry.command() decorator on its handler
function. The registry generates:
  - Socket dispatch table (for unreal_socket_server.py)
  - Parameter validation
  - Alias mapping
  - Destructive metadata

Usage in handler modules:
    from command_registry import registry

    @registry.command("add_node", category="blueprint",
                      mutates_blueprint=True)
    def handle_add_node(command):
        ...

Usage in unreal_socket_server.py:
    from command_registry import registry
    self.handlers = registry.build_dispatch_table()
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any, Callable, Dict, List, Optional


@dataclass
class ParamDef:
    """Definition of a command parameter."""

    type: str = "str"
    required: bool = False
    default: Any = None
    description: str = ""


@dataclass
class CommandDef:
    """Single command definition — the contract for one socket command."""

    name: str
    handler: Callable
    category: str
    aliases: List[str] = field(default_factory=list)
    destructive: bool = False
    mutates_blueprint: bool = False
    requires_confirmation: bool = False
    params: Dict[str, ParamDef] = field(default_factory=dict)
    description: str = ""


class CommandRegistry:
    """Central registry for all socket commands.

    Commands are registered via the @registry.command() decorator.
    The dispatcher calls registry.build_dispatch_table() at startup.
    """

    def __init__(self):
        self._commands: Dict[str, CommandDef] = {}
        self._aliases: Dict[str, str] = {}  # alias -> canonical name

    # ------------------------------------------------------------------
    # Registration
    # ------------------------------------------------------------------

    def command(
        self,
        name: str,
        *,
        category: str = "general",
        aliases: Optional[List[str]] = None,
        destructive: bool = False,
        mutates_blueprint: bool = False,
        requires_confirmation: bool = False,
        params: Optional[Dict[str, ParamDef]] = None,
        description: str = "",
    ):
        """Decorator that registers a handler function as a command."""

        def decorator(func: Callable) -> Callable:
            cmd = CommandDef(
                name=name,
                handler=func,
                category=category,
                aliases=aliases or [],
                destructive=destructive,
                mutates_blueprint=mutates_blueprint,
                requires_confirmation=requires_confirmation,
                params=params or {},
                description=description or func.__doc__ or "",
            )
            self._register(cmd)
            return func

        return decorator

    def _register(self, cmd: CommandDef):
        if cmd.name in self._commands:
            raise ValueError(f"Duplicate command name: {cmd.name}")
        self._commands[cmd.name] = cmd
        for alias in cmd.aliases:
            if alias in self._aliases or alias in self._commands:
                raise ValueError(
                    f"Alias '{alias}' conflicts with existing command or alias"
                )
            self._aliases[alias] = cmd.name

    def clear(self):
        """Reset all registered commands and aliases.

        Called by CommandDispatcher._handle_reload() before importlib.reload()
        so that handler modules can re-register their commands without hitting
        the duplicate-name check in _register(). The live dispatch table
        (self.handlers on the dispatcher) is NOT affected until
        _build_handler_map() is explicitly called.
        """
        self._commands = {}
        self._aliases = {}

    def snapshot(self) -> Dict[str, Dict[str, Any]]:
        """Capture the current registry state for transactional reloads."""
        return {
            "commands": dict(self._commands),
            "aliases": dict(self._aliases),
        }

    def restore(self, snapshot: Dict[str, Dict[str, Any]]):
        """Restore a previously captured registry snapshot."""
        self._commands = dict(snapshot.get("commands", {}))
        self._aliases = dict(snapshot.get("aliases", {}))

    # ------------------------------------------------------------------
    # Dispatch table generation
    # ------------------------------------------------------------------

    def build_dispatch_table(self) -> Dict[str, Callable]:
        """Build {command_type: handler} dict for the socket dispatcher."""
        table: Dict[str, Callable] = {}
        for name, cmd in self._commands.items():
            table[name] = cmd.handler
            for alias in cmd.aliases:
                table[alias] = cmd.handler
        return table

    # ------------------------------------------------------------------
    # Queries
    # ------------------------------------------------------------------

    def get(self, name: str) -> Optional[CommandDef]:
        """Look up a command by name or alias."""
        canonical = self._aliases.get(name, name)
        return self._commands.get(canonical)

    def all_commands(self) -> List[CommandDef]:
        """Return all registered commands."""
        return list(self._commands.values())

    def command_names(self) -> set[str]:
        """Return the canonical command names currently registered."""
        return set(self._commands.keys())

    def by_category(self, category: str) -> List[CommandDef]:
        """Return commands in a given category."""
        return [c for c in self._commands.values() if c.category == category]

    @property
    def count(self) -> int:
        return len(self._commands)

    # ------------------------------------------------------------------
    # Startup validation
    # ------------------------------------------------------------------

    def validate(
        self,
        expected_count: Optional[int] = None,
        expected_names: Optional[set[str]] = None,
    ):
        """Run startup checks. Call after all modules are imported.

        Raises ValueError if any check fails.
        """
        errors = []

        # Check all handlers are callable
        for cmd in self._commands.values():
            if not callable(cmd.handler):
                errors.append(f"Command '{cmd.name}': handler is not callable")

        # Check expected count
        if expected_count is not None and self.count != expected_count:
            errors.append(
                f"Expected {expected_count} commands, found {self.count}. "
                f"A handler module may not have been imported."
            )

        if expected_names is not None:
            missing_names = sorted(expected_names - self.command_names())
            if missing_names:
                errors.append(
                    "Reload lost previously available commands: "
                    + ", ".join(missing_names)
                )

        if errors:
            raise ValueError(
                "Command registry validation failed:\n  " + "\n  ".join(errors)
            )


# Global singleton — handler modules import this and decorate their functions.
registry = CommandRegistry()
