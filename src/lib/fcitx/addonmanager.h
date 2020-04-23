//
// Copyright (C) 2016~2016 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//
#ifndef _FCITX_ADDONMANAGER_H_
#define _FCITX_ADDONMANAGER_H_

#include <memory>
#include <string>
#include <unordered_set>
#include <fcitx-utils/macros.h>
#include <fcitx/addoninfo.h>
#include <fcitx/addonloader.h>
#include "fcitxcore_export.h"

namespace fcitx {

class Instance;
class EventLoop;
class AddonManagerPrivate;
class FCITXCORE_EXPORT AddonManager {
    friend class Instance;

public:
    AddonManager();
    AddonManager(const std::string &addonConfigDir);

    /**
     * Destruct and unload all addons.
     *
     */
    virtual ~AddonManager();

    /**
     * Register addon loader, including static and shared library loader.
     *
     * @param registry static addon registry that can be used to set a list of
     * built-in addons.
     */
    void registerDefaultLoader(StaticAddonRegistry *registry);
    /**
     * Register new addon loader.
     *
     * @param loader addon loader instance.
     */
    void registerLoader(std::unique_ptr<AddonLoader> loader);

    /**
     * Unregister addon loader.
     *
     * @param type name of addon type.
     */
    void unregisterLoader(const std::string &type);
    /**
     * Load addon based on given parameter.
     *
     * By default, addon is enable or disabled by config file, but
     * enabled or disabled may be used to override it.
     *
     * Usually this function should only be called once.
     * You can pass --enable=... --disable= in fcitx's flag to set it.
     * "enabled" will override "disabled" if they have same addon name in it.
     *
     * A special name "all" can be used to enable or disable all addons.
     *
     * @param enabled set of additionally enabled addons.
     * @param disabled set of disabled addons
     */
    void load(const std::unordered_set<std::string> &enabled = {},
              const std::unordered_set<std::string> &disabled = {});
    /**
     * Destruct all addon, all information is cleared to the initial state.
     *
     * But depending on the addon it loads, it may have some leftover data in
     * the memory.
     */
    void unload();
    /**
     * Save all addon configuration.
     *
     * @see fcitx::AddonInstance::save
     */
    void saveAll();

    /**
     * Get the loaded addon instance.
     *
     * @param name name of addon.
     * @param load to force load the addon if possible.
     * @return instance of addon.
     */
    AddonInstance *addon(const std::string &name, bool load = false);
    /**
     * Get addon information for given addon.
     *
     * @param name name of addon.
     * @return const fcitx::AddonInfo*
     */
    const AddonInfo *addonInfo(const std::string &name) const;
    std::unordered_set<std::string> addonNames(AddonCategory category);

    /**
     * Return the fcitx instance when it is created by Fcitx.
     *
     * @return fcitx instance.
     */
    Instance *instance();
    /**
     * Return the associated event loop.
     *
     * If AddonManager is created by Instance, it will return the event loop of
     * associated instance.
     *
     * @return event loop.
     */
    EventLoop *eventLoop();

    /**
     * Set event loop.
     *
     * It should be only used with stand alone AddonManager.
     * E.g. write test or for some special purpose.
     *
     * @param eventLoop event loop.
     * @see fcitx::AddonManager::eventLoop
     */
    void setEventLoop(EventLoop *eventLoop);

private:
    void setInstance(Instance *instance);
    std::unique_ptr<AddonManagerPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(AddonManager);
};
} // namespace fcitx

#endif // _FCITX_ADDONMANAGER_H_
