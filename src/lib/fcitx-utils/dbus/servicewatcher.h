/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_DBUS_SERVICEWATCHER_H_
#define _FCITX_UTILS_DBUS_SERVICEWATCHER_H_

#include <memory>
#include <string>
#include <fcitx-utils/dbus/bus.h>
#include <fcitx-utils/handlertable.h>
#include <fcitx-utils/macros.h>

/// \addtogroup FcitxUtils
/// \{
/// \file
/// \brief API for service monitoring.

namespace fcitx {
namespace dbus {

typedef std::function<void(const std::string &serviceName,
                           const std::string &oldOwner,
                           const std::string &newOwner)>
    ServiceWatcherCallback;
typedef HandlerTableEntry<ServiceWatcherCallback> ServiceWatcherEntry;

class ServiceWatcherPrivate;

/**
 * A class to be used to monitor the services registered on the Bus.
 */
class FCITXUTILS_EXPORT ServiceWatcher {
public:
    ServiceWatcher(Bus &bus);
    ~ServiceWatcher();

    // unlike regular NameOwnerChanged signal, this will also initiate a
    // GetNameOwner call to avoid race condition
    // if GetNameOwner returns, it will intiate a call (name, "", owner) if
    // service exists, otherwise (name, "", "")
    FCITX_NODISCARD std::unique_ptr<ServiceWatcherEntry>
    watchService(const std::string &name, ServiceWatcherCallback callback);

private:
    std::unique_ptr<ServiceWatcherPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(ServiceWatcher);
};
} // namespace dbus
} // namespace fcitx

#endif // _FCITX_UTILS_DBUS_SERVICEWATCHER_H_
