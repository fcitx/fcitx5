/*
 * SPDX-FileCopyrightText: 2021~2021 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#define TEST_DISABLE_LIBUUID

#include <cstdint>
#include <cstring>
#include "fcitx-utils/log.h"
#include "fcitx-utils/uuid_p.h"
#include "testdir.h"

int main() {
    uint8_t t[16];
    uint8_t t2[16];
    uint8_t t3[16];
    FCITX_ASSERT(!fcitx::parseUUID("ff4d1624ze568-4f86-9def-302c73959c1d", t));
    FCITX_ASSERT(!fcitx::parseUUID("ff4d1624-m568-4f86-9def-302c73959c1d", t));
    FCITX_ASSERT(fcitx::parseUUID("ff4d1624-e568-4f86-9def-302c73959c1d", t));
    uint8_t expected[16] = {0xff, 0x4d, 0x16, 0x24, 0xe5, 0x68, 0x4f, 0x86,
                            0x9d, 0xef, 0x30, 0x2c, 0x73, 0x95, 0x9c, 0x1d};
    FCITX_ASSERT(memcmp(t, expected, 16) == 0);
    fcitx::generateUUID(t);

    uint8_t expected2[16] = {0x42, 0xf9, 0xa3, 0x3a, 0xb4, 0xb8, 0x43, 0xca,
                             0xae, 0x70, 0x6a, 0xbb, 0x89, 0xd9, 0x34, 0x0c};
    fcitx::generateUUIDFallback(FCITX5_SOURCE_DIR "/test/uuid", t);
    FCITX_ASSERT(memcmp(t, expected2, 16) == 0);

    fcitx::generateUUIDFallback(FCITX5_SOURCE_DIR "/test/invalid_uuid", t);
    fcitx::generateUUIDFallback(FCITX5_SOURCE_DIR "/test/invalid_uuid", t2);
    fcitx::generateUUIDFallback(nullptr, t3);
    FCITX_ASSERT(memcmp(t, t2, 16) != 0);
    FCITX_ASSERT(memcmp(t, t3, 16) != 0);
    FCITX_ASSERT(memcmp(t2, t3, 16) != 0);
    return 0;
}
