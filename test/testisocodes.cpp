//
// Copyright (C) 2020~2020 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//
#include "config.h"
#include "im/keyboard/isocodes.h"

using namespace fcitx;

int main() {
    IsoCodes isocodes;
    isocodes.read(ISOCODES_ISO639_JSON, ISOCODES_ISO3166_JSON);
    auto entry = isocodes.entry("eng");
    FCITX_ASSERT(entry);
    FCITX_ASSERT(entry->iso_639_1_code == "en");
    return 0;
}
