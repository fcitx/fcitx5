#!/usr/bin/env python3

import fileinput

license = """/*
 * SPDX-FileCopyrightText: 2026 fcitx5 contributors
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

"""


def header(content, guard_var):
    return license + header_guard(content, guard_var)


def header_guard(content, guard_var):
    return """#ifndef {0}
#define {0}

{1}

#endif // {0}
""".format(guard_var, content)


EXCLUDE_RANGES = [
    (0x10A0, 0x10FF),  # Georgian Mkhedruli/Asomtavruli
    (0x1C90, 0x1CBF),  # Georgian Mtavruli
    (0x13A0, 0x13FF),  # Cherokee
    (0xAB70, 0xABBF),  # Cherokee Lower
]

EXTRA_PAIRS = [
    (0x03C2, 0x03A3),  # GREEK SMALL LETTER FINAL SIGMA, fails round-trip below
]


def excluded(cp):
    return any(lo <= cp <= hi for lo, hi in EXCLUDE_RANGES)


up = {}
lo = {}
for line in fileinput.input("UnicodeData.txt"):
    fields = line.split(";")
    cp = int(fields[0], 16)
    upper_field = fields[12].strip()
    lower_field = fields[13].strip()
    if upper_field:
        up[cp] = int(upper_field, 16)
    if lower_field:
        lo[cp] = int(lower_field, 16)

pairs = sorted(
    set(
        (l, u)
        for l, u in up.items()
        if lo.get(u) == l and not excluded(l) and not excluded(u)
    )
    | set(EXTRA_PAIRS)
)

table = "static const CasePair case_pair_tab[] = {\n"
for l, u in pairs:
    table += "    {{0x{:04x}, 0x{:04x}}},\n".format(l, u)
table += "};\n"

content = """#include <array>
#include <cstdint>
#include <fcitx-utils/macros.h>

// IWYU pragma: private, include "fcitx-utils/key.h"

namespace fcitx {{
struct CasePair {{
    uint32_t lower;
    uint32_t upper;
}};

{0}
using CasePairTab = std::array<CasePair, FCITX_ARRAY_SIZE(case_pair_tab)>;
const CasePairTab &case_pair_tab_by_lower();
const CasePairTab &case_pair_tab_by_upper();

}} // namespace fcitx""".format(table)

print(header(content, "_FCITX_UTILS_CASEDATA_H_"))
