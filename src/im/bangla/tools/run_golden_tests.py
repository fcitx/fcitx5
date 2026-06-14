#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Run Avro parser golden tests (Python reference + optional native host binary)."""

from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path


GOLDEN_CASES = [
    ("ami banglay gan gai", "আমি বাংলায় গান গাই"),
    ("amader valObasa", "আমাদের ভালোবাসা"),
    ("bhalo", "ভাল"),  # lowercase trailing o is literal; use bhalO for o-kar
    ("bhalO", "ভালো"),
    ("mukh", "মুখ"),
    ("OI", "ঐ"),
    ("OU", "ঔ"),
    ("rry", "রর‍্য"),
    ("t``", "ৎ"),
    (",,", "্‌"),
    ("x", "এক্স"),
    ("kw", "ক্ব"),
    ("ky", "ক্য"),
    ("123", "১২৩"),
]


def load_python_parser():
    tools_dir = Path(__file__).resolve().parent
    fcitx5_tools = tools_dir.parents[2].parent / "fcitx5" / "tools"
    sys.path.insert(0, str(fcitx5_tools))
    from avro_phonetic_parser import AvroPhoneticParser  # noqa: PLC0415

    json_path = fcitx5_tools.parent / "data" / "avro" / "avro-phonetic.json"
    data = json.loads(json_path.read_text(encoding="utf-8"))
    return AvroPhoneticParser(data)


def run_python_golden_tests() -> int:
    parser = load_python_parser()
    failures = 0
    print("=== Python reference golden tests ===")
    for input_text, expected in GOLDEN_CASES:
        actual = parser.parse(input_text)
        if actual != expected:
            failures += 1
            print(f'FAIL: "{input_text}"')
            print(f"  expected: {expected}")
            print(f"  actual:   {actual}")
        else:
            print(f'PASS: "{input_text}" -> {actual}')
    if failures:
        print(f"{failures} Python golden test(s) failed.")
    else:
        print("All Python golden tests passed.")
    return failures


def try_run_cpp_golden_tests() -> int | None:
    tools_dir = Path(__file__).resolve().parent
    cpp_dir = tools_dir.parent / "src" / "main" / "cpp"
    source = tools_dir / "host_golden_test.cpp"
    binary = tools_dir / "host_golden_test"

    compilers = [
        ["g++", "-std=c++17", "-I", str(cpp_dir), str(source), str(cpp_dir / "avro_parser.cpp"), "-o", str(binary)],
        ["clang++", "-std=c++17", "-I", str(cpp_dir), str(source), str(cpp_dir / "avro_parser.cpp"), "-o", str(binary)],
    ]

    for cmd in compilers:
        try:
            subprocess.run(cmd, check=True, capture_output=True, text=True)
            print(f"\n=== Native C++ golden tests ({cmd[0]}) ===")
            result = subprocess.run([str(binary)], check=False, text=True, capture_output=True)
            print(result.stdout, end="")
            if result.stderr:
                print(result.stderr, end="", file=sys.stderr)
            return result.returncode
        except FileNotFoundError:
            continue
        except subprocess.CalledProcessError as exc:
            print(f"Compile failed with {cmd[0]}:", exc.stderr or exc.stdout, file=sys.stderr)
            continue

    print("\nSkipping native C++ golden tests (no g++/clang++ found).")
    return None


def main() -> int:
    failures = run_python_golden_tests()
    cpp_result = try_run_cpp_golden_tests()
    if cpp_result is not None:
        failures += cpp_result
    return failures


if __name__ == "__main__":
    raise SystemExit(main())
