/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_DBUS_LIBDBUS_SERVICENAMECACHE_P_H_
#define _FCITX_UTILS_DBUS_LIBDBUS_SERVICENAMECACHE_P_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include "../../handlertable.h"

namespace fcitx::dbus {

class Bus;
class ServiceWatcher;

class ServiceNameCache {
public:
    ServiceNameCache(Bus &bus);
    ~ServiceNameCache();
    std::string owner(const std::string &query);
    void addWatch(const std::string &name);
    void removeWatch(const std::string &name);

private:
    std::unique_ptr<ServiceWatcher> watcher_;
    std::unordered_map<std::string, std::string> nameMap_;
    std::unordered_map<std::string,
                       std::pair<int, std::unique_ptr<HandlerTableEntryBase>>>
        watcherMap_;
};

} // namespace fcitx::dbus

#endif // _FCITX_UTILS_DBUS_LIBDBUS_SERVICENAMECACHE_P_H_
