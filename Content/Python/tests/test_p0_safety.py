"""P0 Server Safety — Regression tests for timeout cancel semantics and reload-safe registry.

Run with: uv run python -m pytest tests/test_p0_safety.py -v
No live UE Editor required — unreal module is stubbed at import time.
"""
import importlib
import sys
import threading
import types
import unittest
from pathlib import Path
from unittest.mock import MagicMock, patch

# ---------------------------------------------------------------------------
# Stub the `unreal` module before any project imports touch it.
# ---------------------------------------------------------------------------
PYTHON_ROOT = Path(__file__).resolve().parents[1]
if str(PYTHON_ROOT) not in sys.path:
    sys.path.insert(0, str(PYTHON_ROOT))

_unreal_stub = types.ModuleType("unreal")
# Stub UE API calls made at module-load time in unreal_socket_server.py
_unreal_stub.register_slate_post_tick_callback = MagicMock()
sys.modules.setdefault("unreal", _unreal_stub)

# Stub sub-modules referenced transitively
for _mod in ("utils", "utils.logging", "utils.unreal_conversions"):
    sys.modules.setdefault(_mod, types.ModuleType(_mod))

# Stub utils.logging functions used by unreal_socket_server
_log_stub = sys.modules["utils.logging"]
for _fn in ("log_info", "log_warning", "log_error", "log_command", "log_result"):
    setattr(_log_stub, _fn, MagicMock())

# Patch threading.Thread to prevent actual socket server startup during tests
_real_thread = threading.Thread
_thread_patcher = patch("threading.Thread", MagicMock(return_value=MagicMock(start=MagicMock())))
_thread_patcher.start()


# ---------------------------------------------------------------------------
# P0-3 Tests: Timeout Cancel Semantics
# ---------------------------------------------------------------------------

class TestTimeoutCancelSemantics(unittest.TestCase):
    """Tests for the cancellation-set mechanism in unreal_socket_server."""

    def setUp(self):
        # Import fresh module state for each test
        import unreal_socket_server as uss
        self.uss = uss
        # Reset shared state between tests
        uss.command_queue.clear()
        uss.response_dict.clear()
        with uss._cancel_lock:
            uss.cancelled_ids.clear()

    def test_timeout_adds_to_cancelled_ids(self):
        """Socket-side timeout must add command_id to cancelled_ids and clean response_dict."""
        uss = self.uss
        command_id = 9001
        uss.response_dict[command_id] = {"success": True}  # simulate orphan entry

        with uss._cancel_lock:
            uss.cancelled_ids.add(command_id)
        uss.response_dict.pop(command_id, None)

        self.assertIn(command_id, uss.cancelled_ids)
        self.assertNotIn(command_id, uss.response_dict)

    def test_process_commands_skips_cancelled(self):
        """process_commands must skip a command that is in cancelled_ids."""
        uss = self.uss
        command_id = 42
        command = {"type": "test_command"}
        uss.command_queue.append((command_id, command))
        with uss._cancel_lock:
            uss.cancelled_ids.add(command_id)

        with patch.object(uss.dispatcher, "dispatch") as mock_dispatch:
            uss.process_commands()
            mock_dispatch.assert_not_called()

        self.assertNotIn(command_id, uss.cancelled_ids)
        self.assertNotIn(command_id, uss.response_dict)

    def test_normal_command_not_affected(self):
        """process_commands must execute a command that is NOT in cancelled_ids."""
        uss = self.uss
        command_id = 99
        command = {"type": "handshake"}
        uss.command_queue.append((command_id, command))

        expected = {"success": True, "message": "ok"}
        with patch.object(uss.dispatcher, "dispatch", return_value=expected) as mock_dispatch:
            uss.process_commands()
            mock_dispatch.assert_called_once_with(command)

        self.assertEqual(uss.response_dict.get(command_id), expected)
        self.assertNotIn(command_id, uss.cancelled_ids)

    def test_cancelled_id_removed_after_skip(self):
        """cancelled_ids must not grow unboundedly — entry is discarded after skip."""
        uss = self.uss
        command_id = 77
        uss.command_queue.append((command_id, {"type": "noop"}))
        with uss._cancel_lock:
            uss.cancelled_ids.add(command_id)

        with patch.object(uss.dispatcher, "dispatch"):
            uss.process_commands()

        with uss._cancel_lock:
            self.assertNotIn(command_id, uss.cancelled_ids)


# ---------------------------------------------------------------------------
# P0-4 Tests: Reload-Safe Registry
# ---------------------------------------------------------------------------

# Import registry once — tests manipulate it directly
from command_registry import CommandRegistry


class TestReloadSafeRegistry(unittest.TestCase):
    """Tests for CommandRegistry.clear() supporting hot-reload."""

    def setUp(self):
        self.registry = CommandRegistry()

    def _make_handler(self, name="handler"):
        def handler(cmd):
            return {"success": True}
        handler.__name__ = name
        return handler

    def test_clear_resets_commands_and_aliases(self):
        """clear() must empty both _commands and _aliases."""
        handler = self._make_handler()
        self.registry.command("foo", aliases=["f"])(handler)
        self.assertIn("foo", self.registry._commands)
        self.assertIn("f", self.registry._aliases)

        self.registry.clear()

        self.assertEqual(self.registry._commands, {})
        self.assertEqual(self.registry._aliases, {})

    def test_register_after_clear_succeeds(self):
        """Registering same command name after clear() must not raise ValueError."""
        handler1 = self._make_handler("h1")
        handler2 = self._make_handler("h2")
        self.registry.command("foo")(handler1)
        self.registry.clear()
        # Should not raise
        self.registry.command("foo")(handler2)
        cmd = self.registry.get("foo")
        self.assertIsNotNone(cmd)
        self.assertIs(cmd.handler, handler2)

    def test_register_alias_after_clear_succeeds(self):
        """Registering same alias after clear() must not raise ValueError."""
        handler1 = self._make_handler("h1")
        handler2 = self._make_handler("h2")
        self.registry.command("foo", aliases=["f"])(handler1)
        self.registry.clear()
        # Should not raise
        self.registry.command("bar", aliases=["f"])(handler2)
        self.assertIn("f", self.registry._aliases)

    def test_handler_identity_after_clear_and_reregister(self):
        """After clear+re-register, dispatch table must point to the NEW handler, not old."""
        old_handler = self._make_handler("old")
        new_handler = self._make_handler("new")
        self.registry.command("cmd")(old_handler)
        self.registry.clear()
        self.registry.command("cmd")(new_handler)

        table = self.registry.build_dispatch_table()
        self.assertIs(table["cmd"], new_handler)
        self.assertIsNot(table["cmd"], old_handler)


if __name__ == "__main__":
    unittest.main()
