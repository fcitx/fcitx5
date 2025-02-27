/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_ADDONMANAGER_H_
#define _FCITX_ADDONMANAGER_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <fcitx-utils/macros.h>
#include <fcitx-utils/semver.h>
#include <fcitx/addoninfo.h>
#include <fcitx/addoninstance.h>
#include <fcitx/addonloader.h>
#include <fcitx/fcitxcore_export.h>

/// \addtogroup FcitxCore
/// \{
/// \file
/// \brief Addon Manager class

namespace fcitx {

class Instance;
class EventLoop;
class AddonManagerPrivate;
class FCITXCORE_EXPORT AddonManager {
    friend class Instance;

public:
    /// Construct an addon manager.
    AddonManager();

    /**
     * Create addon manager with given addon config dir.
     *
     * By default, addonConfigDir is set to "addon".
     * It can be a relative path to PkgData, or an absolute path.
     * This function is only used by test.
     *
     * @param addonConfigDir directory name.
     *
     * @see StandardPath
     */
    AddonManager(const std::string &addonConfigDir);

    /**
     * Destruct and unload all addons.
     *
     */
    virtual ~AddonManager();

    /**
     * Register addon loader, including static and shared library loader.
     *
     * This function usually need to be called before any other function call to
     * addon manager.
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
     * @param name name of addon type.
     */
    void unregisterLoader(const std::string &name);

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
     * Get the currently loaded addon instance.
     *
     * This is same as AddonManager::addon(name, false), but allow to be used
     * with a constant AddonManager.
     *
     * @param name of addon.
     * @return instance of addon, null if not found.
     * @since 5.1.6
     */
    AddonInstance *lookupAddon(const std::string &name) const;

    /**
     * Return the loaded addon name in the order of they were loaded.
     *
     * @return the name of loaded addons.
     * @since 5.1.6
     */
    const std::vector<std::string> &loadedAddonNames() const;

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

    /**
     * Return the version number of Fcitx5Core library.
     */
    const SemanticVersion &version() const;

    /**
     * Check directory for quick hint for whether update is required.
     *
     * @since 5.0.6
     */
    bool checkUpdate() const;

    /**
     * Set addon parameters that may be used during addon construction.
     *
     * This is usually passed from command line flags --option or -o.
     *
     * @param options map from addon name to a set of string values
     * @since 5.1.7
     */
    void setAddonOptions(
        std::unordered_map<std::string, std::vector<std::string>> options);

    /**
     * Query addon options that set with setAddonOptions for given addon.
     *
     * @param name addon name
     * @return Options for given addon
     * @since 5.1.7
     */
    std::vector<std::string> addonOptions(const std::string &name);

private:
    void setInstance(Instance *instance);
    std::unique_ptr<AddonManagerPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(AddonManager);
};
} // namespace fcitx

#endif // _FCITX_ADDONMANAGER_H_
