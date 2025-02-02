/*
 * SPDX-FileCopyrightText: 2025~2025 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "environ.h"
#include <optional>
#include <string>
#include <string_view>
#include "fcitx-utils/utf8.h"
#include "stringutils.h"
#include "utf8.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace fcitx {

void setEnvironment(const char *variable, const char *value) {
#ifdef _WIN32
    if (!variable || !value) {
        return;
    }

    auto wname = utf8::UTF8ToUTF16(variable);
    auto wvalue = utf8::UTF8ToUTF16(value);

    auto assign = stringutils::concat(variable, "=", value);
    auto wassign = utf8::UTF8ToUTF16(assign);

    _wputenv(wassign.data());

    SetEnvironmentVariableW(wname.data(), wvalue.data());
#else
    setenv(variable, value, 1);
#endif
}

std::optional<std::string> getEnvironment(const char *variable) {
#ifdef _WIN32
    if (!variable) {
        return std::nullopt;
    }
    auto wname = utf8::UTF8ToUTF16(variable);
    DWORD len = GetEnvironmentVariableW(wname.data(), nullptr, 0);

    if (GetLastError() == ERROR_ENVVAR_NOT_FOUND) {
        return std::nullopt;
    }
    std::vector<wchar_t> wdata(len);
    if (GetEnvironmentVariableW(wname.data(), wdata.data(), len) != len - 1) {
        return std::nullopt;
    }
    std::wstring wvalue(wdata.data());

    if (wvalue.find(L"%") != std::wstring::npos) {
        len = ExpandEnvironmentStringsW(wvalue.data(), nullptr, 0);

        if (len > 0) {
            wdata.resize(len);
            if (ExpandEnvironmentStringsW(wvalue.data(), wdata.data(), len) ==
                len) {
                wvalue = wdata.data();
            }
        }
    }

    return utf8::UTF16ToUTF8(wvalue);
#else
    const char *value = getenv(variable);
    if (value) {
        return std::string(value);
    }
    return std::nullopt;
#endif
}

} // namespace fcitx
