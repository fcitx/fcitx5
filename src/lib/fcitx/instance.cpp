/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "config.h"

#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdint>
#include <ctime>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <fmt/format.h>
#include <getopt.h>
#include "fcitx-config/iniparser.h"
#include "fcitx-utils/capabilityflags.h"
#include "fcitx-utils/event.h"
#include "fcitx-utils/eventdispatcher.h"
#include "fcitx-utils/i18n.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/macros.h"
#include "fcitx-utils/misc.h"
#include "fcitx-utils/standardpath.h"
#include "fcitx-utils/stringutils.h"
#include "fcitx-utils/utf8.h"
#include "fcitx/event.h"
#include "fcitx/inputmethodgroup.h"
#include "../../modules/notifications/notifications_public.h"
#include "addonmanager.h"
#include "focusgroup.h"
#include "globalconfig.h"
#include "inputcontextmanager.h"
#include "inputcontextproperty.h"
#include "inputmethodengine.h"
#include "inputmethodentry.h"
#include "inputmethodmanager.h"
#include "instance.h"
#include "instance_p.h"
#include "misc_p.h"
#include "userinterfacemanager.h"

#ifdef ENABLE_X11
#define FCITX_NO_XCB
#include <../modules/xcb/xcb_public.h>
#endif

#ifdef ENABLE_KEYBOARD
#include <xkbcommon/xkbcommon-compose.h>
#include <xkbcommon/xkbcommon.h>
#endif

FCITX_DEFINE_LOG_CATEGORY(keyTrace, "key_trace");

namespace fcitx {

namespace {

constexpr uint64_t AutoSaveMinInUsecs = 60ull * 1000000ull; // 30 minutes
constexpr uint64_t AutoSaveIdleTime = 60ull * 1000000ull;   // 1 minutes

FCITX_CONFIGURATION(DefaultInputMethod,
                    Option<std::vector<std::string>> defaultInputMethods{
                        this, "DefaultInputMethod", "DefaultInputMethod"};
                    Option<std::vector<std::string>> extraLayouts{
                        this, "ExtraLayout", "ExtraLayout"};);

void initAsDaemon() {
    pid_t pid;
    if ((pid = fork()) > 0) {
        waitpid(pid, nullptr, 0);
        exit(0);
    }
    setsid();
    auto oldint = signal(SIGINT, SIG_IGN);
    auto oldhup = signal(SIGHUP, SIG_IGN);
    auto oldquit = signal(SIGQUIT, SIG_IGN);
    auto oldpipe = signal(SIGPIPE, SIG_IGN);
    auto oldttou = signal(SIGTTOU, SIG_IGN);
    auto oldttin = signal(SIGTTIN, SIG_IGN);
    auto oldchld = signal(SIGCHLD, SIG_IGN);
    if (fork() > 0) {
        exit(0);
    }
    (void)chdir("/");

    signal(SIGINT, oldint);
    signal(SIGHUP, oldhup);
    signal(SIGQUIT, oldquit);
    signal(SIGPIPE, oldpipe);
    signal(SIGTTOU, oldttou);
    signal(SIGTTIN, oldttin);
    signal(SIGCHLD, oldchld);
}

// Switch IM when these capabilities change.
bool shouldSwitchIM(const CapabilityFlags &oldFlags,
                    const CapabilityFlags &newFlags) {
    const bool oldDisable = oldFlags.testAny(
        CapabilityFlags{CapabilityFlag::Password, CapabilityFlag::Disable});
    const bool newDisable = newFlags.testAny(
        CapabilityFlags{CapabilityFlag::Password, CapabilityFlag::Disable});
    return oldDisable != newDisable;
}

} // namespace

void InstanceArgument::printUsage() const {
    std::cout
        << "Usage: " << argv0 << " [Option]\n"
        << "  --disable <addon names>\tA comma separated list of addons to "
           "be disabled.\n"
        << "\t\t\t\t\"all\" can be used to disable all addons.\n"
        << "  --enable <addon names>\tA comma separated list of addons to "
           "be enabled.\n"
        << "\t\t\t\t\"all\" can be used to enable all addons.\n"
        << "\t\t\t\tThis value will override the value in the flag "
           "--disable.\n"
        << "  --verbose <logging rule>\tSet the logging rule for "
           "displaying message.\n"
        << "\t\t\t\tSyntax: category1=level1,category2=level2, ...\n"
        << "\t\t\t\tE.g. default=4,key_trace=5\n"
        << "\t\t\t\tLevels are numbers ranging from 0 to 5.\n"
        << "\t\t\t\t\t0 - NoLog\n"
        << "\t\t\t\t\t1 - Fatal\n"
        << "\t\t\t\t\t2 - Error\n"
        << "\t\t\t\t\t3 - Warn\n"
        << "\t\t\t\t\t4 - Info (default)\n"
        << "\t\t\t\t\t5 - Debug\n"
        << "\t\t\t\tSome built-in categories are:\n"
        << "\t\t\t\t\tdefault - miscellaneous category used by fcitx own "
           "library.\n"
        << "\t\t\t\t\tkey_trace - print the key event received by fcitx.\n"
        << "\t\t\t\t\t\"*\" may be used to represent all logging "
           "category.\n"
        << "  -u, --ui <addon name>\t\tSet the UI addon to be used.\n"
        << "  -d\t\t\t\tRun as a daemon.\n"
        << "  -D\t\t\t\tDo not run as a daemon (default).\n"
        << "  -s <seconds>\t\t\tNumber of seconds to wait before start.\n"
        << "  -k, --keep\t\t\tKeep running even the main display is "
           "disconnected.\n"
        << "  -r, --replace\t\t\tReplace the existing instance.\n"
        << "  -o --option <option>\t\tPass the option to addons\n"
        << "\t\t\t\t<option> is in format like:\n"
        << "\t\t\t\tname1=opt1a:opt1b,name2=opt2a:opt2b... .\n"
        << "  -v, --version\t\t\tShow version and quit.\n"
        << "  -h, --help\t\t\tShow this help message and quit.\n";
}

InstancePrivate::InstancePrivate(Instance *q) : QPtrHolder<Instance>(q) {

    const char *locale = getenv("LC_ALL");
    if (!locale) {
        locale = getenv("LC_CTYPE");
    }
    if (!locale) {
        locale = getenv("LANG");
    }
    if (!locale) {
        locale = "C";
    }
#ifdef ENABLE_KEYBOARD
    xkbContext_.reset(xkb_context_new(XKB_CONTEXT_NO_FLAGS));
    if (xkbContext_) {
        xkb_context_set_log_level(xkbContext_.get(), XKB_LOG_LEVEL_CRITICAL);
        xkbComposeTable_.reset(xkb_compose_table_new_from_locale(
            xkbContext_.get(), locale, XKB_COMPOSE_COMPILE_NO_FLAGS));
        if (!xkbComposeTable_) {
            FCITX_INFO()
                << "Trying to fallback to compose table for en_US.UTF-8";
            xkbComposeTable_.reset(xkb_compose_table_new_from_locale(
                xkbContext_.get(), "en_US.UTF-8",
                XKB_COMPOSE_COMPILE_NO_FLAGS));
        }
        if (!xkbComposeTable_) {
            FCITX_WARN()
                << "No compose table is loaded, you may want to check your "
                   "locale settings.";
        }
    }
#endif
}

std::unique_ptr<HandlerTableEntry<EventHandler>>
InstancePrivate::watchEvent(EventType type, EventWatcherPhase phase,
                            EventHandler callback) {
    return eventHandlers_[type][phase].add(std::move(callback));
}

#ifdef ENABLE_KEYBOARD
xkb_keymap *InstancePrivate::keymap(const std::string &display,
                                    const std::string &layout,
                                    const std::string &variant) {
    auto layoutAndVariant = stringutils::concat(layout, "-", variant);
    if (auto *keymapPtr = findValue(keymapCache_[display], layoutAndVariant)) {
        return (*keymapPtr).get();
    }
    struct xkb_rule_names names;
    names.layout = layout.c_str();
    names.variant = variant.c_str();
    std::tuple<std::string, std::string, std::string> xkbParam;
    if (auto *param = findValue(xkbParams_, display)) {
        xkbParam = *param;
    } else {
        if (!xkbParams_.empty()) {
            xkbParam = xkbParams_.begin()->second;
        } else {
            xkbParam = std::make_tuple(DEFAULT_XKB_RULES, "pc101", "");
        }
    }
    if (globalConfig_.overrideXkbOption()) {
        std::get<2>(xkbParam) = globalConfig_.customXkbOption();
    }
    names.rules = std::get<0>(xkbParam).c_str();
    names.model = std::get<1>(xkbParam).c_str();
    names.options = std::get<2>(xkbParam).c_str();
    UniqueCPtr<xkb_keymap, xkb_keymap_unref> keymap(xkb_keymap_new_from_names(
        xkbContext_.get(), &names, XKB_KEYMAP_COMPILE_NO_FLAGS));
    auto result =
        keymapCache_[display].emplace(layoutAndVariant, std::move(keymap));
    assert(result.second);
    return result.first->second.get();
}
#endif

std::pair<std::unordered_set<std::string>, std::unordered_set<std::string>>
InstancePrivate::overrideAddons() {
    std::unordered_set<std::string> enabled;
    std::unordered_set<std::string> disabled;
    for (const auto &addon : globalConfig_.enabledAddons()) {
        enabled.insert(addon);
    }
    for (const auto &addon : globalConfig_.disabledAddons()) {
        enabled.erase(addon);
        disabled.insert(addon);
    }
    for (auto &addon : arg_.enableList) {
        disabled.erase(addon);
        enabled.insert(addon);
    }
    for (auto &addon : arg_.disableList) {
        enabled.erase(addon);
        disabled.insert(addon);
    }
    return {enabled, disabled};
}

void InstancePrivate::buildDefaultGroup() {
    /// Figure out XKB layout information from system.
    auto *defaultGroup = q_func()->defaultFocusGroup();
    bool infoFound = false;
    std::string layouts, variants;
    auto guessLayout = [this, &layouts, &variants,
                        &infoFound](FocusGroup *focusGroup) {
        // For now we can only do this on X11.
        if (!stringutils::startsWith(focusGroup->display(), "x11:")) {
            return true;
        }
#ifdef ENABLE_X11
        auto *xcb = addonManager_.addon("xcb");
        auto x11Name = focusGroup->display().substr(4);
        if (xcb) {
            auto rules = xcb->call<IXCBModule::xkbRulesNames>(x11Name);
            if (!rules[2].empty()) {
                layouts = rules[2];
                variants = rules[3];
                infoFound = true;
                return false;
            }
        }
#else
        FCITX_UNUSED(this);
        FCITX_UNUSED(layouts);
        FCITX_UNUSED(variants);
        FCITX_UNUSED(infoFound);
#endif
        return true;
    };
    if (!defaultGroup || guessLayout(defaultGroup)) {
        icManager_.foreachGroup(
            [defaultGroup, &guessLayout](FocusGroup *focusGroup) {
                if (defaultGroup == focusGroup) {
                    return true;
                }
                return guessLayout(focusGroup);
            });
    }
    if (!infoFound) {
        layouts = "us";
        variants = "";
    }

    // layouts and variants are comma separated list for layout information.
    constexpr char imNamePrefix[] = "keyboard-";
    auto layoutTokens =
        stringutils::split(layouts, ",", stringutils::SplitBehavior::KeepEmpty);
    auto variantTokens = stringutils::split(
        variants, ",", stringutils::SplitBehavior::KeepEmpty);
    auto size = std::max(layoutTokens.size(), variantTokens.size());
    // Make sure we have token to be the same size.
    layoutTokens.resize(size);
    variantTokens.resize(size);

    OrderedSet<std::string> imLayouts;
    for (decltype(size) i = 0; i < size; i++) {
        if (layoutTokens[i].empty()) {
            continue;
        }
        std::string layoutName = layoutTokens[i];
        if (!variantTokens[i].empty()) {
            layoutName = stringutils::concat(layoutName, "-", variantTokens[i]);
        }

        // Skip the layout if we don't have it.
        if (!imManager_.entry(stringutils::concat(imNamePrefix, layoutName))) {
            continue;
        }
        // Avoid add duplicate entry. layout might have weird duplicate.
        imLayouts.pushBack(layoutName);
    }

    // Load the default profile.
    auto lang = stripLanguage(getCurrentLanguage());
    auto defaultProfile = StandardPath::global().open(
        StandardPath::Type::PkgData, stringutils::joinPath("default", lang),
        O_RDONLY);

    RawConfig config;
    DefaultInputMethod defaultIMConfig;
    readFromIni(config, defaultProfile.fd());
    defaultIMConfig.load(config);

    // Add extra layout from profile.
    for (const auto &extraLayout : defaultIMConfig.extraLayouts.value()) {
        if (!imManager_.entry(stringutils::concat(imNamePrefix, extraLayout))) {
            continue;
        }
        imLayouts.pushBack(extraLayout);
    }

    // Make sure imLayouts is not empty.
    if (imLayouts.empty()) {
        imLayouts.pushBack("us");
    }

    // Figure out the first available default input method.
    std::string defaultIM;
    for (const auto &im : defaultIMConfig.defaultInputMethods.value()) {
        if (imManager_.entry(im)) {
            defaultIM = im;
            break;
        }
    }

    // Create a group for each layout.
    std::vector<std::string> groupOrders;
    for (const auto &imLayout : imLayouts) {
        std::string groupName;
        if (imLayouts.size() == 1) {
            groupName = _("Default");
        } else {
            groupName = fmt::format(_("Group {}"), imManager_.groupCount() + 1);
        }
        imManager_.addEmptyGroup(groupName);
        groupOrders.push_back(groupName);
        InputMethodGroup group(groupName);
        group.inputMethodList().emplace_back(
            InputMethodGroupItem(stringutils::concat(imNamePrefix, imLayout)));
        if (!defaultIM.empty()) {
            group.inputMethodList().emplace_back(
                InputMethodGroupItem(defaultIM));
        }
        FCITX_INFO() << "Items in " << groupName << ": "
                     << group.inputMethodList();
        group.setDefaultLayout(imLayout);
        imManager_.setGroup(std::move(group));
    }
    FCITX_INFO() << "Generated groups: " << groupOrders;
    imManager_.setGroupOrder(groupOrders);
}

void InstancePrivate::showInputMethodInformation(InputContext *ic) {
    FCITX_Q();
    auto *inputState = ic->propertyFor(&inputStateFactory_);
    auto *engine = q->inputMethodEngine(ic);
    const auto *entry = q->inputMethodEntry(ic);
    auto &imManager = q->inputMethodManager();

    if (!inputState->isActive() &&
        !globalConfig_.showFirstInputMethodInformation()) {
        return;
    }

    std::string display;
    if (engine) {
        auto subMode = engine->subMode(*entry, *ic);
        auto subModeLabel = engine->subModeLabel(*entry, *ic);
        auto name = globalConfig_.compactInputMethodInformation() &&
                            !entry->label().empty()
                        ? entry->label()
                        : entry->name();
        if (globalConfig_.compactInputMethodInformation() &&
            !subModeLabel.empty()) {
            display = std::move(subModeLabel);
        } else if (subMode.empty()) {
            display = std::move(name);
        } else {
            display = fmt::format(_("{0} ({1})"), name, subMode);
        }
    } else if (entry) {
        display = fmt::format(_("{0} (Not available)"), entry->name());
    } else {
        display = _("(Not available)");
    }
    if (!globalConfig_.compactInputMethodInformation() &&
        imManager.groupCount() > 1) {
        display = fmt::format(_("Group {0}: {1}"),
                              imManager.currentGroup().name(), display);
    }
    inputState->showInputMethodInformation(display);
}

bool InstancePrivate::canActivate(InputContext *ic) {
    FCITX_Q();
    if (!q->canTrigger()) {
        return false;
    }
    auto *inputState = ic->propertyFor(&inputStateFactory_);
    return !inputState->isActive();
}

bool InstancePrivate::canDeactivate(InputContext *ic) {
    FCITX_Q();
    if (!q->canTrigger()) {
        return false;
    }
    auto *inputState = ic->propertyFor(&inputStateFactory_);
    return inputState->isActive();
}

void InstancePrivate::navigateGroup(InputContext *ic, const Key &key,
                                    bool forward) {
    auto *inputState = ic->propertyFor(&inputStateFactory_);
    inputState->pendingGroupIndex_ =
        (inputState->pendingGroupIndex_ +
         (forward ? 1 : imManager_.groupCount() - 1)) %
        imManager_.groupCount();
    FCITX_DEBUG() << "Switch to group " << inputState->pendingGroupIndex_;

    if (notifications_ && !isSingleKey(key)) {
        notifications_->call<INotifications::showTip>(
            "enumerate-group", _("Input Method"), "input-keyboard",
            _("Switch group"),
            fmt::format(_("Switch group to {0}"),
                        imManager_.groups()[inputState->pendingGroupIndex_]),
            3000);
    }
}

void InstancePrivate::acceptGroupChange(const Key &key, InputContext *ic) {
    FCITX_DEBUG() << "Accept group change, isSingleKey: " << key;

    auto *inputState = ic->propertyFor(&inputStateFactory_);
    auto groups = imManager_.groups();
    if (groups.size() > inputState->pendingGroupIndex_) {
        if (isSingleKey(key)) {
            FCITX_DEBUG() << "EnumerateGroupTo: "
                          << inputState->pendingGroupIndex_ << " " << key;
            imManager_.enumerateGroupTo(groups[inputState->pendingGroupIndex_]);
        } else {
            FCITX_DEBUG() << "SetCurrentGroup: "
                          << inputState->pendingGroupIndex_ << " " << key;
            imManager_.setCurrentGroup(groups[inputState->pendingGroupIndex_]);
        }
    }
    inputState->pendingGroupIndex_ = 0;
}

InputState::InputState(InstancePrivate *d, InputContext *ic)
    : d_ptr(d), ic_(ic) {
    active_ = d->globalConfig_.activeByDefault();
#ifdef ENABLE_KEYBOARD
    if (d->xkbComposeTable_) {
        xkbComposeState_.reset(xkb_compose_state_new(
            d->xkbComposeTable_.get(), XKB_COMPOSE_STATE_NO_FLAGS));
    }
#endif
}

void InputState::showInputMethodInformation(const std::string &name) {
    ic_->inputPanel().setAuxUp(Text(name));
    ic_->updateUserInterface(UserInterfaceComponent::InputPanel);
    lastInfo_ = name;
    imInfoTimer_ = d_ptr->eventLoop_.addTimeEvent(
        CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 1000000, 0,
        [this](EventSourceTime *, uint64_t) {
            hideInputMethodInfo();
            return true;
        });
}

#ifdef ENABLE_KEYBOARD
xkb_state *InputState::customXkbState(bool refresh) {
    auto *instance = d_ptr->q_func();
    const InputMethodGroup &group = d_ptr->imManager_.currentGroup();
    const auto im = instance->inputMethod(ic_);
    auto layout = group.layoutFor(im);
    if (layout.empty() && stringutils::startsWith(im, "keyboard-")) {
        layout = im.substr(9);
    }
    if (layout.empty() || layout == group.defaultLayout()) {
        // Use system one.
        xkbState_.reset();
        modsAllReleased_ = false;
        lastXkbLayout_.clear();
        return nullptr;
    }

    if (layout == lastXkbLayout_ && !refresh) {
        return xkbState_.get();
    }

    lastXkbLayout_ = layout;
    const auto layoutAndVariant = parseLayout(layout);
    if (auto *keymap = d_ptr->keymap(ic_->display(), layoutAndVariant.first,
                                     layoutAndVariant.second)) {
        xkbState_.reset(xkb_state_new(keymap));
    } else {
        xkbState_.reset();
    }
    modsAllReleased_ = false;
    return xkbState_.get();
}
#endif

void InputState::setActive(bool active) {
    if (active_ != active) {
        active_ = active;
        ic_->updateProperty(&d_ptr->inputStateFactory_);
    }
}

void InputState::setLocalIM(const std::string &localIM) {
    if (localIM_ != localIM) {
        localIM_ = localIM;
        ic_->updateProperty(&d_ptr->inputStateFactory_);
    }
}

void InputState::copyTo(InputContextProperty *other) {
    auto *otherState = static_cast<InputState *>(other);
    if (otherState->active_ == active_ && otherState->localIM_ == localIM_) {
        return;
    }

    if (otherState->ic_->hasFocus()) {
        FCITX_DEBUG() << "Sync state to focused ic: "
                      << otherState->ic_->program();
        CheckInputMethodChanged imChangedRAII(otherState->ic_, d_ptr);
        otherState->active_ = active_;
        otherState->localIM_ = localIM_;
    } else {
        otherState->active_ = active_;
        otherState->localIM_ = localIM_;
    }
}

void InputState::reset() {
#ifdef ENABLE_KEYBOARD
    if (xkbComposeState_) {
        xkb_compose_state_reset(xkbComposeState_.get());
    }
#endif
    pendingGroupIndex_ = 0;
    keyReleased_ = -1;
    lastKeyPressed_ = Key();
    totallyReleased_ = true;
}

void InputState::hideInputMethodInfo() {
    if (!imInfoTimer_) {
        return;
    }
    imInfoTimer_.reset();
    auto &panel = ic_->inputPanel();
    if (panel.auxDown().empty() && panel.preedit().empty() &&
        panel.clientPreedit().empty() &&
        (!panel.candidateList() || panel.candidateList()->empty()) &&
        panel.auxUp().size() == 1 && panel.auxUp().stringAt(0) == lastInfo_) {
        panel.reset();
        ic_->updateUserInterface(UserInterfaceComponent::InputPanel);
    }
}

#ifdef ENABLE_KEYBOARD
void InputState::resetXkbState() {
    lastXkbLayout_.clear();
    xkbState_.reset();
}
#endif

CheckInputMethodChanged::CheckInputMethodChanged(InputContext *ic,
                                                 InstancePrivate *instance)
    : instance_(instance->q_func()), instancePrivate_(instance),
      ic_(ic->watch()), inputMethod_(instance_->inputMethod(ic)),
      reason_(InputMethodSwitchedReason::Other) {
    auto *inputState = ic->propertyFor(&instance->inputStateFactory_);
    if (!inputState->imChanged_) {
        inputState->imChanged_ = this;
    } else {
        ic_.unwatch();
    }
}

CheckInputMethodChanged::~CheckInputMethodChanged() {
    if (!ic_.isValid()) {
        return;
    }
    auto *ic = ic_.get();
    auto *inputState = ic->propertyFor(&instancePrivate_->inputStateFactory_);
    inputState->imChanged_ = nullptr;
    if (inputMethod_ != instance_->inputMethod(ic) && !ignore_) {
        instance_->postEvent(
            InputContextSwitchInputMethodEvent(reason_, inputMethod_, ic));
    }
}

Instance::Instance(int argc, char **argv) {
    InstanceArgument arg;
    arg.parseOption(argc, argv);
    if (arg.quietQuit) {
        throw InstanceQuietQuit();
    }

    if (arg.runAsDaemon) {
        initAsDaemon();
    }

    if (arg.overrideDelay > 0) {
        sleep(arg.overrideDelay);
    }

    // we need fork before this
    d_ptr = std::make_unique<InstancePrivate>(this);
    FCITX_D();
    d->arg_ = arg;
    d->eventDispatcher_.attach(&d->eventLoop_);
    d->addonManager_.setInstance(this);
    d->addonManager_.setAddonOptions(arg.addonOptions_);
    d->icManager_.setInstance(this);
    d->connections_.emplace_back(
        d->imManager_.connect<InputMethodManager::CurrentGroupAboutToChange>(
            [this, d](const std::string &lastGroup) {
                d->icManager_.foreachFocused([this](InputContext *ic) {
                    assert(ic->hasFocus());
                    InputContextSwitchInputMethodEvent event(
                        InputMethodSwitchedReason::GroupChange, inputMethod(ic),
                        ic);
                    deactivateInputMethod(event);
                    return true;
                });
                d->lastGroup_ = lastGroup;
                postEvent(InputMethodGroupAboutToChangeEvent());
            }));
    d->connections_.emplace_back(
        d->imManager_.connect<InputMethodManager::CurrentGroupChanged>(
            [this, d](const std::string &newGroup) {
                d->icManager_.foreachFocused([this](InputContext *ic) {
                    assert(ic->hasFocus());
                    InputContextSwitchInputMethodEvent event(
                        InputMethodSwitchedReason::GroupChange, "", ic);
                    activateInputMethod(event);
                    return true;
                });
                postEvent(InputMethodGroupChangedEvent());
                if (!d->lastGroup_.empty() && !newGroup.empty() &&
                    d->lastGroup_ != newGroup && d->notifications_ &&
                    d->imManager_.groupCount() > 1) {
                    d->notifications_->call<INotifications::showTip>(
                        "enumerate-group", _("Input Method"), "input-keyboard",
                        _("Switch group"),
                        fmt::format(_("Switched group to {0}"),
                                    d->imManager_.currentGroup().name()),
                        3000);
                }
                d->lastGroup_ = newGroup;
            }));

    d->eventWatchers_.emplace_back(d->watchEvent(
        EventType::InputContextCapabilityAboutToChange,
        EventWatcherPhase::ReservedFirst, [this](Event &event) {
            auto &capChanged =
                static_cast<CapabilityAboutToChangeEvent &>(event);
            if (!capChanged.inputContext()->hasFocus()) {
                return;
            }

            if (!shouldSwitchIM(capChanged.oldFlags(), capChanged.newFlags())) {
                return;
            }

            InputContextSwitchInputMethodEvent switchIM(
                InputMethodSwitchedReason::CapabilityChanged,
                inputMethod(capChanged.inputContext()),
                capChanged.inputContext());
            deactivateInputMethod(switchIM);
        }));
    d->eventWatchers_.emplace_back(d->watchEvent(
        EventType::InputContextCapabilityChanged,
        EventWatcherPhase::ReservedFirst, [this](Event &event) {
            auto &capChanged = static_cast<CapabilityChangedEvent &>(event);
            if (!capChanged.inputContext()->hasFocus()) {
                return;
            }

            if (!shouldSwitchIM(capChanged.oldFlags(), capChanged.newFlags())) {
                return;
            }

            InputContextSwitchInputMethodEvent switchIM(
                InputMethodSwitchedReason::CapabilityChanged, "",
                capChanged.inputContext());
            activateInputMethod(switchIM);
        }));

    d->eventWatchers_.emplace_back(watchEvent(
        EventType::InputContextKeyEvent, EventWatcherPhase::InputMethod,
        [this, d](Event &event) {
            auto &keyEvent = static_cast<KeyEvent &>(event);
            auto *ic = keyEvent.inputContext();
            CheckInputMethodChanged imChangedRAII(ic, d);
            auto origKey = keyEvent.origKey().normalize();

            struct {
                const KeyList &list;
                std::function<bool()> check;
                std::function<void(bool)> trigger;
            } keyHandlers[] = {
                {d->globalConfig_.triggerKeys(),
                 [this]() { return canTrigger(); },
                 [this, ic](bool totallyReleased) {
                     return trigger(ic, totallyReleased);
                 }},
                {d->globalConfig_.altTriggerKeys(),
                 [this, ic]() { return canAltTrigger(ic); },
                 [this, ic](bool) { return altTrigger(ic); }},
                {d->globalConfig_.activateKeys(),
                 [ic, d]() { return d->canActivate(ic); },
                 [this, ic](bool) { return activate(ic); }},
                {d->globalConfig_.deactivateKeys(),
                 [ic, d]() { return d->canDeactivate(ic); },
                 [this, ic](bool) { return deactivate(ic); }},
                {d->globalConfig_.enumerateForwardKeys(),
                 [this, ic]() { return canEnumerate(ic); },
                 [this, ic](bool) { return enumerate(ic, true); }},
                {d->globalConfig_.enumerateBackwardKeys(),
                 [this, ic]() { return canEnumerate(ic); },
                 [this, ic](bool) { return enumerate(ic, false); }},
                {d->globalConfig_.enumerateGroupForwardKeys(),
                 [this]() { return canChangeGroup(); },
                 [ic, d, origKey](bool) {
                     return d->navigateGroup(ic, origKey, true);
                 }},
                {d->globalConfig_.enumerateGroupBackwardKeys(),
                 [this]() { return canChangeGroup(); },
                 [ic, d, origKey](bool) {
                     return d->navigateGroup(ic, origKey, false);
                 }},
            };

            auto *inputState = ic->propertyFor(&d->inputStateFactory_);
            int keyReleased = inputState->keyReleased_;
            Key lastKeyPressed = inputState->lastKeyPressed_;
            // Keep this value, and reset them in the state
            inputState->keyReleased_ = -1;
            const bool isModifier = origKey.isModifier();
            if (keyEvent.isRelease()) {
                int idx = 0;
                for (auto &keyHandler : keyHandlers) {
                    if (keyReleased == idx &&
                        origKey.isReleaseOfModifier(lastKeyPressed) &&
                        keyHandler.check()) {
                        if (isModifier) {
                            keyHandler.trigger(inputState->totallyReleased_);
                            if (origKey.hasModifier()) {
                                inputState->totallyReleased_ = false;
                            }
                        }
                        keyEvent.filter();
                        break;
                    }
                    idx++;
                }
                if (isSingleModifier(origKey)) {
                    inputState->totallyReleased_ = true;
                }
            }

            if (inputState->pendingGroupIndex_ &&
                inputState->totallyReleased_) {
                auto *inputState = ic->propertyFor(&d->inputStateFactory_);
                if (inputState->imChanged_) {
                    inputState->imChanged_->ignore();
                }
                d->acceptGroupChange(lastKeyPressed, ic);
                inputState->lastKeyPressed_ = Key();
            }

            if (!keyEvent.filtered() && !keyEvent.isRelease()) {
                int idx = 0;
                for (auto &keyHandler : keyHandlers) {
                    auto keyIdx = origKey.keyListIndex(keyHandler.list);
                    if (keyIdx >= 0 && keyHandler.check()) {
                        inputState->keyReleased_ = idx;
                        inputState->lastKeyPressed_ = origKey;
                        if (isModifier) {
                            // don't forward to input method, but make it pass
                            // through to client.
                            return keyEvent.filter();
                        }
                        keyHandler.trigger(inputState->totallyReleased_);
                        if (origKey.hasModifier()) {
                            inputState->totallyReleased_ = false;
                        }
                        return keyEvent.filterAndAccept();
                    }
                    idx++;
                }
            }
        }));
    d->eventWatchers_.emplace_back(watchEvent(
        EventType::InputContextKeyEvent, EventWatcherPhase::PreInputMethod,
        [d](Event &event) {
            auto &keyEvent = static_cast<KeyEvent &>(event);
            auto *ic = keyEvent.inputContext();
            if (!keyEvent.isRelease() &&
                keyEvent.key().checkKeyList(
                    d->globalConfig_.togglePreeditKeys())) {
                ic->setEnablePreedit(!ic->isPreeditEnabled());
                if (d->notifications_) {
                    d->notifications_->call<INotifications::showTip>(
                        "toggle-preedit", _("Input Method"), "", _("Preedit"),
                        ic->isPreeditEnabled() ? _("Preedit enabled")
                                               : _("Preedit disabled"),
                        3000);
                }
                keyEvent.filterAndAccept();
            }
        }));
    d->eventWatchers_.emplace_back(d->watchEvent(
        EventType::InputContextKeyEvent, EventWatcherPhase::ReservedFirst,
        [d](Event &event) {
            // Update auto save.
            d->idleStartTimestamp_ = now(CLOCK_MONOTONIC);
            auto &keyEvent = static_cast<KeyEvent &>(event);
            auto *ic = keyEvent.inputContext();
            auto *inputState = ic->propertyFor(&d->inputStateFactory_);
#ifdef ENABLE_KEYBOARD
            auto *xkbState = inputState->customXkbState();
            if (xkbState) {
                if (auto *mods = findValue(d->stateMask_, ic->display())) {
                    FCITX_KEYTRACE() << "Update mask to customXkbState";
                    // Keep latched, but propagate depressed optionally and
                    // locked.
                    uint32_t depressed;
                    if (inputState->isModsAllReleased()) {
                        depressed = xkb_state_serialize_mods(
                            xkbState, XKB_STATE_MODS_DEPRESSED);
                    } else {
                        depressed = std::get<0>(*mods);
                    }
                    if (std::get<0>(*mods) == 0) {
                        inputState->setModsAllReleased();
                    }
                    auto latched = xkb_state_serialize_mods(
                        xkbState, XKB_STATE_MODS_LATCHED);
                    auto locked = std::get<2>(*mods);

                    // set modifiers in depressed if they don't appear in any of
                    // the final masks
                    // depressed |= ~(depressed | latched | locked);
                    FCITX_DEBUG()
                        << depressed << " " << latched << " " << locked;
                    xkb_state_update_mask(xkbState, depressed, latched, locked,
                                          0, 0, 0);
                }
                const uint32_t effective = xkb_state_serialize_mods(
                    xkbState, XKB_STATE_MODS_EFFECTIVE);
                auto newSym = xkb_state_key_get_one_sym(
                    xkbState, keyEvent.rawKey().code());
                auto newModifier = KeyStates(effective);
                auto keymap = xkb_state_get_keymap(xkbState);
                if (keyEvent.rawKey().states().test(KeyState::Repeat) &&
                    xkb_keymap_key_repeats(keymap, keyEvent.rawKey().code())) {
                    newModifier |= KeyState::Repeat;
                }

                const uint32_t modsDepressed = xkb_state_serialize_mods(
                    xkbState, XKB_STATE_MODS_DEPRESSED);
                const uint32_t modsLatched =
                    xkb_state_serialize_mods(xkbState, XKB_STATE_MODS_LATCHED);
                const uint32_t modsLocked =
                    xkb_state_serialize_mods(xkbState, XKB_STATE_MODS_LOCKED);
                FCITX_KEYTRACE() << "Current mods: " << modsDepressed << " "
                                 << modsLatched << " " << modsLocked;
                auto newCode = keyEvent.rawKey().code();
                Key key(static_cast<KeySym>(newSym), newModifier, newCode);
                FCITX_KEYTRACE()
                    << "Custom Xkb translated Key: " << key.toString();
                keyEvent.setRawKey(key);
            }
#endif
            FCITX_KEYTRACE() << "KeyEvent: " << keyEvent.key()
                             << " rawKey: " << keyEvent.rawKey()
                             << " origKey: " << keyEvent.origKey()
                             << " Release:" << keyEvent.isRelease()
                             << " keycode: " << keyEvent.origKey().code();

            if (keyEvent.isRelease()) {
                return;
            }
            inputState->hideInputMethodInfo();
        }));
    d->eventWatchers_.emplace_back(
        watchEvent(EventType::InputContextKeyEvent,
                   EventWatcherPhase::InputMethod, [this](Event &event) {
                       auto &keyEvent = static_cast<KeyEvent &>(event);
                       auto *ic = keyEvent.inputContext();
                       auto *engine = inputMethodEngine(ic);
                       const auto *entry = inputMethodEntry(ic);
                       if (!engine || !entry) {
                           return;
                       }
                       engine->keyEvent(*entry, keyEvent);
                   }));
    d->eventWatchers_.emplace_back(watchEvent(
        EventType::InputContextVirtualKeyboardEvent,
        EventWatcherPhase::InputMethod, [this](Event &event) {
            auto &keyEvent = static_cast<VirtualKeyboardEvent &>(event);
            auto *ic = keyEvent.inputContext();
            auto *engine = inputMethodEngine(ic);
            const auto *entry = inputMethodEntry(ic);
            if (!engine || !entry) {
                return;
            }
            engine->virtualKeyboardEvent(*entry, keyEvent);
        }));
    d->eventWatchers_.emplace_back(watchEvent(
        EventType::InputContextInvokeAction, EventWatcherPhase::InputMethod,
        [this](Event &event) {
            auto &invokeActionEvent = static_cast<InvokeActionEvent &>(event);
            auto *ic = invokeActionEvent.inputContext();
            auto *engine = inputMethodEngine(ic);
            const auto *entry = inputMethodEntry(ic);
            if (!engine || !entry) {
                return;
            }
            engine->invokeAction(*entry, invokeActionEvent);
        }));
    d->eventWatchers_.emplace_back(d->watchEvent(
        EventType::InputContextKeyEvent, EventWatcherPhase::ReservedLast,
        [this](Event &event) {
            auto &keyEvent = static_cast<KeyEvent &>(event);
            auto *ic = keyEvent.inputContext();
            auto *engine = inputMethodEngine(ic);
            const auto *entry = inputMethodEntry(ic);
            if (!engine || !entry) {
                return;
            }
            engine->filterKey(*entry, keyEvent);
            emit<Instance::KeyEventResult>(keyEvent);
#ifdef ENABLE_KEYBOARD
            if (keyEvent.forward()) {
                FCITX_D();
                // Always let the release key go through, since it shouldn't
                // produce character. Otherwise it may wrongly trigger wayland
                // client side repetition.
                if (keyEvent.isRelease()) {
                    keyEvent.filter();
                    return;
                }
                auto *inputState = ic->propertyFor(&d->inputStateFactory_);
                if (auto *xkbState = inputState->customXkbState()) {
                    if (auto utf32 = xkb_state_key_get_utf32(
                            xkbState, keyEvent.key().code())) {
                        // Ignore newline, return, backspace, tab, and delete.
                        if (utf32 == '\n' || utf32 == '\b' || utf32 == '\r' ||
                            utf32 == '\t' || utf32 == '\033' ||
                            utf32 == '\x7f') {
                            return;
                        }
                        if (keyEvent.key().states().test(KeyState::Ctrl) ||
                            keyEvent.rawKey().sym() ==
                                keyEvent.origKey().sym()) {
                            return;
                        }
                        FCITX_KEYTRACE() << "Will commit char: " << utf32;
                        ic->commitString(utf8::UCS4ToUTF8(utf32));
                        keyEvent.filterAndAccept();
                    } else if (!keyEvent.key().states().test(KeyState::Ctrl) &&
                               keyEvent.rawKey().sym() !=
                                   keyEvent.origKey().sym() &&
                               Key::keySymToUnicode(keyEvent.origKey().sym()) !=
                                   0) {
                        // filter key for the case that: origKey will produce
                        // character, while the translated will not.
                        keyEvent.filterAndAccept();
                    }
                }
            }
#endif
        }));
    d->eventWatchers_.emplace_back(d->watchEvent(
        EventType::InputContextFocusIn, EventWatcherPhase::ReservedFirst,
        [this, d](Event &event) {
            auto &icEvent = static_cast<InputContextEvent &>(event);
            auto isSameProgram = [&icEvent, d]() {
                // Check if they are same IC, or they are same program.
                return (icEvent.inputContext() == d->lastUnFocusedIc_.get()) ||
                       (!icEvent.inputContext()->program().empty() &&
                        (icEvent.inputContext()->program() ==
                         d->lastUnFocusedProgram_));
            };

            if (d->globalConfig_.resetStateWhenFocusIn() ==
                    PropertyPropagatePolicy::All ||
                (d->globalConfig_.resetStateWhenFocusIn() ==
                     PropertyPropagatePolicy::Program &&
                 !isSameProgram())) {
                if (d->globalConfig_.activeByDefault()) {
                    activate(icEvent.inputContext());
                } else {
                    deactivate(icEvent.inputContext());
                }
            }

            activateInputMethod(icEvent);

            if (virtualKeyboardAutoShow()) {
                auto *inputContext = icEvent.inputContext();
                if (!inputContext->clientControlVirtualkeyboardShow()) {
                    inputContext->showVirtualKeyboard();
                }
            }

            if (!d->globalConfig_.showInputMethodInformationWhenFocusIn() ||
                icEvent.inputContext()->capabilityFlags().test(
                    CapabilityFlag::Disable)) {
                return;
            }
            // Give some time because the cursor location may need some time
            // to be updated.
            d->focusInImInfoTimer_ = d->eventLoop_.addTimeEvent(
                CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 30000, 0,
                [d, icRef = icEvent.inputContext()->watch()](EventSourceTime *,
                                                             uint64_t) {
                    // Check if ic is still valid and has focus.
                    if (auto *ic = icRef.get(); ic && ic->hasFocus()) {
                        d->showInputMethodInformation(ic);
                    }
                    return true;
                });
        }));
    d->eventWatchers_.emplace_back(d->watchEvent(
        EventType::InputContextFocusOut, EventWatcherPhase::ReservedFirst,
        [d](Event &event) {
            auto &icEvent = static_cast<InputContextEvent &>(event);
            auto *ic = icEvent.inputContext();
            auto *inputState = ic->propertyFor(&d->inputStateFactory_);
            inputState->reset();
            if (!ic->capabilityFlags().test(
                    CapabilityFlag::ClientUnfocusCommit)) {
                // do server side commit
                auto commit =
                    ic->inputPanel().clientPreedit().toStringForCommit();
                if (!commit.empty()) {
                    ic->commitString(commit);
                }
            }
        }));
    d->eventWatchers_.emplace_back(d->watchEvent(
        EventType::InputContextFocusOut, EventWatcherPhase::InputMethod,
        [this, d](Event &event) {
            auto &icEvent = static_cast<InputContextEvent &>(event);
            d->lastUnFocusedProgram_ = icEvent.inputContext()->program();
            d->lastUnFocusedIc_ = icEvent.inputContext()->watch();
            deactivateInputMethod(icEvent);
            if (virtualKeyboardAutoHide()) {
                auto *inputContext = icEvent.inputContext();
                if (!inputContext->clientControlVirtualkeyboardHide()) {
                    inputContext->hideVirtualKeyboard();
                }
            }
        }));
    d->eventWatchers_.emplace_back(d->watchEvent(
        EventType::InputContextReset, EventWatcherPhase::ReservedFirst,
        [d](Event &event) {
            auto &icEvent = static_cast<InputContextEvent &>(event);
            auto *ic = icEvent.inputContext();
            auto *inputState = ic->propertyFor(&d->inputStateFactory_);
            inputState->reset();
        }));
    d->eventWatchers_.emplace_back(
        watchEvent(EventType::InputContextReset, EventWatcherPhase::InputMethod,
                   [this](Event &event) {
                       auto &icEvent = static_cast<InputContextEvent &>(event);
                       auto *ic = icEvent.inputContext();
                       if (!ic->hasFocus()) {
                           return;
                       }
                       auto *engine = inputMethodEngine(ic);
                       const auto *entry = inputMethodEntry(ic);
                       if (!engine || !entry) {
                           return;
                       }
                       engine->reset(*entry, icEvent);
                   }));
    d->eventWatchers_.emplace_back(d->watchEvent(
        EventType::InputContextSwitchInputMethod,
        EventWatcherPhase::ReservedFirst, [this](Event &event) {
            auto &icEvent =
                static_cast<InputContextSwitchInputMethodEvent &>(event);
            auto *ic = icEvent.inputContext();
            if (!ic->hasFocus()) {
                return;
            }
            deactivateInputMethod(icEvent);
            activateInputMethod(icEvent);
        }));
    d->eventWatchers_.emplace_back(d->watchEvent(
        EventType::InputContextSwitchInputMethod,
        EventWatcherPhase::ReservedLast, [this, d](Event &event) {
            auto &icEvent =
                static_cast<InputContextSwitchInputMethodEvent &>(event);
            auto *ic = icEvent.inputContext();
            if (!ic->hasFocus()) {
                return;
            }

            auto *inputState = ic->propertyFor(&d->inputStateFactory_);
            inputState->lastIMChangeIsAltTrigger_ =
                icEvent.reason() == InputMethodSwitchedReason::AltTrigger;

            if ((icEvent.reason() != InputMethodSwitchedReason::Trigger &&
                 icEvent.reason() != InputMethodSwitchedReason::AltTrigger &&
                 icEvent.reason() != InputMethodSwitchedReason::Enumerate &&
                 icEvent.reason() != InputMethodSwitchedReason::Activate &&
                 icEvent.reason() != InputMethodSwitchedReason::Other &&
                 icEvent.reason() != InputMethodSwitchedReason::GroupChange &&
                 icEvent.reason() != InputMethodSwitchedReason::Deactivate)) {
                return;
            }
            showInputMethodInformation(ic);
        }));
    d->eventWatchers_.emplace_back(
        d->watchEvent(EventType::InputMethodGroupChanged,
                      EventWatcherPhase::ReservedLast, [this, d](Event &) {
                          // Use a timer here. so we can get focus back to real
                          // window.
                          d->imGroupInfoTimer_ = d->eventLoop_.addTimeEvent(
                              CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 30000, 0,
                              [this](EventSourceTime *, uint64_t) {
                                  inputContextManager().foreachFocused(
                                      [this](InputContext *ic) {
                                          showInputMethodInformation(ic);
                                          return true;
                                      });
                                  return true;
                              });
                      }));

    d->eventWatchers_.emplace_back(d->watchEvent(
        EventType::InputContextUpdateUI, EventWatcherPhase::ReservedFirst,
        [d](Event &event) {
            auto &icEvent = static_cast<InputContextUpdateUIEvent &>(event);
            if (icEvent.immediate()) {
                d->uiManager_.update(icEvent.component(),
                                     icEvent.inputContext());
                d->uiManager_.flush();
            } else {
                d->uiManager_.update(icEvent.component(),
                                     icEvent.inputContext());
                d->uiUpdateEvent_->setOneShot();
            }
        }));
    d->eventWatchers_.emplace_back(d->watchEvent(
        EventType::InputContextDestroyed, EventWatcherPhase::ReservedFirst,
        [d](Event &event) {
            auto &icEvent = static_cast<InputContextEvent &>(event);
            d->uiManager_.expire(icEvent.inputContext());
        }));
    d->eventWatchers_.emplace_back(d->watchEvent(
        EventType::InputMethodModeChanged, EventWatcherPhase::ReservedFirst,
        [d](Event &) { d->uiManager_.updateAvailability(); }));
    d->uiUpdateEvent_ = d->eventLoop_.addDeferEvent([d](EventSource *) {
        d->uiManager_.flush();
        return true;
    });
    d->uiUpdateEvent_->setEnabled(false);
    d->periodicalSave_ = d->eventLoop_.addTimeEvent(
        CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 1000000, AutoSaveIdleTime,
        [this, d](EventSourceTime *time, uint64_t) {
            if (exiting()) {
                return true;
            }

            // Check if the idle time is long enough.
            auto currentTime = now(CLOCK_MONOTONIC);
            if (currentTime <= d->idleStartTimestamp_ ||
                currentTime - d->idleStartTimestamp_ < AutoSaveIdleTime) {
                // IF not idle, shorten the next checking period.
                time->setNextInterval(2 * AutoSaveIdleTime);
                time->setOneShot();
                return true;
            }

            FCITX_INFO() << "Running autosave...";
            save();
            FCITX_INFO() << "End autosave";
            if (d->globalConfig_.autoSavePeriod() > 0) {
                time->setNextInterval(d->globalConfig_.autoSavePeriod() *
                                      AutoSaveMinInUsecs);
                time->setOneShot();
            }
            return true;
        });
    d->periodicalSave_->setEnabled(false);
}

Instance::~Instance() {
    FCITX_D();
    d->icManager_.finalize();
    d->addonManager_.unload();
    d->notifications_ = nullptr;
    d->icManager_.setInstance(nullptr);
}

void InstanceArgument::parseOption(int argc, char **argv) {
    if (argc >= 1) {
        argv0 = argv[0];
    } else {
        argv0 = "fcitx5";
    }
    struct option longOptions[] = {{"enable", required_argument, nullptr, 0},
                                   {"disable", required_argument, nullptr, 0},
                                   {"verbose", required_argument, nullptr, 0},
                                   {"keep", no_argument, nullptr, 'k'},
                                   {"ui", required_argument, nullptr, 'u'},
                                   {"replace", no_argument, nullptr, 'r'},
                                   {"version", no_argument, nullptr, 'v'},
                                   {"help", no_argument, nullptr, 'h'},
                                   {"option", required_argument, nullptr, 'o'},
                                   {nullptr, 0, 0, 0}};

    int optionIndex = 0;
    int c;
    std::string addonOptionString;
    while ((c = getopt_long(argc, argv, "ru:dDs:hvo:", longOptions,
                            &optionIndex)) != EOF) {
        switch (c) {
        case 0: {
            switch (optionIndex) {
            case 0:
                enableList = stringutils::split(optarg, ",");
                break;
            case 1:
                disableList = stringutils::split(optarg, ",");
                break;
            case 2:
                Log::setLogRule(optarg);
                break;
            default:
                quietQuit = true;
                printUsage();
                break;
            }
        } break;
        case 'r':
            tryReplace = true;
            break;
        case 'u':
            uiName = optarg;
            break;
        case 'd':
            runAsDaemon = true;
            break;
        case 'D':
            runAsDaemon = false;
            break;
        case 'k':
            exitWhenMainDisplayDisconnected = false;
            break;
        case 's':
            overrideDelay = std::atoi(optarg);
            break;
        case 'h':
            quietQuit = true;
            printUsage();
            break;
        case 'v':
            quietQuit = true;
            printVersion();
            break;
        case 'o':
            addonOptionString = optarg;
            break;
        default:
            quietQuit = true;
            printUsage();
        }
        if (quietQuit) {
            break;
        }
    }

    std::unordered_map<std::string, std::vector<std::string>> addonOptions;
    for (const std::string_view item :
         stringutils::split(addonOptionString, ",")) {
        auto tokens = stringutils::split(item, "=");
        if (tokens.size() != 2) {
            continue;
        }
        addonOptions[tokens[0]] = stringutils::split(tokens[1], ":");
    }
    addonOptions_ = std::move(addonOptions);
}

void Instance::setSignalPipe(int fd) {
    FCITX_D();
    d->signalPipe_ = fd;
    d->signalPipeEvent_ = d->eventLoop_.addIOEvent(
        fd, IOEventFlag::In, [this](EventSource *, int, IOEventFlags) {
            handleSignal();
            return true;
        });
}

bool Instance::willTryReplace() const {
    FCITX_D();
    return d->arg_.tryReplace;
}

bool Instance::exitWhenMainDisplayDisconnected() const {
    FCITX_D();
    return d->arg_.exitWhenMainDisplayDisconnected;
}

bool Instance::exiting() const {
    FCITX_D();
    return d->exit_;
}

void Instance::handleSignal() {
    FCITX_D();
    uint8_t signo = 0;
    while (fs::safeRead(d->signalPipe_, &signo, sizeof(signo)) > 0) {
        if (signo == SIGINT || signo == SIGTERM || signo == SIGQUIT ||
            signo == SIGXCPU) {
            exit();
        } else if (signo == SIGUSR1) {
            reloadConfig();
        } else if (signo == SIGCHLD) {
            d->zombieReaper_->setNextInterval(2000000);
            d->zombieReaper_->setOneShot();
        }
    }
}

void Instance::initialize() {
    FCITX_D();
    if (!d->arg_.uiName.empty()) {
        d->arg_.enableList.push_back(d->arg_.uiName);
    }
    reloadConfig();
    d->icManager_.registerProperty("inputState", &d->inputStateFactory_);
    std::unordered_set<std::string> enabled;
    std::unordered_set<std::string> disabled;
    std::tie(enabled, disabled) = d->overrideAddons();
    FCITX_INFO() << "Override Enabled Addons: " << enabled;
    FCITX_INFO() << "Override Disabled Addons: " << disabled;
    d->addonManager_.load(enabled, disabled);
    if (d->exit_) {
        return;
    }
    d->imManager_.load([d](InputMethodManager &) { d->buildDefaultGroup(); });
    d->uiManager_.load(d->arg_.uiName);

    const auto *entry = d->imManager_.entry("keyboard-us");
    FCITX_LOG_IF(Error, !entry) << "Couldn't find keyboard-us";
    d->preloadInputMethodEvent_ = d->eventLoop_.addTimeEvent(
        CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 1000000, 0,
        [this](EventSourceTime *, uint64_t) {
            FCITX_D();
            if (d->exit_ || !d->globalConfig_.preloadInputMethod()) {
                return false;
            }
            // Preload first input method.
            if (!d->imManager_.currentGroup().inputMethodList().empty()) {
                if (auto entry =
                        d->imManager_.entry(d->imManager_.currentGroup()
                                                .inputMethodList()[0]
                                                .name())) {
                    d->addonManager_.addon(entry->addon(), true);
                }
            }
            // Preload default input method.
            if (!d->imManager_.currentGroup().defaultInputMethod().empty()) {
                if (auto entry = d->imManager_.entry(
                        d->imManager_.currentGroup().defaultInputMethod())) {
                    d->addonManager_.addon(entry->addon(), true);
                }
            }
            return false;
        });
    d->zombieReaper_ = d->eventLoop_.addTimeEvent(
        CLOCK_MONOTONIC, now(CLOCK_MONOTONIC), 0,
        [](EventSourceTime *, uint64_t) {
            pid_t res;
            while ((res = waitpid(-1, nullptr, WNOHANG)) > 0) {
            }
            return false;
        });
    d->zombieReaper_->setEnabled(false);

    d->exitEvent_ = d->eventLoop_.addExitEvent([this](EventSource *) {
        FCITX_DEBUG() << "Running save...";
        save();
        return false;
    });
    d->notifications_ = d->addonManager_.addon("notifications", true);
}

int Instance::exec() {
    FCITX_D();
    if (d->arg_.quietQuit) {
        return 0;
    }
    d->exit_ = false;
    d->exitCode_ = 0;
    initialize();
    if (d->exit_) {
        return d->exitCode_;
    }
    d->running_ = true;
    auto r = eventLoop().exec();
    d->running_ = false;

    return r ? d->exitCode_ : 1;
}

void Instance::setRunning(bool running) {
    FCITX_D();
    d->running_ = running;
}

bool Instance::isRunning() const {
    FCITX_D();
    return d->running_;
}

InputMethodMode Instance::inputMethodMode() const {
    FCITX_D();
    return d->inputMethodMode_;
}

void Instance::setInputMethodMode(InputMethodMode mode) {
    FCITX_D();
    if (d->inputMethodMode_ == mode) {
        return;
    }
    d->inputMethodMode_ = mode;
    postEvent(InputMethodModeChangedEvent());
}

bool Instance::isRestartRequested() const {
    FCITX_D();
    return d->restart_;
}

bool Instance::virtualKeyboardAutoShow() const {
    FCITX_D();
    return d->virtualKeyboardAutoShow_;
}

void Instance::setVirtualKeyboardAutoShow(bool autoShow) {
    FCITX_D();
    d->virtualKeyboardAutoShow_ = autoShow;
}

bool Instance::virtualKeyboardAutoHide() const {
    FCITX_D();
    return d->virtualKeyboardAutoHide_;
}

void Instance::setVirtualKeyboardAutoHide(bool autoHide) {
    FCITX_D();
    d->virtualKeyboardAutoHide_ = autoHide;
}

VirtualKeyboardFunctionMode Instance::virtualKeyboardFunctionMode() const {
    FCITX_D();
    return d->virtualKeyboardFunctionMode_;
}

void Instance::setVirtualKeyboardFunctionMode(
    VirtualKeyboardFunctionMode mode) {
    FCITX_D();
    d->virtualKeyboardFunctionMode_ = mode;
}

void Instance::setBinaryMode() {
    FCITX_D();
    d->binaryMode_ = true;
}

bool Instance::canRestart() const {
    FCITX_D();
    const auto &addonNames = d->addonManager_.loadedAddonNames();
    return d->binaryMode_ &&
           std::all_of(addonNames.begin(), addonNames.end(),
                       [d](const std::string &name) {
                           auto addon = d->addonManager_.lookupAddon(name);
                           if (!addon) {
                               return true;
                           }
                           return addon->canRestart();
                       });
}

InstancePrivate *Instance::privateData() {
    FCITX_D();
    return d;
}

EventLoop &Instance::eventLoop() {
    FCITX_D();
    return d->eventLoop_;
}

EventDispatcher &Instance::eventDispatcher() {
    FCITX_D();
    return d->eventDispatcher_;
}

InputContextManager &Instance::inputContextManager() {
    FCITX_D();
    return d->icManager_;
}

AddonManager &Instance::addonManager() {
    FCITX_D();
    return d->addonManager_;
}

InputMethodManager &Instance::inputMethodManager() {
    FCITX_D();
    return d->imManager_;
}

const InputMethodManager &Instance::inputMethodManager() const {
    FCITX_D();
    return d->imManager_;
}

UserInterfaceManager &Instance::userInterfaceManager() {
    FCITX_D();
    return d->uiManager_;
}

GlobalConfig &Instance::globalConfig() {
    FCITX_D();
    return d->globalConfig_;
}

bool Instance::postEvent(Event &event) {
    return std::as_const(*this).postEvent(event);
}

bool Instance::postEvent(Event &event) const {
    FCITX_D();
    if (d->exit_) {
        return false;
    }
    auto iter = d->eventHandlers_.find(event.type());
    if (iter != d->eventHandlers_.end()) {
        auto &handlers = iter->second;
        EventWatcherPhase phaseOrder[] = {
            EventWatcherPhase::ReservedFirst, EventWatcherPhase::PreInputMethod,
            EventWatcherPhase::InputMethod, EventWatcherPhase::PostInputMethod,
            EventWatcherPhase::ReservedLast};

        for (auto phase : phaseOrder) {
            if (auto iter2 = handlers.find(phase); iter2 != handlers.end()) {
                for (auto &handler : iter2->second.view()) {
                    handler(event);
                    if (event.filtered()) {
                        break;
                    }
                }
            }
            if (event.filtered()) {
                break;
            }
        }

        // Make sure this part of fix is always executed regardless of the
        // filter.
        if (event.type() == EventType::InputContextKeyEvent) {
            auto &keyEvent = static_cast<KeyEvent &>(event);
            auto *ic = keyEvent.inputContext();
#ifdef ENABLE_KEYBOARD
            do {
                if (!keyEvent.forward() && !keyEvent.origKey().code()) {
                    break;
                }
                auto *inputState = ic->propertyFor(&d->inputStateFactory_);
                auto *xkbState = inputState->customXkbState();
                if (!xkbState) {
                    break;
                }
                // This need to be called after xkb_state_key_get_*, and should
                // be called against all Key regardless whether they are
                // filtered or not.
                xkb_state_update_key(xkbState, keyEvent.origKey().code(),
                                     keyEvent.isRelease() ? XKB_KEY_UP
                                                          : XKB_KEY_DOWN);
            } while (0);
#endif
            if (ic->capabilityFlags().test(CapabilityFlag::KeyEventOrderFix) &&
                !keyEvent.accepted() && ic->hasPendingEventsStrictOrder()) {
                // Re-forward the event to ensure we got delivered later than
                // commit.
                keyEvent.filterAndAccept();
                ic->forwardKey(keyEvent.origKey(), keyEvent.isRelease(),
                               keyEvent.time());
            }
            d_ptr->uiManager_.flush();
        }
    }
    return event.accepted();
}

std::unique_ptr<HandlerTableEntry<EventHandler>>
Instance::watchEvent(EventType type, EventWatcherPhase phase,
                     EventHandler callback) {
    FCITX_D();
    if (phase == EventWatcherPhase::ReservedFirst ||
        phase == EventWatcherPhase::ReservedLast) {
        throw std::invalid_argument("Reserved Phase is only for internal use");
    }
    return d->watchEvent(type, phase, std::move(callback));
}

bool groupContains(const InputMethodGroup &group, const std::string &name) {
    const auto &list = group.inputMethodList();
    auto iter = std::find_if(list.begin(), list.end(),
                             [&name](const InputMethodGroupItem &item) {
                                 return item.name() == name;
                             });
    return iter != list.end();
}

std::string Instance::inputMethod(InputContext *ic) {
    FCITX_D();
    auto *inputState = ic->propertyFor(&d->inputStateFactory_);
    // Small hack to make sure when InputMethodEngine::deactivate is called,
    // current im is the right one.
    if (!inputState->overrideDeactivateIM_.empty()) {
        return inputState->overrideDeactivateIM_;
    }

    auto &group = d->imManager_.currentGroup();
    if (ic->capabilityFlags().test(CapabilityFlag::Disable) ||
        (ic->capabilityFlags().test(CapabilityFlag::Password) &&
         !d->globalConfig_.allowInputMethodForPassword())) {
        auto defaultLayoutIM =
            stringutils::concat("keyboard-", group.defaultLayout());
        const auto *entry = d->imManager_.entry(defaultLayoutIM);
        if (!entry) {
            entry = d->imManager_.entry("keyboard-us");
        }
        return entry ? entry->uniqueName() : "";
    }

    if (group.inputMethodList().empty()) {
        return "";
    }
    if (inputState->isActive()) {
        if (!inputState->localIM_.empty() &&
            groupContains(group, inputState->localIM_)) {
            return inputState->localIM_;
        }
        return group.defaultInputMethod();
    }

    return group.inputMethodList()[0].name();
}

const InputMethodEntry *Instance::inputMethodEntry(InputContext *ic) {
    FCITX_D();
    auto imName = inputMethod(ic);
    if (imName.empty()) {
        return nullptr;
    }
    return d->imManager_.entry(imName);
}

InputMethodEngine *Instance::inputMethodEngine(InputContext *ic) {
    FCITX_D();
    const auto *entry = inputMethodEntry(ic);
    if (!entry) {
        return nullptr;
    }
    return static_cast<InputMethodEngine *>(
        d->addonManager_.addon(entry->addon(), true));
}

InputMethodEngine *Instance::inputMethodEngine(const std::string &name) {
    FCITX_D();
    const auto *entry = d->imManager_.entry(name);
    if (!entry) {
        return nullptr;
    }
    return static_cast<InputMethodEngine *>(
        d->addonManager_.addon(entry->addon(), true));
}

std::string Instance::inputMethodIcon(InputContext *ic) {
    std::string icon;
    const auto *entry = inputMethodEntry(ic);
    if (entry) {
        auto *engine = inputMethodEngine(ic);
        if (engine) {
            icon = engine->subModeIcon(*entry, *ic);
        }
        if (icon.empty()) {
            icon = entry->icon();
        }
    } else {
        icon = "input-keyboard";
    }
    return icon;
}

std::string Instance::inputMethodLabel(InputContext *ic) {
    std::string label;

    const auto *entry = inputMethodEntry(ic);
    auto *engine = inputMethodEngine(ic);

    if (engine && entry) {
        label = engine->subModeLabel(*entry, *ic);
    }
    if (label.empty() && entry) {
        label = entry->label();
    }
    return label;
}

uint32_t Instance::processCompose(InputContext *ic, KeySym keysym) {
#ifdef ENABLE_KEYBOARD
    FCITX_D();
    auto *state = ic->propertyFor(&d->inputStateFactory_);

    auto *xkbComposeState = state->xkbComposeState();
    if (!xkbComposeState) {
        return 0;
    }

    auto keyval = static_cast<xkb_keysym_t>(keysym);

    enum xkb_compose_feed_result result =
        xkb_compose_state_feed(xkbComposeState, keyval);
    if (result == XKB_COMPOSE_FEED_IGNORED) {
        return 0;
    }

    enum xkb_compose_status status =
        xkb_compose_state_get_status(xkbComposeState);
    if (status == XKB_COMPOSE_NOTHING) {
        return 0;
    }
    if (status == XKB_COMPOSE_COMPOSED) {
        char buffer[FCITX_UTF8_MAX_LENGTH + 1] = {'\0', '\0', '\0', '\0',
                                                  '\0', '\0', '\0'};
        int length =
            xkb_compose_state_get_utf8(xkbComposeState, buffer, sizeof(buffer));
        xkb_compose_state_reset(xkbComposeState);
        if (length == 0) {
            return FCITX_INVALID_COMPOSE_RESULT;
        }

        uint32_t c = 0;
        fcitx_utf8_get_char(buffer, &c);
        return c;
    }
    if (status == XKB_COMPOSE_CANCELLED) {
        xkb_compose_state_reset(xkbComposeState);
    }

    return FCITX_INVALID_COMPOSE_RESULT;
#else
    FCITX_UNUSED(ic);
    FCITX_UNUSED(keysym);
    return 0;
#endif
}

std::optional<std::string> Instance::processComposeString(InputContext *ic,
                                                          KeySym keysym) {
#ifdef ENABLE_KEYBOARD
    FCITX_D();
    auto *state = ic->propertyFor(&d->inputStateFactory_);

    auto *xkbComposeState = state->xkbComposeState();
    if (!xkbComposeState) {
        return std::string();
    }

    auto keyval = static_cast<xkb_keysym_t>(keysym);
    enum xkb_compose_feed_result result =
        xkb_compose_state_feed(xkbComposeState, keyval);

    if (result == XKB_COMPOSE_FEED_IGNORED) {
        return std::string();
    }

    enum xkb_compose_status status =
        xkb_compose_state_get_status(xkbComposeState);
    if (status == XKB_COMPOSE_NOTHING) {
        return std::string();
    }
    if (status == XKB_COMPOSE_COMPOSED) {
        // This may not be NUL-terminiated.
        std::array<char, 256> buffer;
        auto length = xkb_compose_state_get_utf8(xkbComposeState, buffer.data(),
                                                 buffer.size());
        xkb_compose_state_reset(xkbComposeState);
        if (length == 0) {
            return std::nullopt;
        }

        const auto bufferBegin = buffer.begin();
        const auto bufferEnd = std::next(bufferBegin, length);
        if (utf8::validate(bufferBegin, bufferEnd)) {
            return std::string(bufferBegin, bufferEnd);
        }
        return std::nullopt;
    }
    if (status == XKB_COMPOSE_CANCELLED) {
        xkb_compose_state_reset(xkbComposeState);
    }
    return std::nullopt;
#else
    FCITX_UNUSED(ic);
    FCITX_UNUSED(keysym);
    return std::string();
#endif
}

bool Instance::isComposing(InputContext *inputContext) {
#ifdef ENABLE_KEYBOARD
    FCITX_D();
    auto *state = inputContext->propertyFor(&d->inputStateFactory_);

    auto *xkbComposeState = state->xkbComposeState();
    if (!xkbComposeState) {
        return false;
    }

    return xkb_compose_state_get_status(xkbComposeState) ==
           XKB_COMPOSE_COMPOSING;
#else
    FCITX_UNUSED(inputContext);
    return false;
#endif
}

void Instance::resetCompose(InputContext *inputContext) {
#ifdef ENABLE_KEYBOARD
    FCITX_D();
    auto *state = inputContext->propertyFor(&d->inputStateFactory_);
    auto *xkbComposeState = state->xkbComposeState();
    if (!xkbComposeState) {
        return;
    }
    xkb_compose_state_reset(xkbComposeState);
#else
    FCITX_UNUSED(inputContext);
#endif
}

void Instance::save() {
    FCITX_D();
    // Refresh timestamp for next auto save.
    d->idleStartTimestamp_ = now(CLOCK_MONOTONIC);
    d->imManager_.save();
    d->addonManager_.saveAll();
}

void Instance::activate() {
    FCITX_D();
    if (auto *ic = mostRecentInputContext()) {
        CheckInputMethodChanged imChangedRAII(ic, d);
        activate(ic);
    }
}

std::string Instance::addonForInputMethod(const std::string &imName) {

    if (const auto *entry = inputMethodManager().entry(imName)) {
        return entry->uniqueName();
    }
    return {};
}

void Instance::configure() {
    startProcess({StandardPath::fcitxPath("bindir", "fcitx5-configtool")});
}

void Instance::configureAddon(const std::string &) {}

void Instance::configureInputMethod(const std::string &) {}

std::string Instance::currentInputMethod() {
    if (auto *ic = mostRecentInputContext()) {
        if (const auto *entry = inputMethodEntry(ic)) {
            return entry->uniqueName();
        }
    }
    return {};
}

std::string Instance::currentUI() {
    FCITX_D();
    return d->uiManager_.currentUI();
}

void Instance::deactivate() {
    FCITX_D();
    if (auto *ic = mostRecentInputContext()) {
        CheckInputMethodChanged imChangedRAII(ic, d);
        deactivate(ic);
    }
}

void Instance::exit() { exit(0); }

void Instance::exit(int exitCode) {
    FCITX_D();
    d->exit_ = true;
    d->exitCode_ = exitCode;
    if (d->running_) {
        d->eventLoop_.exit();
    }
}

void Instance::reloadAddonConfig(const std::string &addonName) {
    auto *addon = addonManager().addon(addonName);
    if (addon) {
        addon->reloadConfig();
    }
}

void Instance::refresh() {
    FCITX_D();
    auto [enabled, disabled] = d->overrideAddons();
    d->addonManager_.load(enabled, disabled);
    d->imManager_.refresh();
}

void Instance::reloadConfig() {
    FCITX_D();
    const auto &standardPath = StandardPath::global();
    auto file =
        standardPath.open(StandardPath::Type::PkgConfig, "config", O_RDONLY);
    RawConfig config;
    readFromIni(config, file.fd());
    d->globalConfig_.load(config);
    FCITX_DEBUG() << "Trigger Key: "
                  << Key::keyListToString(d->globalConfig_.triggerKeys());
    d->icManager_.setPropertyPropagatePolicy(
        d->globalConfig_.shareInputState());
    if (d->globalConfig_.preeditEnabledByDefault() !=
        d->icManager_.isPreeditEnabledByDefault()) {
        d->icManager_.setPreeditEnabledByDefault(
            d->globalConfig_.preeditEnabledByDefault());
        d->icManager_.foreach([d](InputContext *ic) {
            ic->setEnablePreedit(d->globalConfig_.preeditEnabledByDefault());
            return true;
        });
    }
#ifdef ENABLE_KEYBOARD
    d->keymapCache_.clear();
    if (d->inputStateFactory_.registered()) {
        d->icManager_.foreach([d](InputContext *ic) {
            auto *inputState = ic->propertyFor(&d->inputStateFactory_);
            inputState->resetXkbState();
            return true;
        });
    }
#endif
    if (d->running_) {
        postEvent(GlobalConfigReloadedEvent());
    }

    if (d->globalConfig_.autoSavePeriod() <= 0) {
        d->periodicalSave_->setEnabled(false);
    } else {
        d->periodicalSave_->setNextInterval(AutoSaveMinInUsecs *
                                            d->globalConfig_.autoSavePeriod());
        d->periodicalSave_->setOneShot();
    }
}

void Instance::resetInputMethodList() {
    FCITX_D();
    d->imManager_.reset([d](InputMethodManager &) { d->buildDefaultGroup(); });
}

void Instance::restart() {
    FCITX_D();
    if (!canRestart()) {
        return;
    }
    d->restart_ = true;
    exit();
}

void Instance::setCurrentInputMethod(const std::string &name) {
    setCurrentInputMethod(mostRecentInputContext(), name, false);
}

void Instance::setCurrentInputMethod(InputContext *ic, const std::string &name,
                                     bool local) {
    FCITX_D();
    if (!canTrigger()) {
        return;
    }

    auto &imManager = inputMethodManager();
    const auto &imList = imManager.currentGroup().inputMethodList();
    auto iter = std::find_if(imList.begin(), imList.end(),
                             [&name](const InputMethodGroupItem &item) {
                                 return item.name() == name;
                             });
    if (iter == imList.end()) {
        return;
    }

    auto setGlobalDefaultInputMethod = [d](const std::string &name) {
        std::vector<std::unique_ptr<CheckInputMethodChanged>> groupRAIICheck;
        d->icManager_.foreachFocused([d, &groupRAIICheck](InputContext *ic) {
            assert(ic->hasFocus());
            groupRAIICheck.push_back(
                std::make_unique<CheckInputMethodChanged>(ic, d));
            return true;
        });
        d->imManager_.setDefaultInputMethod(name);
    };

    auto idx = std::distance(imList.begin(), iter);
    if (ic) {
        CheckInputMethodChanged imChangedRAII(ic, d);
        auto currentIM = inputMethod(ic);
        if (currentIM == name) {
            return;
        }
        auto *inputState = ic->propertyFor(&d->inputStateFactory_);

        if (idx != 0) {
            if (local) {
                inputState->setLocalIM(name);
            } else {
                inputState->setLocalIM({});

                setGlobalDefaultInputMethod(name);
            }
            inputState->setActive(true);
        } else {
            inputState->setActive(false);
        }
        if (inputState->imChanged_) {
            inputState->imChanged_->setReason(InputMethodSwitchedReason::Other);
        }
    } else {
        // We can't set local input method if we don't have a IC, but we should
        // still to change the global default.
        if (local) {
            return;
        }
        if (idx != 0) {
            setGlobalDefaultInputMethod(name);
        }
        return;
    }
}

int Instance::state() {
    FCITX_D();
    if (auto *ic = mostRecentInputContext()) {
        auto *inputState = ic->propertyFor(&d->inputStateFactory_);
        return inputState->isActive() ? 2 : 1;
    }
    return 0;
}

void Instance::toggle() {
    FCITX_D();
    if (auto *ic = mostRecentInputContext()) {
        CheckInputMethodChanged imChangedRAII(ic, d);
        trigger(ic, true);
    }
}

void Instance::enumerate(bool forward) {
    FCITX_D();
    if (auto *ic = mostRecentInputContext()) {
        CheckInputMethodChanged imChangedRAII(ic, d);
        enumerate(ic, forward);
    }
}

bool Instance::canTrigger() const {
    const auto &imManager = inputMethodManager();
    return (imManager.currentGroup().inputMethodList().size() > 1);
}

bool Instance::canAltTrigger(InputContext *ic) const {
    if (!canTrigger()) {
        return false;
    }
    FCITX_D();
    auto *inputState = ic->propertyFor(&d->inputStateFactory_);
    if (inputState->isActive()) {
        return true;
    }
    return inputState->lastIMChangeIsAltTrigger_;
}

bool Instance::canEnumerate(InputContext *ic) const {
    FCITX_D();
    if (!canTrigger()) {
        return false;
    }

    if (d->globalConfig_.enumerateSkipFirst()) {
        auto *inputState = ic->propertyFor(&d->inputStateFactory_);
        if (!inputState->isActive()) {
            return false;
        }
        return d->imManager_.currentGroup().inputMethodList().size() > 2;
    }

    return true;
}

bool Instance::canChangeGroup() const {
    const auto &imManager = inputMethodManager();
    return (imManager.groupCount() > 1);
}

bool Instance::toggle(InputContext *ic, InputMethodSwitchedReason reason) {
    FCITX_D();
    auto *inputState = ic->propertyFor(&d->inputStateFactory_);
    if (!canTrigger()) {
        return false;
    }
    inputState->setActive(!inputState->isActive());
    if (inputState->imChanged_) {
        inputState->imChanged_->setReason(reason);
    }
    return true;
}

bool Instance::trigger(InputContext *ic, bool totallyReleased) {
    FCITX_D();
    auto *inputState = ic->propertyFor(&d->inputStateFactory_);
    if (!canTrigger()) {
        return false;
    }
    // Active -> inactive -> enumerate.
    // Inactive -> active -> inactive -> enumerate.
    if (totallyReleased) {
        toggle(ic);
        inputState->firstTrigger_ = true;
    } else {
        if (!d->globalConfig_.enumerateWithTriggerKeys() ||
            (inputState->firstTrigger_ && inputState->isActive()) ||
            (d->globalConfig_.enumerateSkipFirst() &&
             d->imManager_.currentGroup().inputMethodList().size() <= 2)) {
            toggle(ic);
        } else {
            enumerate(ic, true);
        }
        inputState->firstTrigger_ = false;
    }
    return true;
}

bool Instance::altTrigger(InputContext *ic) {
    if (!canAltTrigger(ic)) {
        return false;
    }

    toggle(ic, InputMethodSwitchedReason::AltTrigger);
    return true;
}

bool Instance::activate(InputContext *ic) {
    FCITX_D();
    auto *inputState = ic->propertyFor(&d->inputStateFactory_);
    if (!canTrigger()) {
        return false;
    }
    if (inputState->isActive()) {
        return true;
    }
    inputState->setActive(true);
    if (inputState->imChanged_) {
        inputState->imChanged_->setReason(InputMethodSwitchedReason::Activate);
    }
    return true;
}

bool Instance::deactivate(InputContext *ic) {
    FCITX_D();
    auto *inputState = ic->propertyFor(&d->inputStateFactory_);
    if (!canTrigger()) {
        return false;
    }
    if (!inputState->isActive()) {
        return true;
    }
    inputState->setActive(false);
    if (inputState->imChanged_) {
        inputState->imChanged_->setReason(
            InputMethodSwitchedReason::Deactivate);
    }
    return true;
}

bool Instance::enumerate(InputContext *ic, bool forward) {
    FCITX_D();
    auto &imManager = inputMethodManager();
    auto *inputState = ic->propertyFor(&d->inputStateFactory_);
    const auto &imList = imManager.currentGroup().inputMethodList();
    if (!canTrigger()) {
        return false;
    }

    if (d->globalConfig_.enumerateSkipFirst() && imList.size() <= 2) {
        return false;
    }

    auto currentIM = inputMethod(ic);

    auto iter = std::find_if(imList.begin(), imList.end(),
                             [&currentIM](const InputMethodGroupItem &item) {
                                 return item.name() == currentIM;
                             });
    if (iter == imList.end()) {
        return false;
    }
    int idx = std::distance(imList.begin(), iter);
    auto nextIdx = [forward, &imList](int idx) {
        // be careful not to use negative to avoid overflow.
        return (idx + (forward ? 1 : (imList.size() - 1))) % imList.size();
    };

    idx = nextIdx(idx);
    if (d->globalConfig_.enumerateSkipFirst() && idx == 0) {
        idx = nextIdx(idx);
    }
    if (idx != 0) {
        std::vector<std::unique_ptr<CheckInputMethodChanged>> groupRAIICheck;
        d->icManager_.foreachFocused([d, &groupRAIICheck](InputContext *ic) {
            assert(ic->hasFocus());
            groupRAIICheck.push_back(
                std::make_unique<CheckInputMethodChanged>(ic, d));
            return true;
        });
        imManager.setDefaultInputMethod(imList[idx].name());
        inputState->setActive(true);
        inputState->setLocalIM({});
    } else {
        inputState->setActive(false);
    }
    if (inputState->imChanged_) {
        inputState->imChanged_->setReason(InputMethodSwitchedReason::Enumerate);
    }

    return true;
}

std::string Instance::commitFilter(InputContext *inputContext,
                                   const std::string &orig) {
    std::string result = orig;
    emit<Instance::CommitFilter>(inputContext, result);
    return result;
}

Text Instance::outputFilter(InputContext *inputContext, const Text &orig) {
    Text result = orig;
    emit<Instance::OutputFilter>(inputContext, result);
    if ((&orig == &inputContext->inputPanel().clientPreedit() ||
         &orig == &inputContext->inputPanel().preedit()) &&
        !globalConfig().showPreeditForPassword() &&
        inputContext->capabilityFlags().test(CapabilityFlag::Password)) {
        Text newText;
        for (int i = 0, e = result.size(); i < e; i++) {
            auto length = utf8::length(result.stringAt(i));
            std::string dot;
            dot.reserve(length * 3);
            while (length != 0) {
                dot += "\xe2\x80\xa2";
                length -= 1;
            }
            newText.append(std::move(dot),
                           result.formatAt(i) | TextFormatFlag::DontCommit);
        }
        result = std::move(newText);
    }
    return result;
}

InputContext *Instance::lastFocusedInputContext() {
    FCITX_D();
    return d->icManager_.lastFocusedInputContext();
}

InputContext *Instance::mostRecentInputContext() {
    FCITX_D();
    return d->icManager_.mostRecentInputContext();
}

void Instance::flushUI() {
    FCITX_D();
    d->uiManager_.flush();
}

int scoreForGroup(FocusGroup *group, const std::string &displayHint) {
    // Hardcode wayland over X11.
    if (displayHint.empty()) {
        if (group->display() == "x11:") {
            return 2;
        }
        if (stringutils::startsWith(group->display(), "x11:")) {
            return 1;
        }
        if (group->display() == "wayland:") {
            return 4;
        }
        if (stringutils::startsWith(group->display(), "wayland:")) {
            return 3;
        }
    } else {
        if (group->display() == displayHint) {
            return 2;
        }
        if (stringutils::startsWith(group->display(), displayHint)) {
            return 1;
        }
    }
    return -1;
}

FocusGroup *Instance::defaultFocusGroup(const std::string &displayHint) {
    FCITX_D();
    FocusGroup *defaultFocusGroup = nullptr;

    int score = 0;
    d->icManager_.foreachGroup(
        [&score, &displayHint, &defaultFocusGroup](FocusGroup *group) {
            auto newScore = scoreForGroup(group, displayHint);
            if (newScore > score) {
                defaultFocusGroup = group;
                score = newScore;
            }

            return true;
        });
    return defaultFocusGroup;
}

void Instance::activateInputMethod(InputContextEvent &event) {
    FCITX_D();
    FCITX_DEBUG() << "Instance::activateInputMethod";
    InputContext *ic = event.inputContext();
    auto *inputState = ic->propertyFor(&d->inputStateFactory_);
    const auto *entry = inputMethodEntry(ic);
    if (entry) {
        FCITX_DEBUG() << "Activate: "
                      << "[Last]:" << inputState->lastIM_
                      << " [Activating]:" << entry->uniqueName();
        assert(inputState->lastIM_.empty());
        inputState->lastIM_ = entry->uniqueName();
    }
    auto *engine = inputMethodEngine(ic);
    if (!engine || !entry) {
        return;
    }
#ifdef ENABLE_KEYBOARD
    if (auto *xkbState = inputState->customXkbState(true)) {
        if (auto *mods = findValue(d->stateMask_, ic->display())) {
            FCITX_KEYTRACE() << "Update mask to customXkbState";
            auto depressed = std::get<0>(*mods);
            auto latched = std::get<1>(*mods);
            auto locked = std::get<2>(*mods);

            // set modifiers in depressed if they don't appear in any of the
            // final masks
            // depressed |= ~(depressed | latched | locked);
            FCITX_KEYTRACE() << depressed << " " << latched << " " << locked;
            if (depressed == 0) {
                inputState->setModsAllReleased();
            }
            xkb_state_update_mask(xkbState, depressed, latched, locked, 0, 0,
                                  0);
        }
    }
#endif
    ic->statusArea().clearGroup(StatusGroup::InputMethod);
    engine->activate(*entry, event);
    postEvent(InputMethodActivatedEvent(entry->uniqueName(), ic));
}

void Instance::deactivateInputMethod(InputContextEvent &event) {
    FCITX_D();
    FCITX_DEBUG() << "Instance::deactivateInputMethod event_type="
                  << static_cast<uint32_t>(event.type());
    InputContext *ic = event.inputContext();
    auto *inputState = ic->propertyFor(&d->inputStateFactory_);
    const InputMethodEntry *entry = nullptr;
    InputMethodEngine *engine = nullptr;

    if (event.type() == EventType::InputContextSwitchInputMethod) {
        auto &icEvent =
            static_cast<InputContextSwitchInputMethodEvent &>(event);
        FCITX_DEBUG() << "Switch reason: "
                      << static_cast<int>(icEvent.reason());
        FCITX_DEBUG() << "Old Input method: " << icEvent.oldInputMethod();
        entry = d->imManager_.entry(icEvent.oldInputMethod());
    } else {
        entry = inputMethodEntry(ic);
    }
    if (entry) {
        FCITX_DEBUG() << "Deactivate: "
                      << "[Last]:" << inputState->lastIM_
                      << " [Deactivating]:" << entry->uniqueName();
        assert(entry->uniqueName() == inputState->lastIM_);
        engine = static_cast<InputMethodEngine *>(
            d->addonManager_.addon(entry->addon()));
    }
    inputState->lastIM_.clear();
    if (!engine || !entry) {
        return;
    }
    inputState->overrideDeactivateIM_ = entry->uniqueName();
    engine->deactivate(*entry, event);
    inputState->overrideDeactivateIM_.clear();
    postEvent(InputMethodDeactivatedEvent(entry->uniqueName(), ic));
}

bool Instance::enumerateGroup(bool forward) {
    auto &imManager = inputMethodManager();
    auto groups = imManager.groups();
    if (groups.size() <= 1) {
        return false;
    }
    if (forward) {
        imManager.setCurrentGroup(groups[1]);
    } else {
        imManager.setCurrentGroup(groups.back());
    }
    return true;
}

void Instance::showInputMethodInformation(InputContext *ic) {
    FCITX_DEBUG() << "Input method switched";
    FCITX_D();
    if (!d->globalConfig_.showInputMethodInformation()) {
        return;
    }
    d->showInputMethodInformation(ic);
}

bool Instance::checkUpdate() const {
    FCITX_D();
    return (isInFlatpak() && fs::isreg("/app/.updated")) ||
           d->addonManager_.checkUpdate() || d->imManager_.checkUpdate() ||
           postEvent(CheckUpdateEvent());
}

void Instance::setXkbParameters(const std::string &display,
                                const std::string &rule,
                                const std::string &model,
                                const std::string &options) {
#ifdef ENABLE_KEYBOARD
    FCITX_D();
    bool resetState = false;
    if (auto *param = findValue(d->xkbParams_, display)) {
        if (std::get<0>(*param) != rule || std::get<1>(*param) != model ||
            std::get<2>(*param) != options) {
            std::get<0>(*param) = rule;
            std::get<1>(*param) = model;
            std::get<2>(*param) = options;
            resetState = true;
        }
    } else {
        d->xkbParams_.emplace(display, std::make_tuple(rule, model, options));
    }

    if (resetState) {
        d->keymapCache_[display].clear();
        d->icManager_.foreach([d, &display](InputContext *ic) {
            if (ic->display() == display ||
                d->xkbParams_.count(ic->display()) == 0) {
                auto *inputState = ic->propertyFor(&d->inputStateFactory_);
                inputState->resetXkbState();
            }
            return true;
        });
    }
#else
    FCITX_UNUSED(display);
    FCITX_UNUSED(rule);
    FCITX_UNUSED(model);
    FCITX_UNUSED(options);
#endif
}

void Instance::updateXkbStateMask(const std::string &display,
                                  uint32_t depressed_mods,
                                  uint32_t latched_mods, uint32_t locked_mods) {
    FCITX_D();
    d->stateMask_[display] =
        std::make_tuple(depressed_mods, latched_mods, locked_mods);
}

void Instance::clearXkbStateMask(const std::string &display) {
    FCITX_D();
    d->stateMask_.erase(display);
}

const char *Instance::version() { return FCITX_VERSION_STRING; }

} // namespace fcitx
