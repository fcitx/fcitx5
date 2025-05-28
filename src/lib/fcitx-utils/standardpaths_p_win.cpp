/*
 * SPDX-FileCopyrightText: 2016-2025 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include <algorithm>
#include <filesystem>
#include <string>
#include <initguid.h>
#include <knownfolders.h>
#include "fcitx-utils/fs.h"
#include "fcitx-utils/standardpaths.h"
#include "fcitx-utils/stringutils.h"
#include "fcitx-utils/utf8.h"
#include "shlobj.h"
#include "standardpaths.h"
#include "standardpaths_p.h"

namespace fcitx {

namespace {

std::wstring removeUncOrLongPathPrefix(std::wstring path) {
    constexpr size_t minPrefixSize = 4;
    if (path.size() < minPrefixSize) {
        return path;
    }

    auto data = path.data();
    const auto slash = path[0];
    if (slash != '\\' && slash != '/') {
        return path;
    }

    // check for "//?/" or "/??/"
    if (data[2] == u'?' && data[3] == slash &&
        (data[1] == slash || data[1] == u'?')) {
        path = path.substr(minPrefixSize);

        // check for a possible "UNC/" prefix left-over
        if (path.size() >= 4) {
            data = path.data();
            if (data[0] == 'U' && data[1] == 'N' && data[2] == 'C' &&
                data[3] == slash) {
                data[2] = slash;
                return path.substr(2);
            }
        }
    }

    return path;
}

static bool isProcessLowIntegrity() {
    const auto processToken = GetCurrentProcessToken();

    std::vector<uint8_t> tokenInfoBuffer(256);
    auto *tokenInfo =
        reinterpret_cast<TOKEN_MANDATORY_LABEL *>(tokenInfoBuffer.data());
    DWORD length = tokenInfoBuffer.size();
    if (!GetTokenInformation(processToken, TokenIntegrityLevel, tokenInfo,
                             length, &length)) {
        // grow buffer and retry GetTokenInformation
        tokenInfoBuffer.resize(length);
        tokenInfo =
            reinterpret_cast<TOKEN_MANDATORY_LABEL *>(tokenInfoBuffer.data());
        if (!GetTokenInformation(processToken, TokenIntegrityLevel, tokenInfo,
                                 length, &length))
            return false; // assume "normal" process
    }

    // The GetSidSubAuthorityCount return-code is undefined on failure, so
    // there's no point in checking before dereferencing
    DWORD level =
        *GetSidSubAuthority(tokenInfo->Label.Sid,
                            *GetSidSubAuthorityCount(tokenInfo->Label.Sid) - 1);
    return (level < SECURITY_MANDATORY_MEDIUM_RID);
}

std::filesystem::path userPath(StandardPathsType type) {
    bool isPackage = false;
    GUID id{};
    switch (type) {
    case StandardPathsType::PkgConfig:
        isPackage = true;
        [[fallthrough]];
    case StandardPathsType::Config:
        id = FOLDERID_LocalAppData;
        break;
    case StandardPathsType::PkgData:
        isPackage = true;
        [[fallthrough]];
    case StandardPathsType::Data:
        id = FOLDERID_LocalAppData;
        break;
    case StandardPathsType::Cache:
        break;
    case StandardPathsType::Runtime:
        break;
    case StandardPathsType::Addon:
        break;
    }

    std::wstring path;
    LPWSTR wpath;
    if (SUCCEEDED(SHGetKnownFolderPath(id, KF_FLAG_DONT_VERIFY, 0, &wpath))) {
        path = wpath;
        CoTaskMemFree(wpath);
    }

    if (path.empty()) {
        return path;
    }

    path = removeUncOrLongPathPrefix(path);

    std::ranges::for_each(path, [](wchar_t &c) {
        if (c == '\\') {
            c = '/';
        }
    });

    return path;
}

} // namespace

StandardPathsPrivate::StandardPathsPrivate(
    const std::string &packageName,
    const std::unordered_map<std::string, std::filesystem::path>
        &builtInPathMap,
    bool skipBuiltInPath, bool skipUserPath)
    : skipBuiltInPath_(skipBuiltInPath), skipUserPath_(skipUserPath) {
    // initialize user directory
    configDirs_ = {userPath(StandardPathsType::Config)};
    pkgconfigDirs_ = {userPath(StandardPathsType::PkgConfig)};

    dataHome_ = defaultPath("XDG_DATA_HOME", ".local/share");
    pkgdataHome_ = defaultPath((isFcitx ? "FCITX_DATA_HOME" : nullptr),
                               constructPath(dataHome_, packageName).c_str());
    dataDirs_ =
        defaultPaths("XDG_DATA_DIRS", "/usr/local/share:/usr/share",
                     builtInPathMap, skipBuiltInPath_ ? nullptr : "datadir");
    auto pkgdataDirFallback = dataDirs_;
    for (auto &path : pkgdataDirFallback) {
        path = constructPath(path, packageName);
    }
    pkgdataDirs_ =
        defaultPaths((isFcitx ? "FCITX_DATA_DIRS" : nullptr),
                     stringutils::join(pkgdataDirFallback, ":").c_str(),
                     builtInPathMap, skipBuiltInPath_ ? nullptr : "pkgdatadir");
    cacheHome_ = defaultPath("XDG_CACHE_HOME", ".cache");
    auto tmpdir = getEnvironment("TMPDIR");
    runtimeDir_ =
        defaultPath("XDG_RUNTIME_DIR",
                    (!tmpdir || tmpdir->empty()) ? "/tmp" : tmpdir->data());
    addonDirs_ =
        defaultPaths("FCITX_ADDON_DIRS", StandardPath::fcitxPath("addondir"),
                     builtInPathMap, nullptr);

    syncUmask();
}

const char *StandardPath::fcitxPath(const char *path) {
    if (!path) {
        return nullptr;
    }

    const static std::string basePath = []() {
        std::vector<wchar_t> space;
        DWORD v;
        size_t size = MAX_PATH + 1;
        do {
            space.resize(size);
            size += MAX_PATH;
            v = GetModuleFileNameW(nullptr, space.data(), DWORD(space.size()));
        } while (v >= size);

        auto exePath = utf8::UTF16ToUTF8(std::wstring_view(space.data(), v));
        std::string basePath = fs::cleanPath(stringutils::join(exePath, ".."));
        return basePath;
    }();

    static const std::unordered_map<std::string, std::string> pathMap = {
        std::make_pair<std::string, std::string>(
            "datadir", stringutils::joinPath(basePath, "share")),
        std::make_pair<std::string, std::string>(
            "pkgdatadir", stringutils::joinPath(basePath, "share/fcitx5")),
        std::make_pair<std::string, std::string>(
            "libdir", stringutils::joinPath(basePath, "lib")),
        std::make_pair<std::string, std::string>(
            "bindir", stringutils::joinPath(basePath, "bin")),
        std::make_pair<std::string, std::string>(
            "localedir", stringutils::joinPath(basePath, "share/locale")),
        std::make_pair<std::string, std::string>(
            "addondir", stringutils::joinPath(basePath, "lib/fcitx5")),
        std::make_pair<std::string, std::string>(
            "libdatadir", stringutils::joinPath(basePath, "lib")),
        std::make_pair<std::string, std::string>(
            "libexecdir", stringutils::joinPath(basePath, "lib")),
    };

    auto iter = pathMap.find(path);
    if (iter != pathMap.end()) {
        return iter->second.c_str();
    }

    return nullptr;
}

std::string StandardPathPrivate::defaultPath(const char *env,
                                             const char *defaultPath) {
    std::optional<std::string> cdir;
    if (env) {
        cdir = getEnvironment(env);
    }
    std::string dir;
    if (cdir && !cdir->empty()) {
        dir = *cdir;
    } else {
        // caller need to ensure HOME is not empty;
        if (defaultPath[0] != '/') {
            auto home = getEnvironment("HOME");
            if (!home) {
                throw std::runtime_error("Home is not set");
            }
            dir = stringutils::joinPath(*home, defaultPath);
        } else {
            if (env && strcmp(env, "XDG_RUNTIME_DIR") == 0) {
                return {};
#if 0
                dir = stringutils::joinPath(
                    defaultPath,
                    stringutils::concat("fcitx-runtime-", geteuid()));
                if (!fs::isdir(dir)) {
                    if (mkdir(dir.c_str(), 0700) != 0) {
                        return {};
                    }
                }
#endif
            } else {
                dir = defaultPath;
            }
        }
    }

    if (!dir.empty() && env && strcmp(env, "XDG_RUNTIME_DIR") == 0) {
#ifndef _WIN32
        struct stat buf;
        if (stat(dir.c_str(), &buf) != 0 || buf.st_uid != geteuid() ||
            (buf.st_mode & 0777) != S_IRWXU) {
            return {};
        }
#endif
    }
    return dir;
}

} // namespace fcitx
