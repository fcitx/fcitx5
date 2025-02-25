/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "../../dbus/bus.h"
#include <sys/types.h>
#include <unistd.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <dbus/dbus-protocol.h>
#include <dbus/dbus-shared.h>
#include <dbus/dbus.h>
#include "fcitx-utils/environ.h"
#include "../../charutils.h"
#include "../../dbus/matchrule.h"
#include "../../dbus/message.h"
#include "../../dbus/objectvtable.h"
#include "../../event.h"
#include "../../eventloopinterface.h"
#include "../../flags.h"
#include "../../log.h"
#include "../../macros.h"
#include "../../misc_p.h"
#include "../../stringutils.h"
#include "../../trackableobject.h"
#include "bus_p.h"
#include "config.h"
#include "message_p.h"
#include "objectvtable_p_libdbus.h"

namespace fcitx::dbus {

FCITX_DEFINE_LOG_CATEGORY(libdbus_logcategory, "libdbus");

class BusWatches : public std::enable_shared_from_this<BusWatches> {
    struct Private {};

public:
    BusWatches(BusPrivate &bus, Private /*unused*/) : bus_(bus.watch()) {}

    void addWatch(DBusWatch *watch) {
        watches_[watch] = std::make_shared<DBusWatch *>(watch);
        refreshWatch();
    }
    bool removeWatch(DBusWatch *watch) {
        watches_.erase(watch);
        refreshWatch();
        return watches_.empty();
    }
    void refreshWatch() {
        if (watches_.empty() || !bus_.isValid()) {
            ioEvent_.reset();
            return;
        }

        int fd = dbus_watch_get_unix_fd(watches_.begin()->first);
        IOEventFlags flags;
        for (const auto &[watch, _] : watches_) {
            if (!dbus_watch_get_enabled(watch)) {
                continue;
            }
            int dflags = dbus_watch_get_flags(watch);
            if (dflags & DBUS_WATCH_READABLE) {
                flags |= IOEventFlag::In;
            }
            if (dflags & DBUS_WATCH_WRITABLE) {
                flags |= IOEventFlag::Out;
            }
        }

        FCITX_LIBDBUS_DEBUG()
            << "IOWatch for dbus fd: " << fd << " flags: " << flags;
        if (flags == 0) {
            ioEvent_.reset();
            return;
        }

        if (!ioEvent_) {
            ioEvent_ = bus_.get()->loop_->addIOEvent(
                fd, flags, [this](EventSourceIO *, int, IOEventFlags flags) {
                    // Ensure this is valid.
                    // At this point, callback is always valid, so no need to
                    // keep "this".
                    auto lock = shared_from_this();
                    // Create a copy of watcher pointers, so we can safely
                    // remove the watcher during the loop.
                    std::vector<std::weak_ptr<DBusWatch *>> watchesView;
                    watchesView.reserve(watches_.size());
                    for (const auto &[_, watchRef] : watches_) {
                        watchesView.push_back(watchRef);
                    }

                    for (const auto &watchRef : watchesView) {
                        auto watchStrongRef = watchRef.lock();
                        if (!watchStrongRef) {
                            continue;
                        }
                        auto *watch = *watchStrongRef;
                        if (!dbus_watch_get_enabled(watch)) {
                            continue;
                        }

                        int dflags = 0;

                        if ((dbus_watch_get_flags(watch) &
                             DBUS_WATCH_READABLE) &&
                            (flags & IOEventFlag::In)) {
                            dflags |= DBUS_WATCH_READABLE;
                        }
                        if ((dbus_watch_get_flags(watch) &
                             DBUS_WATCH_WRITABLE) &&
                            (flags & IOEventFlag::Out)) {
                            dflags |= DBUS_WATCH_WRITABLE;
                        }
                        if (flags & IOEventFlag::Err) {
                            dflags |= DBUS_WATCH_ERROR;
                        }
                        if (flags & IOEventFlag::Hup) {
                            dflags |= DBUS_WATCH_HANGUP;
                        }
                        if (!dflags) {
                            continue;
                        }
                        dbus_watch_handle(watch, dflags);
                        if (auto *bus = bus_.get()) {
                            bus->dispatch();
                        }
                    }
                    return true;
                });
        } else {
            ioEvent_->setEvents(flags);
        }
    }

    static std::shared_ptr<BusWatches> create(BusPrivate &bus) {
        return std::make_shared<BusWatches>(bus, Private());
    }

private:
    TrackableObjectReference<BusPrivate> bus_;
    // We the value as shared ptr so we know when the watch is removed during
    // the loop.
    std::unordered_map<DBusWatch *, std::shared_ptr<DBusWatch *>> watches_;
    std::unique_ptr<EventSourceIO> ioEvent_;
};

DBusHandlerResult DBusMessageCallback(DBusConnection * /*unused*/,
                                      DBusMessage *message, void *userdata) {
    auto *bus = static_cast<BusPrivate *>(userdata);
    if (!bus) {
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
    try {
        auto ref = bus->watch();
        auto msg = MessagePrivate::fromDBusMessage(ref, message, false, true);
        for (const auto &filter : bus->filterHandlers_.view()) {
            if (filter && filter(msg)) {
                return DBUS_HANDLER_RESULT_HANDLED;
            }
            msg.rewind();
        }

        if (msg.type() == MessageType::Signal) {
            if (auto *bus = ref.get()) {
                for (auto &pair : bus->matchHandlers_.view()) {
                    auto *bus = ref.get();
                    std::string alterName;
                    if (bus && bus->nameCache_ &&
                        !pair.first.service().empty()) {
                        alterName =
                            bus->nameCache_->owner(pair.first.service());
                    }
                    if (pair.first.check(msg, alterName)) {
                        if (pair.second && pair.second(msg)) {
                            return DBUS_HANDLER_RESULT_HANDLED;
                        }
                    }
                    msg.rewind();
                }
            }
        }
    } catch (const std::exception &e) {
        // some abnormal things threw
        FCITX_ERROR() << e.what();
        std::abort();
    }
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

Slot::~Slot() {}

constexpr const char xmlHeader[] =
    "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection "
    "1.0//EN\" "
    "\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">"
    "<node>"
    "<interface name=\"" DBUS_INTERFACE_INTROSPECTABLE "\">"
    "<method name=\"Introspect\">"
    "<arg name=\"data\" direction=\"out\" type=\"s\"/>"
    "</method>"
    "</interface>";
constexpr const char xmlProperties[] =
    "<interface name=\"" DBUS_INTERFACE_PROPERTIES "\">"
    "<method name=\"Get\">"
    "<arg name=\"interface_name\" direction=\"in\" type=\"s\"/>"
    "<arg name=\"property_name\" direction=\"in\" type=\"s\"/>"
    "<arg name=\"value\" direction=\"out\" type=\"v\"/>"
    "</method>"
    "<method name=\"Set\">"
    "<arg name=\"interface_name\" direction=\"in\" type=\"s\"/>"
    "<arg name=\"property_name\" direction=\"in\" type=\"s\"/>"
    "<arg name=\"value\" direction=\"in\" type=\"v\"/>"
    "</method>"
    "<method name=\"GetAll\">"
    "<arg name=\"interface_name\" direction=\"in\" type=\"s\"/>"
    "<arg name=\"values\" direction=\"out\" type=\"a{sv}\"/>"
    "</method>"
    "<signal name=\"PropertiesChanged\">"
    "<arg name=\"interface_name\" type=\"s\"/>"
    "<arg name=\"changed_properties\" type=\"a{sv}\"/>"
    "<arg name=\"invalidated_properties\" type=\"as\"/>"
    "</signal>"
    "</interface>";

constexpr const char xmlInterfaceFooter[] = "</interface>";

constexpr const char xmlFooter[] = "</node>";

std::string DBusObjectVTableSlot::getXml() const {
    std::string xml;
    xml += stringutils::concat("<interface name=\"", interface_, "\">");
    xml += objPriv_->getXml(obj_);
    xml += xmlInterfaceFooter;
    return xml;
}

BusPrivate::BusPrivate(Bus *bus)
    : bus_(bus),
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
              return !isError;
          },
          [this](const MatchRule &rule) {
              if (!conn_) {
                  return;
              }
              if (needWatchService(rule)) {
                  nameCache()->removeWatch(rule.service());
              }
              FCITX_LIBDBUS_DEBUG() << "Remove dbus match: " << rule.rule();
              dbus_bus_remove_match(conn_.get(), rule.rule().c_str(), nullptr);
          }),
      objectRegistration_(
          [this](const std::string &path) {
              if (!conn_) {
                  return false;
              }
              DBusObjectPathVTable vtable;
              memset(&vtable, 0, sizeof(vtable));

              vtable.message_function = DBusObjectPathVTableMessageCallback;
              return dbus_connection_register_object_path(
                         conn_.get(), path.c_str(), &vtable, this) != 0;
          },
          [this](const std::string &path) {
              if (!conn_) {
                  return;
              }

              dbus_connection_unregister_object_path(conn_.get(), path.c_str());
          }) {}

BusPrivate::~BusPrivate() {
    if (conn_) {
        dbus_connection_flush(conn_.get());
    }
}

DBusObjectVTableSlot *BusPrivate::findSlot(const std::string &path,
                                           const std::string &interface) {
    // Check if interface exists.
    for (auto &item : objectRegistration_.view(path)) {
        if (auto *slot = item.get()) {
            if (slot->interface_ == interface) {
                return slot;
            }
        }
    }
    return nullptr;
}

bool BusPrivate::objectVTableCallback(Message &message) {
    if (!objectRegistration_.hasKey(message.path())) {
        return false;
    }
    if (message.interface() == "org.freedesktop.DBus.Introspectable") {
        if (message.member() != "Introspect" || !message.signature().empty()) {
            return false;
        }
        std::string xml = xmlHeader;
        bool hasProperties = false;
        for (auto &item : objectRegistration_.view(message.path())) {
            if (auto *slot = item.get()) {
                hasProperties =
                    hasProperties || !slot->objPriv_->properties_.empty();
                xml += slot->xml_;
            }
        }
        if (hasProperties) {
            xml += xmlProperties;
        }
        xml += xmlFooter;
        auto reply = message.createReply();
        reply << xml;
        reply.send();
        return true;
    }
    if (message.interface() == "org.freedesktop.DBus.Properties") {
        if (message.member() == "Get" && message.signature() == "ss") {
            std::string interfaceName;
            std::string propertyName;
            message >> interfaceName >> propertyName;
            if (auto *slot = findSlot(message.path(), interfaceName)) {
                auto *property = slot->obj_->findProperty(propertyName);
                if (property) {
                    auto reply = message.createReply();
                    reply << Container(Container::Type::Variant,
                                       property->signature());
                    property->getMethod()(reply);
                    reply << ContainerEnd();
                    reply.send();
                } else {
                    auto reply = message.createError(
                        DBUS_ERROR_UNKNOWN_PROPERTY, "No such property");
                    reply.send();
                }
                return true;
            }
        } else if (message.member() == "Set" && message.signature() == "ssv") {
            std::string interfaceName;
            std::string propertyName;
            message >> interfaceName >> propertyName;
            if (auto *slot = findSlot(message.path(), interfaceName)) {
                auto *property = slot->obj_->findProperty(propertyName);
                if (property) {
                    if (property->writable()) {
                        message >> Container(Container::Type::Variant,
                                             property->signature());
                        if (message) {
                            auto reply = message.createReply();
                            static_cast<ObjectVTableWritableProperty *>(
                                property)
                                ->setMethod()(message);
                            message >> ContainerEnd();
                            reply.send();
                        }
                    } else {
                        auto reply =
                            message.createError(DBUS_ERROR_PROPERTY_READ_ONLY,
                                                "Read-only property");
                        reply.send();
                    }
                } else {
                    auto reply = message.createError(
                        DBUS_ERROR_UNKNOWN_PROPERTY, "No such property");
                    reply.send();
                }
                return true;
            }
        } else if (message.member() == "GetAll" && message.signature() == "s") {
            std::string interfaceName;
            message >> interfaceName;
            if (auto *slot = findSlot(message.path(), interfaceName)) {
                auto reply = message.createReply();
                reply << Container(Container::Type::Array, Signature("{sv}"));
                for (auto &pair : slot->objPriv_->properties_) {
                    if (pair.second->options().test(PropertyOption::Hidden)) {
                        continue;
                    }
                    reply << Container(Container::Type::DictEntry,
                                       Signature("sv"));
                    reply << pair.first;
                    auto *property = pair.second;
                    reply << Container(Container::Type::Variant,
                                       property->signature());
                    property->getMethod()(reply);
                    reply << ContainerEnd();
                    reply << ContainerEnd();
                }
                reply << ContainerEnd();
                reply.send();
                return true;
            }
        }
    } else if (auto *slot = findSlot(message.path(), message.interface())) {
        if (auto *method = slot->obj_->findMethod(message.member())) {
            if (method->signature() != message.signature()) {
                return false;
            }
            return method->handler()(std::move(message));
        }
        return false;
    }
    return false;
}

std::string escapePath(const std::string &path) {
    std::string newPath;
    newPath.reserve(path.size() * 3);
    for (auto c : path) {
        if (charutils::islower(c) || charutils::isupper(c) ||
            charutils::isdigit(c) || c == '_' || c == '-' || c == '/' ||
            c == '.') {
            newPath.push_back(c);
        } else {
            newPath.push_back('%');
            newPath.push_back(charutils::toHex(c >> 4));
            newPath.push_back(charutils::toHex(c & 0xf));
        }
    }

    return newPath;
}

std::string sessionBusAddress() {
    auto e = getEnvironment("DBUS_SESSION_BUS_ADDRESS");
    if (e) {
        return *e;
    }
    auto xdg = getEnvironment("XDG_RUNTIME_DIR");
    if (!xdg) {
        return {};
    }
    auto escapedXdg = escapePath(*xdg);
    return stringutils::concat("unix:path=", escapedXdg, "/bus");
}

std::string addressByType(BusType type) {
    switch (type) {
    case BusType::Session:
        return sessionBusAddress();
    case BusType::System:
        if (auto env = getEnvironment("DBUS_SYSTEM_BUS_ADDRESS")) {
            return *env;
        } else {
            return DBUS_SYSTEM_BUS_DEFAULT_ADDRESS;
        }
    case BusType::Default:
        if (auto starter = getEnvironment("DBUS_STARTER_BUS_TYPE")) {
            if (stringutils::startsWith(*starter, "system")) {
                return addressByType(BusType::System);
            }
            if (stringutils::startsWith(*starter, "user") ||
                stringutils::startsWith(*starter, "session")) {
                return addressByType(BusType::Session);
            }
        }
        if (auto address = getEnvironment("DBUS_STARTER_ADDRESS")) {
            return *address;
        }

        {
            uid_t uid = getuid();
            uid_t euid = geteuid();
            if (uid != euid || euid != 0) {
                return addressByType(BusType::Session);
            }
            return addressByType(BusType::System);
        }
    }
    return {};
}

Bus::Bus(BusType type) : Bus(addressByType(type)) {}

Bus::Bus(const std::string &address)
    : d_ptr(std::make_unique<BusPrivate>(this)) {
    FCITX_D();
    if (address.empty()) {
        goto fail;
    }
    d->address_ = address;
    d->conn_.reset(dbus_connection_open_private(address.c_str(), nullptr));
    if (!d->conn_) {
        goto fail;
    }

    dbus_connection_set_exit_on_disconnect(d->conn_.get(), false);

    if (!dbus_bus_register(d->conn_.get(), nullptr)) {
        goto fail;
    }
    if (!dbus_connection_add_filter(d->conn_.get(), DBusMessageCallback, d,
                                    nullptr)) {
        goto fail;
    }
    return;

fail:
    throw std::runtime_error("Failed to create dbus connection");
}

Bus::~Bus() {
    FCITX_D();
    if (d->loop_) {
        detachEventLoop();
    }
}

Bus::Bus(Bus &&other) noexcept : d_ptr(std::move(other.d_ptr)) {}

bool Bus::isOpen() const {
    FCITX_D();
    return d->conn_ && dbus_connection_get_is_connected(d->conn_.get());
}

Message Bus::createMethodCall(const char *destination, const char *path,
                              const char *interface, const char *member) {
    FCITX_D();
    auto *dmsg =
        dbus_message_new_method_call(destination, path, interface, member);
    if (!dmsg) {
        return {};
    }
    return MessagePrivate::fromDBusMessage(d->watch(), dmsg, true, false);
}

Message Bus::createSignal(const char *path, const char *interface,
                          const char *member) {
    FCITX_D();
    auto *dmsg = dbus_message_new_signal(path, interface, member);
    if (!dmsg) {
        return {};
    }
    return MessagePrivate::fromDBusMessage(d->watch(), dmsg, true, false);
}

void DBusToggleWatch(DBusWatch *watch, void *data) {
    auto *bus = static_cast<BusPrivate *>(data);
    if (auto *watchers =
            findValue(bus->ioWatchers_, dbus_watch_get_unix_fd(watch))) {
        watchers->get()->refreshWatch();
    }
}

dbus_bool_t DBusAddWatch(DBusWatch *watch, void *data) {
    auto *bus = static_cast<BusPrivate *>(data);
    int fd = dbus_watch_get_unix_fd(watch);
    FCITX_LIBDBUS_DEBUG() << "DBusAddWatch fd: " << fd
                          << " flags: " << dbus_watch_get_flags(watch);
    auto &watchers = bus->ioWatchers_[fd];
    if (!watchers) {
        watchers = BusWatches::create(*bus);
    }
    watchers->addWatch(watch);
    return true;
}

void DBusRemoveWatch(DBusWatch *watch, void *data) {
    FCITX_LIBDBUS_DEBUG() << "DBusRemoveWatch fd: "
                          << dbus_watch_get_unix_fd(watch);
    auto *bus = static_cast<BusPrivate *>(data);
    auto iter = bus->ioWatchers_.find(dbus_watch_get_unix_fd(watch));
    if (iter == bus->ioWatchers_.end()) {
        return;
    }

    if (iter->second->removeWatch(watch)) {
        bus->ioWatchers_.erase(iter);
    }
}

dbus_bool_t DBusAddTimeout(DBusTimeout *timeout, void *data) {
    auto *bus = static_cast<BusPrivate *>(data);
    if (!dbus_timeout_get_enabled(timeout)) {
        return false;
    }
    int interval = dbus_timeout_get_interval(timeout);
    FCITX_LIBDBUS_DEBUG() << "DBusAddTimeout: " << interval;
    auto ref = bus->watch();
    try {
        bus->timeWatchers_.emplace(
            timeout,
            bus->loop_->addTimeEvent(
                CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + interval * 1000ULL, 0,
                [timeout, ref](EventSourceTime *event, uint64_t) {
                    // Copy is required since the lambda may be deleted.
                    // NOLINTBEGIN(performance-unnecessary-copy-initialization)
                    const auto refPivot = ref;
                    // NOLINTEND(performance-unnecessary-copy-initialization)
                    if (dbus_timeout_get_enabled(timeout)) {
                        event->setNextInterval(
                            dbus_timeout_get_interval(timeout) * 1000ULL);
                        event->setOneShot();
                    }
                    dbus_timeout_handle(timeout);

                    if (auto *bus = refPivot.get()) {
                        bus->dispatch();
                    }
                    return true;
                }));
    } catch (const EventLoopException &) {
        return false;
    }
    return true;
}
void DBusRemoveTimeout(DBusTimeout *timeout, void *data) {
    auto *bus = static_cast<BusPrivate *>(data);
    bus->timeWatchers_.erase(timeout);
}

void DBusToggleTimeout(DBusTimeout *timeout, void *data) {
    DBusRemoveTimeout(timeout, data);
    DBusAddTimeout(timeout, data);
}

void DBusDispatchStatusCallback(DBusConnection * /*unused*/,
                                DBusDispatchStatus status, void *userdata) {
    auto *bus = static_cast<BusPrivate *>(userdata);
    if (status == DBUS_DISPATCH_DATA_REMAINS) {
        bus->deferEvent_->setOneShot();
    }
}

void Bus::attachEventLoop(EventLoop *loop) {
    FCITX_D();
    if (d->loop_) {
        return;
    }
    d->loop_ = loop;
    do {
        if (!dbus_connection_set_watch_functions(d->conn_.get(), DBusAddWatch,
                                                 DBusRemoveWatch,
                                                 DBusToggleWatch, d, nullptr)) {
            break;
        }
        if (!dbus_connection_set_timeout_functions(
                d->conn_.get(), DBusAddTimeout, DBusRemoveTimeout,
                DBusToggleTimeout, d, nullptr)) {
            break;
        }
        if (!d->deferEvent_) {
            d->deferEvent_ = d->loop_->addDeferEvent([d](EventSource *) {
                d->dispatch();
                return true;
            });
            d->deferEvent_->setOneShot();
        }
        dbus_connection_set_dispatch_status_function(
            d->conn_.get(), DBusDispatchStatusCallback, d, nullptr);
        d->attached_ = true;
        return;
    } while (0);

    detachEventLoop();
}

void Bus::detachEventLoop() {
    FCITX_D();
    dbus_connection_set_watch_functions(d->conn_.get(), nullptr, nullptr,
                                        nullptr, nullptr, nullptr);
    dbus_connection_set_timeout_functions(d->conn_.get(), nullptr, nullptr,
                                          nullptr, nullptr, nullptr);
    dbus_connection_set_dispatch_status_function(d->conn_.get(), nullptr,
                                                 nullptr, nullptr);
    d->deferEvent_.reset();
    d->loop_ = nullptr;
}

EventLoop *Bus::eventLoop() const {
    FCITX_D();
    return d->loop_;
}

std::unique_ptr<Slot> Bus::addMatch(const MatchRule &rule,
                                    MessageCallback callback) {
    FCITX_D();
    auto slot = std::make_unique<DBusMatchSlot>();

    FCITX_LIBDBUS_DEBUG() << "Add match for rule " << rule.rule()
                          << " in rule set " << d->matchRuleSet_.hasKey(rule);

    slot->ruleRef_ = d->matchRuleSet_.add(rule, 1);

    if (!slot->ruleRef_) {
        return nullptr;
    }
    slot->handler_ = d->matchHandlers_.add(rule, std::move(callback));

    return slot;
}

std::unique_ptr<Slot> Bus::addFilter(MessageCallback callback) {
    FCITX_D();

    auto slot = std::make_unique<DBusFilterSlot>();
    slot->handler_ = d->filterHandlers_.add(std::move(callback));

    return slot;
}

DBusHandlerResult DBusObjectPathMessageCallback(DBusConnection * /*unused*/,
                                                DBusMessage *message,
                                                void *userdata) {
    auto *slot = static_cast<DBusObjectSlot *>(userdata);
    if (!slot) {
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
    auto msg =
        MessagePrivate::fromDBusMessage(slot->bus_, message, false, true);
    if (slot->callback_(msg)) {
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

std::unique_ptr<Slot> Bus::addObject(const std::string &path,
                                     MessageCallback callback) {
    FCITX_D();
    auto slot = std::make_unique<DBusObjectSlot>(path, std::move(callback));
    DBusObjectPathVTable vtable;
    memset(&vtable, 0, sizeof(vtable));
    vtable.message_function = DBusObjectPathMessageCallback;
    if (dbus_connection_register_object_path(d->conn_.get(), path.c_str(),
                                             &vtable, slot.get())) {
        return nullptr;
    }

    slot->bus_ = d->watch();
    return slot;
}

DBusHandlerResult
DBusObjectPathVTableMessageCallback(DBusConnection * /*unused*/,
                                    DBusMessage *message, void *userdata) {
    auto *bus = static_cast<BusPrivate *>(userdata);
    if (!bus) {
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
    auto msg =
        MessagePrivate::fromDBusMessage(bus->watch(), message, false, true);
    if (bus->objectVTableCallback(msg)) {
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

bool Bus::addObjectVTable(const std::string &path, const std::string &interface,
                          ObjectVTableBase &vtable) {
    FCITX_D();
    // Check if interface exists.
    for (auto &item : d->objectRegistration_.view(path)) {
        if (auto *slot = item.get()) {
            if (slot->interface_ == interface) {
                return false;
            }
        }
    }

    auto slot = std::make_unique<DBusObjectVTableSlot>(path, interface, &vtable,
                                                       vtable.d_func());

    auto handler = d->objectRegistration_.add(path, slot->watch());
    if (!handler) {
        return false;
    }

    slot->handler_ = std::move(handler);
    slot->bus_ = d->watch();

    vtable.setSlot(slot.release());
    return true;
}

const char *Bus::impl() { return "libdbus"; }

void *Bus::nativeHandle() const {
    FCITX_D();
    return d->conn_.get();
}

bool Bus::requestName(const std::string &name, Flags<RequestNameFlag> flags) {
    FCITX_D();
    int d_flags =
        ((flags & RequestNameFlag::ReplaceExisting)
             ? DBUS_NAME_FLAG_REPLACE_EXISTING
             : 0) |
        ((flags & RequestNameFlag::AllowReplacement)
             ? DBUS_NAME_FLAG_ALLOW_REPLACEMENT
             : 0) |
        ((flags & RequestNameFlag::Queue) ? 0 : DBUS_NAME_FLAG_DO_NOT_QUEUE);
    auto ret =
        dbus_bus_request_name(d->conn_.get(), name.c_str(), d_flags, nullptr);
    return ret == DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER ||
           ret == DBUS_REQUEST_NAME_REPLY_ALREADY_OWNER ||
           ((ret == DBUS_REQUEST_NAME_REPLY_IN_QUEUE ||
             ret == DBUS_REQUEST_NAME_REPLY_EXISTS) &&
            (flags & RequestNameFlag::Queue));
}

bool Bus::releaseName(const std::string &name) {
    FCITX_D();
    return dbus_bus_release_name(d->conn_.get(), name.c_str(), nullptr) >= 0;
}

std::string Bus::serviceOwner(const std::string &name, uint64_t usec) {
    auto msg = createMethodCall("org.freedesktop.DBus", "/org/freedesktop/DBus",
                                "org.freedesktop.DBus", "GetNameOwner");
    msg << name;
    auto reply = msg.call(usec);

    if (reply.type() == dbus::MessageType::Reply) {
        std::string ownerName;
        reply >> ownerName;
        return ownerName;
    }
    return {};
}

std::unique_ptr<Slot> Bus::serviceOwnerAsync(const std::string &name,
                                             uint64_t usec,
                                             MessageCallback callback) {
    auto msg = createMethodCall("org.freedesktop.DBus", "/org/freedesktop/DBus",
                                "org.freedesktop.DBus", "GetNameOwner");
    msg << name;
    return msg.callAsync(usec, std::move(callback));
}

std::string Bus::uniqueName() {
    FCITX_D();
    const char *name = dbus_bus_get_unique_name(d->conn_.get());
    if (!name) {
        return {};
    }
    return name;
}

std::string Bus::address() {
    FCITX_D();
    return d->address_;
}

void Bus::flush() {
    FCITX_D();
    dbus_connection_flush(d->conn_.get());
}
} // namespace fcitx::dbus
