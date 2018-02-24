//
// Copyright (C) 2017~2017 by CSSlayer
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
#ifndef _FCITX_UTILS_DBUS_LIBDBUS_SERVICENAMECACHE_P_H_
#define _FCITX_UTILS_DBUS_LIBDBUS_SERVICENAMECACHE_P_H_

#include "../../handlertable.h"
#include <memory>
#include <string>
#include <unordered_map>

namespace fcitx {
namespace dbus {

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

} // namespace dbus
} // namespace fcitx

#endif // _FCITX_UTILS_DBUS_LIBDBUS_SERVICENAMECACHE_P_H_
