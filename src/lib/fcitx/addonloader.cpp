/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "fcitx/addonloader.h"
#include <exception>
#include <memory>
#include <string>
#include <utility>
#include "fcitx-utils/flags.h"
#include "fcitx-utils/library.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/standardpath.h"
#include "fcitx-utils/stringutils.h"
#include "fcitx/addoninfo.h"
#include "fcitx/addoninstance.h"
#include "addonloader_p.h"
#include "config.h"

namespace fcitx {

AddonLoader::~AddonLoader() {}

SharedLibraryLoader::~SharedLibraryLoader() {}

AddonInstance *SharedLibraryLoader::load(const AddonInfo &info,
                                         AddonManager *manager) {
    auto iter = registry_.find(info.uniqueName());
    if (iter == registry_.end()) {
        std::string libname = info.library();
        Flags<LibraryLoadHint> flag = LibraryLoadHint::DefaultHint;
        if (stringutils::startsWith(libname, "export:")) {
            libname = libname.substr(7);
            flag |= LibraryLoadHint::ExportExternalSymbolsHint;
        }
        auto file = libname + FCITX_LIBRARY_SUFFIX;
        auto libs = standardPath_.locateAll(StandardPath::Type::Addon, file);
        if (libs.empty()) {
            FCITX_ERROR() << "Could not locate library " << file
                          << " for addon " << info.uniqueName() << ".";
        }
        for (const auto &libraryPath : libs) {
            Library lib(libraryPath);
            if (!lib.load(flag)) {
                FCITX_ERROR()
                    << "Failed to load library for addon " << info.uniqueName()
                    << " on " << libraryPath << ". Error: " << lib.error();
                continue;
            }
            try {
                registry_.emplace(info.uniqueName(),
                                  std::make_unique<SharedLibraryFactory>(
                                      info, std::move(lib)));
            } catch (const std::exception &e) {
                FCITX_ERROR() << "Failed to initialize addon factory for addon "
                              << info.uniqueName() << ". Error: " << e.what();
            }
            break;
        }
        iter = registry_.find(info.uniqueName());
    }

    if (iter == registry_.end()) {
        return nullptr;
    }

    try {
        return iter->second->factory()->create(manager);
    } catch (const std::exception &e) {
        FCITX_ERROR() << "Failed to create addon: " << info.uniqueName() << " "
                      << e.what();
    } catch (...) {
        FCITX_ERROR() << "Failed to create addon: " << info.uniqueName();
    }
    return nullptr;
}

StaticLibraryLoader::StaticLibraryLoader(StaticAddonRegistry *registry_)
    : registry(registry_) {}

AddonInstance *StaticLibraryLoader::load(const AddonInfo &info,
                                         AddonManager *manager) {
    auto iter = registry->find(info.uniqueName());
    if (iter == registry->end()) {
        return nullptr;
    }
    try {
        return iter->second->create(manager);
    } catch (const std::exception &e) {
        FCITX_ERROR() << "Failed to create addon: " << info.uniqueName() << " "
                      << e.what();
    } catch (...) {
        FCITX_ERROR() << "Failed to create addon: " << info.uniqueName();
    }
    return nullptr;
}
} // namespace fcitx
