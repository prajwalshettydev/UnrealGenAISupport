"""Regression tests for blueprint patch rollback semantics.

Run with:
    uv run python -m pytest tests/test_patch_rollback.py -v
or:
    python -m pytest tests/test_patch_rollback.py -v
"""

from __future__ import annotations

import sys
import unittest
from pathlib import Path


PYTHON_ROOT = Path(__file__).resolve().parents[1]
if str(PYTHON_ROOT) not in sys.path:
    sys.path.insert(0, str(PYTHON_ROOT))

from tools.patch import _patch_finalize, _patch_resolve_refs  # noqa: E402


class TestPatchResolveRefs(unittest.TestCase):
    def test_preflight_reject_is_captured_without_mutating(self):
        calls = []

        def fake_send(command):
            calls.append(command["type"])
            if command["type"] == "get_all_nodes_with_details":
                return {"success": True, "nodes": []}
            if command["type"] == "preflight_blueprint_patch":
                return {
                    "success": True,
                    "valid": False,
                    "issues": [
                        {
                            "severity": "error",
                            "message": "Pin 'then' not found",
                        }
                    ],
                }
            raise AssertionError(f"Unexpected command: {command}")

        patch = {
            "add_nodes": [{"ref_id": "NewNode", "node_type": "K2Node_CallFunction"}],
            "add_connections": [{"from": "A.then", "to": "B.execute"}],
        }

        ctx = _patch_resolve_refs(patch, "/Game/BP_Test", "EventGraph", fake_send)

        self.assertEqual(
            calls,
            [
                "get_all_nodes_with_details",
                "preflight_blueprint_patch",
                "list_graphs",
            ],
        )
        self.assertIsNotNone(ctx["preflight_reject"])
        self.assertEqual(ctx["patch_log_updates"]["auto_preflight"]["issues"], 1)


class TestPatchFinalize(unittest.TestCase):
    def test_finalize_marks_complete_rollback_when_no_ghost_nodes_remain(self):
        commands = []

        def fake_send(command):
            commands.append(command["type"])
            if command["type"] == "cancel_transaction":
                return {"success": True}
            if command["type"] == "remove_unused_nodes":
                return {"success": True}
            if command["type"] == "get_node_details":
                return {"success": False, "error": "Node not found"}
            if command["type"] == "list_graphs":
                return {
                    "success": True,
                    "graphs": [
                        {
                            "graph_name": "EventGraph",
                            "graph_type": "EventGraph",
                            "node_count": 0,
                        }
                    ],
                }
            return {"success": True}

        results = {
            "success": False,
            "errors": ["compile_error: bad pin"],
            "created_nodes": {"NewNode": "GUID-1"},
            "_txn_started": True,
        }
        patch_log = {"phases": {}, "error_categories": []}

        finalized = _patch_finalize("/Game/BP_Test", "EventGraph", results, patch_log, fake_send, False)

        self.assertTrue(finalized["rolled_back"])
        self.assertEqual(finalized["rollback_status"], "complete")
        self.assertEqual(finalized["created_nodes"]["NewNode"]["status"], "rolled_back")
        self.assertFalse(patch_log.get("rollback_partial", False))
        self.assertIn("cancel_transaction", commands)
        self.assertIn("remove_unused_nodes", commands)

    def test_finalize_marks_partial_rollback_when_ghost_nodes_survive(self):
        def fake_send(command):
            if command["type"] == "cancel_transaction":
                return {"success": True}
            if command["type"] == "remove_unused_nodes":
                return {"success": True}
            if command["type"] == "get_node_details":
                return {"success": True, "title": "GhostNode"}
            if command["type"] == "list_graphs":
                return {
                    "success": True,
                    "graphs": [
                        {
                            "graph_name": "EventGraph",
                            "graph_type": "EventGraph",
                            "node_count": 1,
                        }
                    ],
                }
            return {"success": True}

        results = {
            "success": False,
            "errors": ["connect_bulk: failed"],
            "created_nodes": {"GhostNodeRef": "GUID-GHOST"},
            "_txn_started": True,
        }
        patch_log = {"phases": {}, "error_categories": []}

        finalized = _patch_finalize("/Game/BP_Test", "EventGraph", results, patch_log, fake_send, False)

        self.assertTrue(finalized["rolled_back"])
        self.assertEqual(finalized["rollback_status"], "partial")
        self.assertEqual(finalized["created_nodes"]["GhostNodeRef"]["status"], "ghost")
        self.assertTrue(patch_log["rollback_partial"])
        self.assertEqual(patch_log["ghost_nodes"][0]["guid"], "GUID-GHOST")
        warning_codes = [item["code"] for item in finalized.get("diagnostics", [])]
        self.assertIn("ROLLBACK_INCOMPLETE", warning_codes)


if __name__ == "__main__":
    unittest.main()
