/*
 * SPDX-FileCopyrightText: 2021~2021 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "inputmethodengine.h"

std::string
fcitx::InputMethodEngine::subModeIcon(const fcitx::InputMethodEntry &entry,
                                      fcitx::InputContext &ic) {
    if (auto *this2 = dynamic_cast<InputMethodEngineV2 *>(this)) {
        return this2->subModeIconImpl(entry, ic);
    }
    return overrideIcon(entry);
}
