"""
Standalone validation script for the three source-file MCP tools.

Run from the project root with:
    python SmellyCat/Plugins/GenerativeAISupport/Content/Python/test_file_tools.py

The script imports the helper functions from mcp_server directly (no MCP server
process needed) and runs all acceptance cases defined in the plan:
  1. Successful read
  2. Successful write (create + overwrite)
  3. Successful patch
  4. Read-back consistency after write
  5. Path traversal rejected
  6. Disallowed extension rejected
  7. patch with non-unique old_text rejected
  8. patch with missing old_text rejected
"""

import sys
import os
import tempfile
import pathlib

# Make sure we can import from this directory
_here = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(_here))

from mcp_server import _get_project_root, _validate_source_path, \
    read_source_file, write_source_file, patch_source_file

PASS = "\u2705 PASS"
FAIL = "\u274c FAIL"


def check(label: str, cond: bool, detail: str = "") -> None:
    status = PASS if cond else FAIL
    print(f"{status}  {label}")
    if not cond:
        print(f"       detail: {detail}")


# ------------------------------------------------------------------ setup
root = _get_project_root()
print(f"Project root detected: {root}\n")

# Use a scratch file inside Source/ for write/patch tests
SCRATCH_REL = "Source/_mcp_test_scratch.h"
scratch_abs = root / SCRATCH_REL

# ------------------------------------------------------------------ case 1: project root detection
check("Project root has a .uproject", list(root.glob("*.uproject")) != [])

# ------------------------------------------------------------------ case 2: read an existing file
EXISTING = "Source/AiNpcCore/Private/AiNpcClient.cpp"
result = read_source_file(EXISTING, 1, 5)
check(
    f"read_source_file (first 5 lines of {EXISTING})",
    result.startswith("//") and "lines 1-5" in result,
    result[:120],
)

# ------------------------------------------------------------------ case 3: write (create)
initial_content = "#pragma once\n// MCP test file\n"
out = write_source_file(SCRATCH_REL, initial_content)
check("write_source_file creates new file", out.startswith("OK:") and "created" in out, out)
check("file actually exists on disk", scratch_abs.exists())

# ------------------------------------------------------------------ case 4: read-back consistency
read_back = read_source_file(SCRATCH_REL)
check(
    "read_back matches written content",
    initial_content in read_back,
    read_back,
)

# ------------------------------------------------------------------ case 5: write (overwrite)
new_content = "#pragma once\n// MCP test file v2\nstruct Empty {};\n"
out = write_source_file(SCRATCH_REL, new_content)
check("write_source_file overwrites", out.startswith("OK:") and "updated" in out, out)

# ------------------------------------------------------------------ case 6: patch
out = patch_source_file(SCRATCH_REL, "struct Empty {};", "struct Empty { int x; };")
check("patch_source_file replaces unique text", out.startswith("OK:"), out)
patched_content = read_source_file(SCRATCH_REL)
check("patch read-back contains new text", "int x;" in patched_content, patched_content)

# ------------------------------------------------------------------ case 7: patch missing
out = patch_source_file(SCRATCH_REL, "THIS_DOES_NOT_EXIST", "anything")
check("patch_source_file rejects missing old_text", out.startswith("Error:") and "not found" in out, out)

# ------------------------------------------------------------------ case 8: patch non-unique
dup_content = "// line\n// line\n"
write_source_file(SCRATCH_REL, dup_content)
out = patch_source_file(SCRATCH_REL, "// line", "// replaced")
check("patch_source_file rejects non-unique old_text", out.startswith("Error:") and "times" in out, out)

# ------------------------------------------------------------------ case 9: path traversal
out = read_source_file("../../secret.txt")
check("path traversal rejected", out.startswith("Error:") and "traversal" in out.lower(), out)

# ------------------------------------------------------------------ case 10: disallowed extension
out = read_source_file("Source/SomeFile.exe")
check("disallowed extension rejected", out.startswith("Error:") and "not in the allowed list" in out, out)

# ------------------------------------------------------------------ case 11: validate inside Source/
try:
    p = _validate_source_path("Source/AiNpcCore/Public/AiNpcClient.h")
    check("_validate_source_path succeeds for valid path", str(root) in str(p))
except ValueError as e:
    check("_validate_source_path succeeds for valid path", False, str(e))

# ------------------------------------------------------------------ cleanup
if scratch_abs.exists():
    scratch_abs.unlink()
print("\nScratch file cleaned up.")
