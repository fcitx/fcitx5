/*
 * SPDX-FileCopyrightText: 2021~2021 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_UUID_P_H_
#define _FCITX_UTILS_UUID_P_H_

#include "config.h"

#include <fcntl.h>
#include <unistd.h>
#include <cstdint>
#include <cstring>
#include "charutils.h"
#include "eventloopinterface.h"
#include "fs.h"
#include "unixfd.h"

#ifdef TEST_DISABLE_LIBUUID
#undef ENABLE_LIBUUID
#endif

#ifdef ENABLE_LIBUUID
#include <uuid.h>
#endif

namespace fcitx {

static inline bool parseUUID(const char *str, uint8_t *data) {
    auto validate = [](const char *str) {
        for (int i = 0; i < 36; i++) {
            if ((i == 8) || (i == 13) || (i == 18) || (i == 23)) {
                if (str[i] == '-') {
                    continue;
                }
                return false;
            }
            if (!charutils::isxdigit(str[i])) {
                return false;
            }
        }
        return true;
    };

    if (!validate(str)) {
        return false;
    }
    constexpr uint8_t indices[16] = {0,  2,  4,  6,  9,  11, 14, 16,
                                     19, 21, 24, 26, 28, 30, 32, 34};
    for (auto idx : indices) {
        int hi = charutils::fromHex(str[idx]);
        int lo = charutils::fromHex(str[idx + 1]);

        *data = (hi << 4) | lo;
        data += 1;
    }
    return true;
}

static inline void generateUUIDFallback(const char *file, uint8_t *data) {
    memset(data, 0, 16);
    UnixFD fd;
    if (file) {
        fd = UnixFD::own(open(file, O_RDONLY));
    }

    constexpr ssize_t uuidStrLength = 36;
    char uuidStr[uuidStrLength];

    if (fd.isValid() &&
        uuidStrLength == fs::safeRead(fd.fd(), uuidStr, uuidStrLength) &&
        parseUUID(uuidStr, data)) {
        return;
    }
    thread_local uint8_t uuid[16];
    thread_local bool init = false;
    if (!init) {
        uint64_t t = now(CLOCK_MONOTONIC);
        int i = 15;
        while (t) {
            uuid[i--] = t & 0xff;
            t >>= 8;
        }
        init = true;
    }
    auto increaseInternalId = [](uint8_t *data) {
        for (int i = 15; i >= 0; i--) {
            data[i] += 1;
            if (data[i] != 0) {
                break;
            }
        }
    };
    increaseInternalId(uuid);
    memcpy(data, uuid, 16);
}

// The fallback implementation would use a dummy self increament id, and it
// doesn't follow uuid spec, don't treat the value seriously.
static inline void generateUUID(uint8_t *data) {
#ifdef ENABLE_LIBUUID
    static_assert(sizeof(uuid_t) == 16, "uuid size mismatch");
    uuid_generate(data);
#else

#ifdef __linux__
    generateUUIDFallback("/proc/sys/kernel/random/uuid", data);
#else
    generateUUIDFallback(nullptr, data);
#endif

#endif
}

} // namespace fcitx

#endif // _FCITX_UTILS_UUID_P_H_
