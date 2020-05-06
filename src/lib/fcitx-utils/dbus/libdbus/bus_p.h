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
#ifndef _FCITX_UTILS_DBUS_BUS_P_H_
#define _FCITX_UTILS_DBUS_BUS_P_H_

#include <dbus/dbus.h>
#include "../../log.h"
#include "../bus.h"
#include "servicenamecache.h"

namespace fcitx {
namespace dbus {

FCITX_DECLARE_LOG_CATEGORY(libdbus_logcategory);

#define FCITX_LIBDBUS_DEBUG()                                                  \
    FCITX_LOGC(::fcitx::dbus::libdbus_logcategory, Debug)

DBusHandlerResult DBusObjectPathVTableMessageCallback(DBusConnection *,
                                                      DBusMessage *message,
                                                      void *userdata);

class ScopedDBusError {
public:
    ScopedDBusError() { dbus_error_init(&error_); }
    ~ScopedDBusError() { dbus_error_free(&error_); }

    DBusError &error() { return error_; }

private:
    DBusError error_;
};

class DBusObjectVTableSlot : public Slot,
                             public TrackableObject<DBusObjectVTableSlot> {
public:
    DBusObjectVTableSlot(const std::string &path, const std::string &interface,
                         ObjectVTableBase *obj,
                         ObjectVTableBasePrivate *objPriv)
        : path_(path), interface_(interface), obj_(obj), objPriv_(objPriv),
          xml_(getXml()) {}

    ~DBusObjectVTableSlot() {}

    std::string getXml();

    std::string path_;
    std::string interface_;
    ObjectVTableBase *obj_;
    ObjectVTableBasePrivate *objPriv_;
    TrackableObjectReference<BusPrivate> bus_;
    std::unique_ptr<HandlerTableEntryBase> handler_;
    std::string xml_;
};

class BusPrivate : public TrackableObject<BusPrivate> {
public:
    BusPrivate(Bus *bus)
        : bus_(bus), conn_(nullptr, &DBusConnectionCloser),
          matchRuleSet_(
              [this](const MatchRule &rule) {
                  if (!conn_) {
                      return false;
                  }
                  ScopedDBusError error;
                  if (needWatchService(rule)) {
                      nameCache()->addWatch(rule.service());
                  }
                  FCITX_LIBDBUS_DEBUG() << "Add dbus match: " << rule.rule();
                  dbus_bus_add_match(conn_.get(), rule.rule().c_str(),
                                     &error.error());
                  bool isError = dbus_error_is_set(&error.error());
                  if (!isError) {
                      return true;
                  }
                  return false;
              },
              [this](const MatchRule &rule) {
                  if (!conn_) {
                      return;
                  }
                  if (needWatchService(rule)) {
                      nameCache()->removeWatch(rule.service());
                  }
                  FCITX_LIBDBUS_DEBUG() << "Remove dbus match: " << rule.rule();
                  dbus_bus_remove_match(conn_.get(), rule.rule().c_str(),
                                        nullptr);
              }),
          objectRegistration_(
              [this](const std::string &path) {
                  if (!conn_) {
                      return false;
                  }
                  DBusObjectPathVTable vtable;
                  memset(&vtable, 0, sizeof(vtable));

                  vtable.message_function = DBusObjectPathVTableMessageCallback;
                  if (!dbus_connection_register_object_path(
                          conn_.get(), path.c_str(), &vtable, this)) {
                      return false;
                  }
                  return true;
              },
              [this](const std::string &path) {
                  if (!conn_) {
                      return;
                  }

                  dbus_connection_unregister_object_path(conn_.get(),
                                                         path.c_str());
              }) {}

    ~BusPrivate() {
        if (conn_) {
            dbus_connection_flush(conn_.get());
        }
    }

    void dispatch() {
        if (!conn_) {
            return;
        }
        dbus_connection_ref(conn_.get());
        while (dbus_connection_dispatch(conn_.get()) ==
               DBUS_DISPATCH_DATA_REMAINS) {
        }
        dbus_connection_unref(conn_.get());
    }

    static bool needWatchService(const MatchRule &rule) {
        // Non bus and non empty.
        return !rule.service().empty() &&
               rule.service() != "org.freedesktop.DBus";
    }

    ServiceNameCache *nameCache() {
        if (!nameCache_) {
            nameCache_ = std::make_unique<ServiceNameCache>(*bus_);
        }
        return nameCache_.get();
    }

    DBusObjectVTableSlot *findSlot(const std::string &path,
                                   const std::string interface);
    bool objectVTableCallback(Message &message);

    static void DBusConnectionCloser(DBusConnection *conn) {
        if (conn) {
            dbus_connection_close(conn);
            dbus_connection_unref(conn);
        }
    }

    Bus *bus_;
    std::string address_;
    std::unique_ptr<DBusConnection, decltype(&DBusConnectionCloser)> conn_;
    MultiHandlerTable<MatchRule, int> matchRuleSet_;
    HandlerTable<std::pair<MatchRule, MessageCallback>> matchHandlers_;
    HandlerTable<MessageCallback> filterHandlers_;
    bool attached_ = false;
    EventLoop *loop_ = nullptr;
    std::unordered_map<DBusWatch *, std::unique_ptr<EventSourceIO>> ioWatchers_;
    std::unordered_map<DBusTimeout *, std::unique_ptr<EventSourceTime>>
        timeWatchers_;
    MultiHandlerTable<std::string,
                      TrackableObjectReference<DBusObjectVTableSlot>>
        objectRegistration_;
    std::unique_ptr<EventSource> deferEvent_;
    std::unique_ptr<ServiceNameCache> nameCache_;
};

class DBusObjectSlot : public Slot {
public:
    DBusObjectSlot(const std::string &path, MessageCallback callback)
        : path_(path), callback_(std::move(callback)) {}

    ~DBusObjectSlot() {
        if (auto conn = connection()) {
            dbus_connection_unregister_object_path(conn, path_.data());
        }
    }

    DBusConnection *connection() {
        if (auto bus = bus_.get()) {
            return bus->conn_.get();
        }
        return nullptr;
    }

    std::string path_;
    MessageCallback callback_;
    TrackableObjectReference<BusPrivate> bus_;
};

class DBusMatchSlot : public Slot {
public:
    std::unique_ptr<HandlerTableEntryBase> ruleRef_;
    std::unique_ptr<HandlerTableEntryBase> handler_;
};

class DBusFilterSlot : public Slot {
public:
    std::unique_ptr<HandlerTableEntryBase> handler_;
};

class DBusAsyncCallSlot : public Slot {
public:
    DBusAsyncCallSlot(MessageCallback callback)
        : callback_(std::move(callback)) {}

    ~DBusAsyncCallSlot() {
        if (reply_) {
            dbus_pending_call_set_notify(reply_, nullptr, nullptr, nullptr);
            dbus_pending_call_unref(reply_);
            reply_ = nullptr;
        }
    }

    MessageCallback callback_;
    DBusPendingCall *reply_ = nullptr;
    TrackableObjectReference<BusPrivate> bus_;
};
} // namespace dbus
} // namespace fcitx

#endif // _FCITX_UTILS_DBUS_BUS_P_H_
