/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "fcitx-utils/flags.h"
#include "fcitx-utils/library.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/standardpaths.h"
#include "fcitx-utils/stringutils.h"
#include "addonfactory.h"
#include "addoninfo.h"
#include "config.h"
#include "sharedlibraryloader_p.h"

namespace fcitx {

namespace {
constexpr char FCITX_ADDON_FACTORY_ENTRY[] = "fcitx_addon_factory_instance";
}

class SharedLibraryFactory {
public:
    SharedLibraryFactory(const AddonInfo &info, std::vector<Library> libraries)
        : libraries_(std::move(libraries)) {
        std::string v2Name = stringutils::concat(FCITX_ADDON_FACTORY_ENTRY, "_",
                                                 info.uniqueName());
        if (libraries_.empty()) {
            throw std::runtime_error("Got empty libraries.");
        }

        // Only resolve with last library.
        auto &library = libraries_.back();
        auto *funcPtr = library.resolve(v2Name.data());
        if (!funcPtr) {
            funcPtr = library.resolve(FCITX_ADDON_FACTORY_ENTRY);
        }
        if (!funcPtr) {
            throw std::runtime_error(library.error());
        }
        auto func = Library::toFunction<AddonFactory *()>(funcPtr);
        factory_ = func();
        if (!factory_) {
            throw std::runtime_error("Failed to get a factory");
        }
    }

    AddonFactory *factory() { return factory_; }

private:
    std::vector<Library> libraries_;
    AddonFactory *factory_;
};

SharedLibraryLoader::SharedLibraryLoader() = default;

SharedLibraryLoader::~SharedLibraryLoader() {}

AddonInstance *SharedLibraryLoader::load(const AddonInfo &info,
                                         AddonManager *manager) {
    auto iter = registry_.find(info.uniqueName());
    if (iter == registry_.end()) {
        std::vector<std::string> libnames =
            stringutils::split(info.library(), ";");

        if (libnames.empty()) {
            FCITX_ERROR() << "Failed to parse Library field: " << info.library()
                          << " for addon " << info.uniqueName();
            return nullptr;
        }

        std::vector<Library> libraries;
        for (std::string_view libname : libnames) {
            Flags<LibraryLoadHint> flag = LibraryLoadHint::DefaultHint;
            if (stringutils::consumePrefix(libname, "export:")) {
                flag |= LibraryLoadHint::ExportExternalSymbolsHint;
            }
            const auto file =
                stringutils::concat(libname, FCITX_LIBRARY_SUFFIX);
            const auto libraryPaths = StandardPaths::global().locateAll(
                StandardPathsType::Addon, file);
            if (libraryPaths.empty()) {
                FCITX_ERROR() << "Could not locate library " << file
                              << " for addon " << info.uniqueName() << ".";
            }
            bool loaded = false;
            for (const auto &libraryPath : libraryPaths) {
                Library library(libraryPath);
                if (library.load(flag)) {
                    libraries.push_back(std::move(library));
                    loaded = true;
                    break;
                }
                FCITX_ERROR()
                    << "Failed to load library for addon " << info.uniqueName()
                    << " on " << libraryPath << ". Error: " << library.error();
            }
            if (!loaded) {
                break;
            }
        }

        if (libraries.size() == libnames.size()) {
            try {
                registry_.emplace(info.uniqueName(),
                                  std::make_unique<SharedLibraryFactory>(
                                      info, std::move(libraries)));
            } catch (const std::exception &e) {
                FCITX_ERROR() << "Failed to initialize addon factory for addon "
                              << info.uniqueName() << ". Error: " << e.what();
            }
            iter = registry_.find(info.uniqueName());
        }
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

} // namespace fcitx
