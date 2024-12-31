/*
 * SPDX-FileCopyrightText: 2020-2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "fcitx-utils/log.h"
#include "config.h"
#include "im/keyboard/isocodes.h"

using namespace fcitx;

int main() {
    IsoCodes isocodes;
    isocodes.read(ISOCODES_ISO639_JSON, ISOCODES_ISO3166_JSON);
    const auto *entry = isocodes.entry("eng");
    FCITX_ASSERT(entry);
    FCITX_ASSERT(entry->iso_639_1_code == "en");
    return 0;
}
