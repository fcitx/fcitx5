/*
 * Copyright (C) 2016~2016 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; see the file COPYING. If not,
 * see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include "addonloader_p.h"
#include "fcitx-utils/library.h"
#include "config.h"

namespace fcitx {

AddonLoader::~AddonLoader() {}

SharedLibraryLoader::~SharedLibraryLoader() {}

AddonInstance *SharedLibraryLoader::load(const AddonInfo &info,
                                         AddonManager *manager) {
    auto iter = m_registry.find(info.name());
    if (iter == m_registry.end()) {
        auto libs = m_standardPath.locateAll(
            StandardPath::Type::Addon, info.library() + FCITX_LIBRARY_SUFFIX);
        for (const auto &libraryPath : libs) {
            Library lib(libraryPath);
            if (!lib.load()) {
                continue;
            }
            try {
                m_registry.emplace(
                    info.name(),
                    std::make_unique<SharedLibraryFactory>(std::move(lib)));
            } catch (const std::exception &e) {
                std::cout << e.what() << std::endl;
            }
            break;
        }
        iter = m_registry.find(info.name());
    }

    if (iter == m_registry.end()) {
        return nullptr;
    }
    return iter->second->factory()->create(manager);
}

StaticLibraryLoader::StaticLibraryLoader(StaticAddonRegistry *registry_)
    : AddonLoader(), registry(registry_) {}

AddonInstance *StaticLibraryLoader::load(const AddonInfo &info,
                                         AddonManager *manager) {
    auto iter = registry->find(info.name());
    if (iter == registry->end()) {
        return nullptr;
    }
    return iter->second->create(manager);
}
}
