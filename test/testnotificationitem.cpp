/*
 * SPDX-FileCopyrightText: 2026 myukitty
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include <cstdint>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include <coroutine>
#include "fcitx-utils/coroutine.h"
#include "fcitx-utils/dbus/bus.h"
#include "fcitx-utils/dbus/coroutine.h"
#include "fcitx-utils/dbus/matchrule.h"
#include "fcitx-utils/dbus/message.h"
#include "fcitx-utils/dbus/objectvtable.h"
#include "fcitx-utils/dbus/variant.h"
#include "fcitx-utils/eventdispatcher.h"
#include "fcitx-utils/eventloopinterface.h"
#include "fcitx-utils/i18n.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/macros.h"
#include "fcitx-utils/testing.h"
#include "fcitx/action.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addonloader.h"
#include "fcitx/addonmanager.h"
#include "fcitx/inputcontext.h"
#include "fcitx/inputcontextmanager.h"
#include "fcitx/instance.h"
#include "fcitx/statusarea.h"
#include "fcitx/userinterface.h"
#include "fcitx/userinterfacemanager.h"
#include "notificationitem_public.h"
#include "testdir.h"
#include "testfrontend_public.h"

using namespace fcitx;
FCITX_DEFINE_STATIC_ADDON_REGISTRY(staticAddon);
FCITX_IMPORT_ADDON_FACTORY(staticAddon, keyboard);

namespace {
constexpr char menuInterface[] = "com.canonical.dbusmenu";
using Properties = std::vector<dbus::DictEntry<std::string, dbus::Variant>>;
using Layout =
    dbus::DBusStruct<int32_t, Properties, std::vector<dbus::Variant>>;

class Watcher : public dbus::ObjectVTable<Watcher> {
public:
    std::string service;

private:
    void registerItem(const std::string &name) { service = name; }
    FCITX_OBJECT_VTABLE_METHOD(registerItem, "RegisterStatusNotifierItem", "s",
                               "");
};

struct DispatchBarrier {
    EventDispatcher &dispatcher;
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> continuation) const {
        dispatcher.schedule([continuation]() { continuation.resume(); });
    }
    void await_resume() const noexcept {}
};

Coroutine<Layout> getLayout(dbus::Bus &bus, const std::string &service) {
    auto call = bus.createMethodCall(service.c_str(), "/MenuBar", menuInterface,
                                     "GetLayout");
    call << int32_t(0) << int32_t(-1) << std::vector<std::string>{"label"};
    auto [revision, layout] =
        co_await dbus::AsyncReturn<uint32_t, Layout>(std::move(call));
    FCITX_UNUSED(revision);
    co_return std::move(layout);
}

bool hasLabel(const Layout &layout, const std::string &label) {
    for (const auto &property : std::get<1>(layout)) {
        if (property.key() == "label" &&
            property.value().dataAs<std::string>() == label) {
            return true;
        }
    }
    for (const auto &child : std::get<2>(layout)) {
        if (hasLabel(child.dataAs<Layout>(), label)) {
            return true;
        }
    }
    return false;
}

Coroutine<void> checkMenu(Instance &instance, dbus::Bus &bus,
                          const std::string &service, bool topLevelEvents,
                          bool &passed) {
    int updates = 0;
    auto match = bus.addMatch(
        dbus::MatchRule(service, "/MenuBar", menuInterface, "LayoutUpdated"),
        [&updates](dbus::Message &) {
            ++updates;
            return true;
        });
    FCITX_ASSERT(match);
    auto *frontend = instance.addonManager().addon("testfrontend", true);
    FCITX_ASSERT(frontend);
    instance.inputContextManager().setPropertyPropagatePolicy(
        PropertyPropagatePolicy::No);
    SimpleAction action;
    action.setShortText("Context action");
    FCITX_ASSERT(action.registerAction("notification-test",
                                       &instance.userInterfaceManager()));

    {
        auto uuid =
            frontend->call<ITestFrontend::createInputContext>("testapp");
        auto *ic = instance.inputContextManager().findByUUID(uuid);
        FCITX_ASSERT(ic);
        ic->focusIn();
        ic->statusArea().addAction(StatusGroup::InputMethod, &action);
        ic->updateUserInterface(UserInterfaceComponent::StatusArea, true);
        if (topLevelEvents) {
            auto opened = bus.createMethodCall(service.c_str(), "/MenuBar",
                                               menuInterface, "Event");
            opened << int32_t(0) << std::string("opened")
                   << dbus::Variant(int32_t(0)) << uint32_t(0);
            co_await dbus::AsyncReturn<>(std::move(opened));
        }
        auto show = bus.createMethodCall(service.c_str(), "/MenuBar",
                                         menuInterface, "AboutToShow");
        show << int32_t(0);
        co_await dbus::AsyncReturn<bool>(std::move(show));
        FCITX_ASSERT(
            hasLabel(co_await getLayout(bus, service), "Context action"));

        if (topLevelEvents) {
            // A menu remains pinned to its original context while another
            // context gains focus. Destroying that other context is unrelated.
            SimpleAction otherAction;
            otherAction.setShortText("Other context action");
            FCITX_ASSERT(otherAction.registerAction(
                "notification-other", &instance.userInterfaceManager()));
            auto other =
                frontend->call<ITestFrontend::createInputContext>("other");
            auto *otherIc = instance.inputContextManager().findByUUID(other);
            otherIc->focusIn();
            otherIc->statusArea().addAction(StatusGroup::InputMethod,
                                            &otherAction);
            otherIc->updateUserInterface(UserInterfaceComponent::StatusArea,
                                         true);
            auto pinned = co_await getLayout(bus, service);
            FCITX_ASSERT(hasLabel(pinned, "Context action"));
            FCITX_ASSERT(!hasLabel(pinned, "Other context action"));
            int beforeOther = updates;
            frontend->call<ITestFrontend::destroyInputContext>(other);
            co_await DispatchBarrier{instance.eventDispatcher()};
            FCITX_ASSERT(
                hasLabel(co_await getLayout(bus, service), "Context action"));
            FCITX_ASSERT(updates == beforeOther);
        }

        const int previousUpdates = updates;
        frontend->call<ITestFrontend::destroyInputContext>(uuid);
        FCITX_ASSERT(!instance.mostRecentInputContext());
        co_await DispatchBarrier{instance.eventDispatcher()};
        auto layout = co_await getLayout(bus, service);
        FCITX_ASSERT(!hasLabel(layout, "Context action"));
        FCITX_ASSERT(hasLabel(layout, _("Input Method Settings")));

        // A panel caching the previous layout needs an invalidation, not merely
        // correct data when it happens to request GetLayout again.
        FCITX_ASSERT(updates > previousUpdates)
            << "Missing LayoutUpdated after the last input context was "
               "destroyed";
        FCITX_INFO() << "Context destruction invalidated cached layout; "
                        "top-level events="
                     << topLevelEvents;
        // Creating a new application restores its actions; destroying an
        // unrelated background context must leave them visible.
        auto replacement =
            frontend->call<ITestFrontend::createInputContext>("replacement");
        auto *replacementIc =
            instance.inputContextManager().findByUUID(replacement);
        FCITX_ASSERT(replacementIc);
        replacementIc->statusArea().addAction(StatusGroup::AfterInputMethod,
                                              &action);
        // These actions survive input-method activation. Drain their initial
        // status update while the context is still unfocused.
        FCITX_ASSERT(
            !hasLabel(co_await getLayout(bus, service), "Context action"));
        const int beforeNewContext = updates;
        // A focus event must invalidate the cached empty layout even before
        // a later status-area flush can send an unconditional update.
        replacementIc->focusIn();
        FCITX_ASSERT(
            hasLabel(co_await getLayout(bus, service), "Context action"));
        FCITX_ASSERT(updates > beforeNewContext)
            << "Missing LayoutUpdated for a new fallback context after pin "
               "expiry";
        const int beforeStatusChange = updates;
        action.setShortText("Updated context action");
        replacementIc->updateUserInterface(UserInterfaceComponent::StatusArea,
                                           true);
        FCITX_ASSERT(hasLabel(co_await getLayout(bus, service),
                              "Updated context action"));
        FCITX_ASSERT(updates > beforeStatusChange)
            << "Missing LayoutUpdated for fallback context status changes";
        const int beforeBackground = updates;
        auto background =
            frontend->call<ITestFrontend::createInputContext>("background");
        frontend->call<ITestFrontend::destroyInputContext>(background);
        co_await DispatchBarrier{instance.eventDispatcher()};
        FCITX_ASSERT(hasLabel(co_await getLayout(bus, service),
                              "Updated context action"));
        FCITX_ASSERT(updates == beforeBackground);
        const int beforeReplacement = updates;
        frontend->call<ITestFrontend::destroyInputContext>(replacement);
        co_await DispatchBarrier{instance.eventDispatcher()};
        FCITX_ASSERT(!hasLabel(co_await getLayout(bus, service),
                               "Updated context action"));
        FCITX_ASSERT(updates > beforeReplacement);
    }
    passed = true;
    instance.exit();
}
} // namespace

int main(int argc, char **args) {
    dbus::registerVariantType<Layout>();
    setupTestingEnvironmentPath(FCITX5_BINARY_DIR, {"bin"},
                                {"test", "test/notification-test",
                                 "src/modules",
                                 FCITX5_SOURCE_DIR "/src/modules"});
    char arg0[] = "testnotificationitem";
    char arg1[] = "--disable=all";
    char arg2[] = "--enable=dbus,notificationitem,testfrontend,keyboard";
    char *argv[] = {arg0, arg1, arg2};
    Instance instance(FCITX_ARRAY_SIZE(argv), argv);
    instance.addonManager().registerDefaultLoader(&staticAddon());
    dbus::Bus bus(dbus::BusType::Session);
    FCITX_ASSERT(bus.isOpen());
    bus.attachEventLoop(&instance.eventLoop());
    Watcher watcher;
    FCITX_ASSERT(bus.addObjectVTable("/StatusNotifierWatcher",
                                     "org.kde.StatusNotifierWatcher", watcher));
    FCITX_ASSERT(bus.requestName("org.kde.StatusNotifierWatcher",
                                 dbus::RequestNameFlag::None));
    bool passed = false;
    auto timeout = instance.eventLoop().addTimeEvent(
        CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 10000000, 0,
        [&instance](EventSource *, uint64_t) {
            instance.exit();
            return false;
        });
    std::unique_ptr<HandlerTableEntry<NotificationItemCallback>> registered;
    instance.eventDispatcher().schedule([&]() {
        auto *item = instance.addonManager().addon("notificationitem", true);
        FCITX_ASSERT(item);
        registered = item->call<INotificationItem::watch>([&](bool ready) {
            if (ready) {
                FCITX_ASSERT(!watcher.service.empty());
                sendToEventDispatcherDetached(
                    &instance.eventDispatcher(),
                    checkMenu(instance, bus, watcher.service,
                              argc > 1 && std::string(args[1]) == "kde",
                              passed));
            }
        });
        item->call<INotificationItem::enable>();
    });
    instance.exec();
    FCITX_ASSERT(passed) << "Notification item test did not finish";
}
