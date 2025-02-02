/*
 * SPDX-FileCopyrightText: 2025~2025 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include <optional>
#include "fcitx-utils/environ.h"
#include "fcitx-utils/log.h"

int main() {
    fcitx::setEnvironment("TEST_VAR", "TEST_VALUE");
    FCITX_ASSERT(fcitx::getEnvironment("TEST_VAR") == "TEST_VALUE");
    FCITX_ASSERT(fcitx::getEnvironment("__BAD_VAR") == std::nullopt);

    return 0;
}
