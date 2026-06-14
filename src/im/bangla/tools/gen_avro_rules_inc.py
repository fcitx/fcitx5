#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Generate avro_rules.inc from avro-phonetic.json for the native C++ parser."""

from __future__ import annotations

import json
import sys
from pathlib import Path


def esc(s: str) -> str:
    return s.replace("\\", "\\\\").replace('"', '\\"')


def main() -> int:
    plugin_root = Path(__file__).resolve().parents[1]
    repo_root = plugin_root.parents[1]
    json_path = repo_root.parent / "fcitx5" / "data" / "avro" / "avro-phonetic.json"
    if len(sys.argv) > 1:
        json_path = Path(sys.argv[1])
    out_path = plugin_root / "src" / "main" / "cpp" / "avro_rules.inc"

    if not json_path.is_file():
        print(f"Rules JSON not found: {json_path}", file=sys.stderr)
        return 1

    data = json.loads(json_path.read_text(encoding="utf-8"))
    lines: list[str] = []
    lines.append("std::vector<AvroPattern> makePatterns() {")
    lines.append("    return {")
    for pattern in data["patterns"]:
        lines.append(
            f'        AvroPattern{{"{esc(pattern["find"])}", "{esc(pattern["replace"])}", {{'
        )
        for rule in pattern.get("rules", []):
            lines.append("            AvroRule{{")
            for match in rule.get("matches", []):
                scope = match.get("scope", "")
                negative = bool(match.get("negative", False))
                if scope.startswith("!"):
                    negative = True
                    scope = scope[1:]
                neg = "true" if negative else "false"
                lines.append(
                    f'                AvroMatchRule{{"{esc(match.get("type", ""))}", '
                    f'"{esc(scope)}", "{esc(match.get("value", ""))}", {neg}}},'
                )
            lines.append(f'            }}, "{esc(rule.get("replace", ""))}"}},')
        lines.append("        }},")
    lines.append("    };")
    lines.append("}")
    lines.append(f'std::string makeVowels() {{ return "{esc(data["vowel"])}"; }}')
    lines.append(f'std::string makeConsonants() {{ return "{esc(data["consonant"])}"; }}')
    lines.append(
        f'std::string makeCaseSensitive() {{ return "{esc(data["casesensitive"])}"; }}'
    )

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"Wrote {out_path} ({len(lines)} lines)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
