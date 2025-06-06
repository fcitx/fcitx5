/*
 * SPDX-FileCopyrightText: 2016-2025 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include <algorithm>
#include <cassert>
#include <cctype>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <fileapi.h>
#include <initguid.h>
#include <knownfolders.h>
#include <ranges>
#include "fcitx-utils/fcitxutils_export.h"
#include "fcitx-utils/fs.h"
#include "fcitx-utils/standardpaths.h"
#include "fcitx-utils/stringutils.h"
#include "fcitx-utils/utf8.h"
#include "misc_p.h"
#include "shlobj.h"
#include "standardpaths.h"
#include "standardpaths_p.h"

namespace fcitx {

FCITXUTILS_EXPORT HINSTANCE mainInstanceHandle = nullptr;

namespace {

// For windows, we install all files used by fcitx under a sub directory of
// standard path. We will have a 3 layer directory structure:
// AppData/Roaming/Fcitx5/{config,data}/[package]/
// AppData/Local/Fcitx5/{config,data}/[package]/
// appFilePath/../{config,data}/[package]/
constexpr std::string_view windowsTopLevelAppName = "Fcitx5";

void normalizeSlash(std::wstring &path) {
    std::ranges::for_each(path, [](wchar_t &c) {
        if (c == '\\') {
            c = '/';
        }
    });
}

std::filesystem::path appFileName() // get application file name
{
    std::vector<wchar_t> space;
    DWORD v;
    size_t size = 1;
    do {
        size += MAX_PATH;
        space.resize(size);
        auto hInstance = reinterpret_cast<HINSTANCE>(mainInstanceHandle);
        v = GetModuleFileNameW(hInstance, space.data(), DWORD(space.size()));
    } while (v >= size);

    std::wstring_view nativePath(space.data(), v);
    return std::filesystem::path(nativePath);
}

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

std::filesystem::path localAppData(bool isRoaming) {
    static const bool isLow = isProcessLowIntegrity();
    GUID id{};
    if (isRoaming) {
        id = FOLDERID_RoamingAppData;
    } else {
        id = isLow ? FOLDERID_LocalAppDataLow : FOLDERID_LocalAppData;
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
    normalizeSlash(path);

    return path;
}

std::vector<std::filesystem::path> builtinPath(
    const std::unordered_map<std::string, std::vector<std::filesystem::path>>
        &builtInPathMap,
    const char *builtInPathType) {

    if (auto *path = findValue(builtInPathMap, builtInPathType)) {
        return *path;
    }

    static const std::filesystem::path defaultBasePath = []() {
        auto filename = appFileName();
        auto parent = filename.parent_path();
        if (parent.filename() != "bin") {
            throw std::runtime_error(
                "Application file is expected directory: " + filename.string());
        }
        return parent.parent_path();
    }();

    std::filesystem::path basePath = defaultBasePath;
    if (auto *path = findValue(builtInPathMap, "basedir");
        path && !path->empty()) {
        basePath = path->front();
    }

    const std::unordered_map<std::string, std::filesystem::path> pathMap = {
        {"basedir", basePath},
        {"configdir", basePath / "config"},
        {"pkgconfigdir", basePath / "config/fcitx5"},
        {"datadir", basePath / "data"},
        {"pkgdatadir", basePath / "data/fcitx5"},
        {"libdir", basePath / "lib"},
        {"bindir", basePath / "bin"},
        {"localedir", basePath / "data/locale"},
        {"addondir", basePath / "lib/fcitx5"},
        {"libdatadir", basePath / "lib"},
        {"libexecdir", basePath / "libexec"},
    };
    if (auto *path = findValue(pathMap, builtInPathType)) {
        return {*path};
    }
    return {};
}

std::filesystem::path getTempPath() {
    std::wstring ret;
    using GetTempPathPrototype =
        DWORD WINAPI (*)(DWORD nBufferLength, LPWSTR lpBuffer);
    // We try to resolve GetTempPath2 and use that, otherwise fall back to
    // GetTempPath:
    static GetTempPathPrototype getTempPathW = []() {
        const HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
        using FunctionPointer = void (*)();
        if (auto *func =
                FunctionPointer(GetProcAddress(kernel32, "GetTempPath2W")))
            return GetTempPathPrototype(func);
        return GetTempPathW;
    }();

    wchar_t tempPath[MAX_PATH];
    const DWORD len = getTempPathW(MAX_PATH, tempPath);
    if (len) { // GetTempPath() can return short names, expand.
        wchar_t longTempPath[MAX_PATH];
        const DWORD longLen =
            GetLongPathNameW(tempPath, longTempPath, MAX_PATH);
        ret = longLen && longLen < MAX_PATH
                  ? std::wstring(longTempPath, longLen)
                  : std::wstring(tempPath, len);
    }
    if (!ret.empty()) {
        while (ret.ends_with(u'\\'))
            ret.pop_back();
        std::ranges::for_each(ret, [](wchar_t &c) {
            if (c == u'\\') {
                c = u'/';
            }
        });
    }
    if (ret.empty()) {
        ret = L"C:/tmp"; // Fallback to a default temp path.
    } else if (ret.length() >= 2 && ret[1] == u':') {
        ret[0] = std::toupper(ret.at(0)); // Force uppercase drive letters.
    }
    return ret;
}

std::vector<std::filesystem::path> getPath(
    StandardPathsOptions options, StandardPathsType type,
    const std::unordered_map<std::string, std::vector<std::filesystem::path>>
        &builtInPathMap,
    std::string_view packageName = "") {
    std::vector<std::filesystem::path> paths;

    switch (type) {
    case StandardPathsType::PkgConfig:
        assert(!packageName.empty());
        paths.push_back(localAppData(true) / windowsTopLevelAppName / "config" /
                        packageName);
        if (!options.test(StandardPathsOption::SkipSystemPath)) {
            paths.push_back(localAppData(false) / windowsTopLevelAppName /
                            "config" / packageName);
        }
        if (!options.test(StandardPathsOption::SkipBuiltInPath)) {
            if (packageName == "fcitx5") {
                std::ranges::copy(builtinPath(builtInPathMap, "pkgconfigdir"),
                                  std::back_inserter(paths));
            } else if (packageName == "fcitx5-config") {
                std::ranges::copy(
                    builtinPath(builtInPathMap, "configdir") |
                        std::views::transform(
                            [packageName](const std::filesystem::path &path) {
                                return path / packageName;
                            }),
                    std::back_inserter(paths));
            }
        }
        break;
    case StandardPathsType::PkgData:
        assert(!packageName.empty());
        paths.push_back(localAppData(true) / packageName);
        if (!options.test(StandardPathsOption::SkipSystemPath)) {
            paths.push_back(localAppData(false) / packageName);
        }
        if (!options.test(StandardPathsOption::SkipBuiltInPath)) {
            if (packageName == "fcitx5") {
                std::ranges::copy(builtinPath(builtInPathMap, "pkgdatadir"),
                                  std::back_inserter(paths));
            } else {
                std::ranges::copy(
                    builtinPath(builtInPathMap, "datadir") |
                        std::views::transform(
                            [packageName](const std::filesystem::path &path) {
                                return path / packageName;
                            }),
                    std::back_inserter(paths));
            }
        }
        break;
    case StandardPathsType::Config:
        paths.push_back(localAppData(true) / windowsTopLevelAppName / "config");
        if (!options.test(StandardPathsOption::SkipSystemPath)) {
            paths.push_back(localAppData(false) / windowsTopLevelAppName /
                            "config");
        }
        if (!options.test(StandardPathsOption::SkipBuiltInPath)) {
            std::ranges::copy(builtinPath(builtInPathMap, "configdir"),
                              std::back_inserter(paths));
        }
        break;
    case StandardPathsType::Data:
        paths.push_back(localAppData(true) / windowsTopLevelAppName / "data");
        if (!options.test(StandardPathsOption::SkipSystemPath)) {
            paths.push_back(localAppData(false) / windowsTopLevelAppName /
                            "data");
        }
        if (!options.test(StandardPathsOption::SkipBuiltInPath)) {
            std::ranges::copy(builtinPath(builtInPathMap, "datadir"),
                              std::back_inserter(paths));
        }
        break;
    case StandardPathsType::Cache:
        paths.push_back(localAppData(false) / windowsTopLevelAppName / "cache");
        break;
    case StandardPathsType::Addon:
        paths.push_back({});
        if (!options.test(StandardPathsOption::SkipSystemPath)) {
            paths.push_back(localAppData(false) / windowsTopLevelAppName /
                            "lib");
        }
        if (!options.test(StandardPathsOption::SkipBuiltInPath)) {
            std::ranges::copy(builtinPath(builtInPathMap, "addondir"),
                              std::back_inserter(paths));
        }
        break;
    case StandardPathsType::Runtime:
        paths.push_back(getTempPath());
        break;
    }

    std::ranges::for_each(paths, [](std::filesystem::path &path) {
        path = path.lexically_normal().generic_wstring();
    });

    std::unordered_set<std::filesystem::path> seen;
    std::erase_if(paths, [&seen](const std::filesystem::path &path) {
        if (seen.contains(path)) {
            return true;
        }
        seen.insert(path);
        return false;
    });

    return paths;
}

} // namespace

StandardPathsPrivate::StandardPathsPrivate(
    const std::string &packageName,
    const std::unordered_map<std::string, std::vector<std::filesystem::path>>
        &builtInPathMap,
    StandardPathsOptions options)
    : options_(options) {
    // initialize user directory
    configDirs_ = getPath(options_, StandardPathsType::Config, builtInPathMap);
    pkgconfigDirs_ = getPath(options_, StandardPathsType::PkgConfig,
                             builtInPathMap, packageName);
    dataDirs_ =
        getPath(options_, StandardPathsType::Data, builtInPathMap, packageName);
    pkgdataDirs_ = getPath(options_, StandardPathsType::PkgData, builtInPathMap,
                           packageName);
    runtimeDir_ = getPath(options_, StandardPathsType::Runtime, builtInPathMap);
    addonDirs_ = getPath(options_, StandardPathsType::Addon, builtInPathMap);

    syncUmask();
}

std::filesystem::path
StandardPathsPrivate::fcitxPath(const char *path,
                                const std::filesystem::path &subPath) {
    if (!path) {
        return {};
    }

    auto fsPath = builtinPath({}, path);
    if (fsPath.empty() || fsPath.front().empty()) {
        return {};
    }
    return fsPath.front() / subPath;
}

} // namespace fcitx
