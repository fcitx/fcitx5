/*
 * SPDX-FileCopyrightText: 2025~2025 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_SHIM_WIN32_H_
#define _FCITX_UTILS_SHIM_WIN32_H_

#include <optional>
#include <string>
#include <fcitx-utils/fcitxutils_export.h>

namespace fcitx {

/**
 * Set environment variable
 *
 * @param variable variable name
 * @return value
 *
 * @since 5.1.13
 */
FCITXUTILS_EXPORT void setEnvironment(const char *variable, const char *value);

/**
 * Get environment variable value.
 *
 * @param variable variable name
 * @return value
 *
 * @since 5.1.13
 */
FCITXUTILS_EXPORT std::optional<std::string>
getEnvironment(const char *variable);

FCITXUTILS_EXPORT std::string getEnvironmentOrEmpty(const char *variable);

} // namespace fcitx

#endif
