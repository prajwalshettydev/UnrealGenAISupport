"""Regression tests for bp_cpp_api freshness logic.

Run with:
    uv run python -m pytest tests/test_bp_api_freshness.py -v
or:
    python -m pytest tests/test_bp_api_freshness.py -v
"""

from __future__ import annotations

import importlib.util
import json
import os
import shutil
import sys
import unittest
import uuid
from pathlib import Path
from unittest.mock import patch


def _load_export_module():
    repo_root = Path(__file__).resolve().parents[5]
    script_path = repo_root / "Scripts" / "export_cpp_bp_api.py"
    spec = importlib.util.spec_from_file_location("export_cpp_bp_api", script_path)
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


class TestBpApiFreshness(unittest.TestCase):
    def setUp(self):
        self.mod = _load_export_module()
        self._temp_roots: list[Path] = []

    def tearDown(self):
        for path in reversed(self._temp_roots):
            shutil.rmtree(path, ignore_errors=True)

    def _make_project(self) -> Path:
        temp_root = Path(__file__).resolve().parent / "_tmp"
        temp_root.mkdir(exist_ok=True)
        temp_dir = temp_root / f"case_{uuid.uuid4().hex}"
        os.makedirs(temp_dir, exist_ok=False)
        self._temp_roots.append(temp_dir)
        (temp_dir / "Source" / "Demo" / "Public").mkdir(parents=True)
        (temp_dir / "docs" / "blueprint_knowledge").mkdir(parents=True)
        (temp_dir / ".claude").mkdir()
        (temp_dir / "Demo.uproject").write_text(
            json.dumps({"EngineAssociation": "5.7"}),
            encoding="utf-8",
        )
        return temp_dir

    def test_check_freshness_reports_missing_when_artifacts_absent(self):
        project_root = self._make_project()

        with patch("builtins.print") as mock_print:
            exit_code = self.mod.check_freshness(project_root)

        self.assertEqual(exit_code, 1)
        payload = json.loads(mock_print.call_args.args[0])
        self.assertEqual(payload["status"], "missing")

    def test_check_freshness_reports_fresh_after_export(self):
        project_root = self._make_project()
        header_path = project_root / "Source" / "Demo" / "Public" / "DemoBlueprintLib.h"
        header_path.write_text(
            "\n".join(
                [
                    "#pragma once",
                    "UCLASS(BlueprintType)",
                    "class UDemoBlueprintLib {",
                    "};",
                ]
            ),
            encoding="utf-8",
        )

        export_code = self.mod.export_snapshot(
            project_root,
            "DemoEditor",
            str(project_root / "Demo.uproject"),
        )
        self.assertEqual(export_code, 0)

        with patch("builtins.print") as mock_print:
            exit_code = self.mod.check_freshness(project_root)

        self.assertEqual(exit_code, 0)
        payload = json.loads(mock_print.call_args.args[0])
        self.assertEqual(payload["status"], "fresh")

    def test_check_freshness_reports_stale_when_header_set_changes(self):
        project_root = self._make_project()
        first_header = project_root / "Source" / "Demo" / "Public" / "DemoBlueprintLib.h"
        first_header.write_text("#pragma once\n", encoding="utf-8")

        export_code = self.mod.export_snapshot(
            project_root,
            "DemoEditor",
            str(project_root / "Demo.uproject"),
        )
        self.assertEqual(export_code, 0)

        second_header = project_root / "Source" / "Demo" / "Public" / "NewBlueprintType.h"
        second_header.write_text("#pragma once\n", encoding="utf-8")

        with patch("builtins.print") as mock_print:
            exit_code = self.mod.check_freshness(project_root)

        self.assertEqual(exit_code, 1)
        payload = json.loads(mock_print.call_args.args[0])
        self.assertEqual(payload["status"], "stale")
        self.assertEqual(payload["reason"], "header set changed")
        self.assertIn("Source/Demo/Public/NewBlueprintType.h", payload["added"])

    def test_check_freshness_reports_stale_when_header_contents_change(self):
        project_root = self._make_project()
        header_path = project_root / "Source" / "Demo" / "Public" / "DemoBlueprintLib.h"
        header_path.write_text("#pragma once\n", encoding="utf-8")

        export_code = self.mod.export_snapshot(
            project_root,
            "DemoEditor",
            str(project_root / "Demo.uproject"),
        )
        self.assertEqual(export_code, 0)

        header_path.write_text(
            "#pragma once\nUSTRUCT(BlueprintType)\nstruct FDemoType {};\n",
            encoding="utf-8",
        )

        with patch("builtins.print") as mock_print:
            exit_code = self.mod.check_freshness(project_root)

        self.assertEqual(exit_code, 1)
        payload = json.loads(mock_print.call_args.args[0])
        self.assertEqual(payload["status"], "stale")
        self.assertEqual(payload["reason"], "header contents changed")
        self.assertIn("Source/Demo/Public/DemoBlueprintLib.h", payload["changed"])


if __name__ == "__main__":
    unittest.main()
