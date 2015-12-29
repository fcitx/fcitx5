#!/bin/sh
find . \( -not \( -name 'keynametable.h' -o -name 'keysymgen.h' -o -name 'keysymdef.h' -o -name 'XF86keysym.h' \) \) -a \( -name '*.h' -o -name '*.cpp' \)  | xargs clang-format -i
