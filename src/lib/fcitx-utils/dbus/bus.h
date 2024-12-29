/*
 * SPDX-FileCopyrightText: 2015-2015 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_DBUS_BUS_H_
#define _FCITX_UTILS_DBUS_BUS_H_

#include <cstdint>
#include <memory>
#include <string>
#include <fcitx-utils/dbus/matchrule.h>
#include <fcitx-utils/dbus/message.h>
#include <fcitx-utils/dbus/objectvtable.h>
#include <fcitx-utils/event.h>
#include <fcitx-utils/flags.h>
#include <fcitx-utils/macros.h>
#include "fcitxutils_export.h"

/// \addtogroup FcitxUtils
/// \{
/// \file
/// \brief API for DBus bus.

namespace fcitx::dbus {

/**
 * Virtual base class represent some internal registration of the bus.
 *
 * Mainly used with C++ RAII idiom.
 */
class FCITXUTILS_EXPORT Slot {
public:
    virtual ~Slot();
};

enum class BusType { Default, Session, System };
enum class RequestNameFlag {
    None = 0,
    ReplaceExisting = 1ULL << 0,
    AllowReplacement = 1ULL << 1,
    Queue = 1ULL << 2
};

class BusPrivate;

/**
 * A class that represents a connection to the Bus.
 */
class FCITXUTILS_EXPORT Bus {
public:
    /// Connect to given address.
    Bus(const std::string &address);

    /// Connect to given dbus type.
    Bus(BusType type);

    virtual ~Bus();
    Bus(const Bus &other) = delete;
    Bus(Bus &&other) noexcept;

    /// Check if the connection is open.
    FCITX_NODISCARD bool isOpen() const;

    /// Attach this bus to an event loop.
    void attachEventLoop(EventLoop *loop);

    /// Remove this bus from an event loop.
    void detachEventLoop();

    /**
     * Return the attached event loop
     *
     * @return attached event loop, not necessary a valid pointer if event loop
     * is destructed before bus.
     *
     * @since 5.0.22
     */
    FCITX_NODISCARD EventLoop *eventLoop() const;

    FCITX_NODISCARD std::unique_ptr<Slot> addMatch(const MatchRule &rule,
                                                   MessageCallback callback);
    FCITX_NODISCARD std::unique_ptr<Slot> addFilter(MessageCallback callback);
    FCITX_NODISCARD std::unique_ptr<Slot> addObject(const std::string &path,
                                                    MessageCallback callback);
    /**
     * Register a new object on the dbus.
     *
     * @param path object path
     * @param interface object interface
     * @param obj object
     * @return registration succeeds or not.
     */
    bool addObjectVTable(const std::string &path, const std::string &interface,
                         ObjectVTableBase &vtable);

    /// Create a new signal message
    Message createSignal(const char *path, const char *interface,
                         const char *member);

    /// Create a new method message.
    Message createMethodCall(const char *destination, const char *path,
                             const char *interface, const char *member);

    /**
     * Return the name of the compiled implentation of fcitx dbus
     *
     * @return "sdbus" or "libdbus"
     */
    static const char *impl();

    /**
     * Return the internal pointer of the implemenation.
     *
     * @return internal pointer
     */
    FCITX_NODISCARD void *nativeHandle() const;

    /**
     * Request the dbus name on the bus.
     *
     * @param name service name
     * @param flags request name flag.
     * @return requesting name is successful or not.
     */
    bool requestName(const std::string &name, Flags<RequestNameFlag> flags);

    /// Release the dbus name.
    bool releaseName(const std::string &name);

    /**
     * Helper function to query the service owner.
     *
     * @param name dbus name
     * @param usec dbus timeout
     * @return unique name of the owner.
     */
    std::string serviceOwner(const std::string &name, uint64_t usec);
    std::unique_ptr<Slot> serviceOwnerAsync(const std::string &name,
                                            uint64_t usec,
                                            MessageCallback callback);

    /**
     * Return the unique name of current connection. E.g. :1.34
     *
     * @return unique name
     */
    std::string uniqueName();

    /**
     * Return the dbus address being connected to.
     *
     * @return dbus address
     */
    std::string address();

    /**
     * Flush the bus immediately.
     */
    void flush();

private:
    std::unique_ptr<BusPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(Bus);
};
} // namespace fcitx::dbus

#endif // _FCITX_UTILS_DBUS_BUS_H_
