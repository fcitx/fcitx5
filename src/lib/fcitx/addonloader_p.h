/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_ADDONLOADER_P_H_
#define _FCITX_ADDONLOADER_P_H_

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include "fcitx-utils/library.h"
#include "fcitx-utils/standardpath.h"
#include "fcitx-utils/stringutils.h"
#include "addonfactory.h"
#include "addoninfo.h"
#include "addoninstance.h"
#include "addonloader.h"

namespace fcitx {

namespace {
constexpr char FCITX_ADDON_FACTORY_ENTRY[] = "fcitx_addon_factory_instance";
}

class SharedLibraryFactory {
public:
    SharedLibraryFactory(const AddonInfo &info, Library lib)
        : library_(std::move(lib)) {
        std::string v2Name = stringutils::concat(FCITX_ADDON_FACTORY_ENTRY, "_",
                                                 info.uniqueName());
        auto *funcPtr = library_.resolve(v2Name.data());
        if (!funcPtr) {
            funcPtr = library_.resolve(FCITX_ADDON_FACTORY_ENTRY);
        }
        if (!funcPtr) {
            throw std::runtime_error(library_.error());
        }
        auto func = Library::toFunction<AddonFactory *()>(funcPtr);
        factory_ = func();
        if (!factory_) {
            throw std::runtime_error("Failed to get a factory");
        }
    }

    AddonFactory *factory() { return factory_; }

private:
    Library library_;
    AddonFactory *factory_;
};

class SharedLibraryLoader : public AddonLoader {
public:
    ~SharedLibraryLoader();
    AddonInstance *load(const AddonInfo &info, AddonManager *manager) override;

    std::string type() const override { return "SharedLibrary"; }

private:
    StandardPath standardPath_;
    std::unordered_map<std::string, std::unique_ptr<SharedLibraryFactory>>
        registry_;
};

class StaticLibraryLoader : public AddonLoader {
public:
    StaticLibraryLoader(StaticAddonRegistry *registry_);

    AddonInstance *load(const AddonInfo &info, AddonManager *manager) override;

    std::string type() const override { return "StaticLibrary"; }

    StaticAddonRegistry *registry;
};
} // namespace fcitx

#endif // _FCITX_ADDONLOADER_P_H_
