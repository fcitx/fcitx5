#!/usr/bin/env python3

import fileinput

license = """/*
 * Copyright (C) 2015~2015 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; see the file COPYING. If not,
 * see <http://www.gnu.org/licenses/>.
 */

"""

def header(content, guard_var):
    return license + header_guard(content, guard_var);

def header_guard(content, guard_var):
    return """

#ifndef {0}
#define {0}

{1}

#endif""".format(guard_var, content)

keysymdef = ""
keynametable = ""
data=[]

data.append(("None", "0x0", ""))
nameList=[]
valueList=[]
valueToOffset=dict()

for line in fileinput.input("keylist"):
    l = line.split('\t')
    if len(l) != 3:
        continue
    data.append((l[0], l[1].lower(), l[2].strip()))

for (i, (name, value, comment)) in enumerate(data):
    keysymdef += ("FcitxKey_{0} = {1}, {2}\n".format(name, value, comment))

for (i, (name, value, comment)) in enumerate(sorted(data, key=lambda n: n[0] )):
    nameList.append(name)
    valueList.append(value)
    if value not in valueToOffset:
        valueToOffset[value] = i

keysymdef = """
#include <fcitx-utils/macros.h>

FCITX_C_DECL_BEGIN

typedef enum _FcitxKeySym
{{
{0}
}} FcitxKeySym;

FCITX_C_DECL_END
""".format(keysymdef)
f = open("keysymgen.h", "w")
f.write(header(keysymdef, "_FCITX_UTILS_KEYSYMGEN_H_"))
f.close()

keynametable = """
#include <fcitx-utils/macros.h>

FCITX_C_DECL_BEGIN

static const char *keyNameList[] _FCITX_UNUSED_ =
{{
{0}
}};

static const uint32_t keyValueByNameOffset[] _FCITX_UNUSED_ =
{{
{1}
}};

static const struct KeyNameOffsetByValue {{
    uint32_t sym;
    uint16_t offset;
}} keyNameOffsetByValue[] _FCITX_UNUSED_ = {{
{2}
}};

FCITX_C_DECL_END
""".format("\n".join(['"{0}",'.format(s) for s in nameList]),
           ",\n".join(valueList),
           "\n".join('{{{0}, {1}}},'.format(s[0], s[1]) for s in sorted(valueToOffset.items(), key=lambda n: int(n[0], 16))))

f = open("keynametable.h" ,"w")
f.write(header(keynametable, "_FCITX_UTILS_KEYNAMETABLE_H_"))
f.close()
