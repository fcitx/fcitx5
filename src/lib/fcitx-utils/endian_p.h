/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_ENDIAN_P_H_
#define _FCITX_UTILS_ENDIAN_P_H_

#include <cstdint>
#if defined(__linux__) || defined(__GLIBC__) || defined(__EMSCRIPTEN__)
#include <endian.h> // IWYU pragma: export
#elif defined(__APPLE__)

#include <libkern/OSByteOrder.h> // IWYU pragma: export

#define htobe16(x) OSSwapHostToBigInt16(x)
#define htole16(x) OSSwapHostToLittleInt16(x)
#define be16toh(x) OSSwapBigToHostInt16(x)
#define le16toh(x) OSSwapLittleToHostInt16(x)

#define htobe32(x) OSSwapHostToBigInt32(x)
#define htole32(x) OSSwapHostToLittleInt32(x)
#define be32toh(x) OSSwapBigToHostInt32(x)
#define le32toh(x) OSSwapLittleToHostInt32(x)

#define htobe64(x) OSSwapHostToBigInt64(x)
#define htole64(x) OSSwapHostToLittleInt64(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#define le64toh(x) OSSwapLittleToHostInt64(x)

#else
#include <sys/endian.h> // IWYU pragma: export
#endif

enum { BYTE_ORDER_MSB_FIRST = 1, BYTE_ORDER_LSB_FIRST = 0 };
inline char hostByteOrder() {
    const uint16_t endian = 1;
    uint8_t byteOrder = 0;
    if (*reinterpret_cast<const char *>(&endian)) {
        byteOrder = BYTE_ORDER_LSB_FIRST;
    } else {
        byteOrder = BYTE_ORDER_MSB_FIRST;
    }
    return byteOrder;
}

inline bool isLittleEndian() { return hostByteOrder() == BYTE_ORDER_LSB_FIRST; }

#endif // _FCITX_UTILS_ENDIAN_P_H_
