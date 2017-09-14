/*
 * Copyright (C) 2016~2016 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
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
#ifndef _FCITX_UTILS_DBUS_SERVICEWATCHER_H_
#define _FCITX_UTILS_DBUS_SERVICEWATCHER_H_

#include <fcitx-utils/dbus/bus.h>
#include <fcitx-utils/handlertable.h>
#include <fcitx-utils/macros.h>
#include <memory>
#include <string>

namespace fcitx {
namespace dbus {

typedef std::function<void(const std::string &serviceName,
                           const std::string &oldOwner,
                           const std::string &newOwner)>
    ServiceWatcherCallback;
typedef HandlerTableEntry<ServiceWatcherCallback> ServiceWatcherEntry;

class ServiceWatcherPrivate;

class FCITXUTILS_EXPORT ServiceWatcher {
public:
    ServiceWatcher(Bus &bus);
    ~ServiceWatcher();

    // unlike regular NameOwnerChanged signal, this will also initiate a
    // GetNameOwner call to avoid race condition
    // if GetNameOwner returns, it will intiate a call (name, "", owner) if
    // service exists, otherwise (name, "", "")
    ServiceWatcherEntry *watchService(const std::string &name,
                                      ServiceWatcherCallback callback);

private:
    std::unique_ptr<ServiceWatcherPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(ServiceWatcher);
};
}
}

#endif // _FCITX_UTILS_DBUS_SERVICEWATCHER_H_
