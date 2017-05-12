/*
 * Copyright (C) 2016~2016 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
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

#include "instance.h"
#include "addonmanager.h"
#include "fcitx-utils/event.h"
#include "fcitx-utils/standardpath.h"
#include "fcitx-utils/stringutils.h"
#include "globalconfig.h"
#include "inputcontextmanager.h"
#include "inputcontextproperty.h"
#include "inputmethodengine.h"
#include "inputmethodentry.h"
#include "inputmethodmanager.h"
#include "userinterfacemanager.h"
#include <getopt.h>
#include <iostream>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {

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
    chdir("/");

    signal(SIGINT, oldint);
    signal(SIGHUP, oldhup);
    signal(SIGQUIT, oldquit);
    signal(SIGPIPE, oldpipe);
    signal(SIGTTOU, oldttou);
    signal(SIGTTIN, oldttin);
    signal(SIGCHLD, oldchld);
}
}

namespace fcitx {

class CheckInputMethodChanged;

struct InputState : public InputContextProperty {
    int keyReleased = -1;
    bool active;
    CheckInputMethodChanged *imChanged;
};

class CheckInputMethodChanged {
public:
    CheckInputMethodChanged(InputContext *ic, Instance *instance)
        : instance_(instance), ic_(ic->watch()),
          inputMethod_(instance->inputMethod(ic)),
          reason_(InputMethodSwitchedReason::Other) {
        auto inputState = ic->propertyAs<InputState>("inputState");
        inputState->imChanged = this;
    }
    ~CheckInputMethodChanged() {
        if (!ic_.isValid()) {
            return;
        }
        auto ic = ic_.get();
        auto inputState = ic->propertyAs<InputState>("inputState");
        inputState->imChanged = nullptr;
        if (inputMethod_ != instance_->inputMethod(ic)) {
            instance_->postEvent(
                InputContextSwitchInputMethodEvent(reason_, inputMethod_, ic));
        }
    }

    void setReason(InputMethodSwitchedReason reason) { reason_ = reason; }

private:
    Instance *instance_;
    TrackableObjectReference<InputContext> ic_;
    std::string inputMethod_;
    InputMethodSwitchedReason reason_;
};

struct InstanceArgument {
    InstanceArgument() {}
    InstanceArgument(const InstanceArgument &) = default;
    void parseOption(int argc, char *argv[]);
    void printVersion() {}
    void printUsage() {}

    InstanceArgument &operator=(const InstanceArgument &) = default;

    int overrideDelay = -1;
    bool tryReplace = false;
    bool quietQuit = false;
    bool runAsDaemon = false;
    bool quitWhenMainDisplayDisconnected = true;
    std::string uiName;
    std::vector<std::string> enableList;
    std::vector<std::string> disableList;
};

class InstancePrivate : public QPtrHolder<Instance> {
public:
    InstancePrivate(Instance *q) : QPtrHolder<Instance>(q) {}

    HandlerTableEntry<EventHandler> *
    watchEvent(EventType type, EventWatcherPhase phase, EventHandler callback) {
        return eventHandlers_[type][phase].add(callback);
    }

    InstanceArgument arg_;
    bool initialized_ = false;

    int signalPipe_ = -1;
    EventLoop eventLoop_;
    std::unique_ptr<EventSourceIO> signalPipeEvent_;
    std::unique_ptr<EventSource> exitEvent_;
    InputContextManager icManager_;
    AddonManager addonManager_;
    InputMethodManager imManager_{&this->addonManager_};
    UserInterfaceManager uiManager_;
    GlobalConfig globalConfig_;
    std::unordered_map<EventType,
                       std::unordered_map<EventWatcherPhase,
                                          HandlerTable<EventHandler>, EnumHash>,
                       EnumHash>
        eventHandlers_;
    std::vector<std::unique_ptr<HandlerTableEntry<EventHandler>>>
        eventWatchers_;
    std::unique_ptr<EventSource> uiUpdateEvent_;

    FCITX_DEFINE_SIGNAL_PRIVATE(Instance, CommitFilter);
    FCITX_DEFINE_SIGNAL_PRIVATE(Instance, OutputFilter);
    FCITX_DEFINE_SIGNAL_PRIVATE(Instance, KeyEventResult);

    class InputStateFactory : public InputContextPropertyFactory {
    public:
        InputStateFactory(InstancePrivate *d) : d_ptr(d) {}

        InputContextProperty *create(InputContext &) override {
            auto property = new InputState;
            property->active = d_ptr->globalConfig_.activeByDefault();
            return property;
        }

    private:
        InstancePrivate *d_ptr;
    };

    InputStateFactory inputStateFactory{this};
};

Instance::Instance(int argc, char **argv) {
    InstanceArgument arg;
    arg.parseOption(argc, argv);
    if (arg.quietQuit) {
        return;
    }

    if (arg.runAsDaemon) {
        initAsDaemon();
    }

    if (arg.overrideDelay > 0) {
        sleep(arg.overrideDelay);
    }

    // we need fork before this
    d_ptr.reset(new InstancePrivate(this));
    FCITX_D();
    d->arg_ = arg;
    d->addonManager_.setInstance(this);
    d->icManager_.setInstance(this);
    d->imManager_.setInstance(this);

    d->icManager_.registerProperty("inputState", &d->inputStateFactory);

    d->eventWatchers_.emplace_back(watchEvent(
        EventType::InputContextKeyEvent, EventWatcherPhase::PreInputMethod,
        [this, d](Event &event) {
            auto &keyEvent = static_cast<KeyEvent &>(event);
            auto ic = keyEvent.inputContext();
            CheckInputMethodChanged imChangedRAII(ic, this);

            struct {
                const KeyList &list;
                std::function<bool()> callback;
            } keyHandlers[] = {
                {d->globalConfig_.triggerKeys(),
                 [this, ic]() { return trigger(ic); }},
            };

            auto inputState = ic->propertyAs<InputState>("inputState");
            const bool isModifier = keyEvent.key().isModifier();
            if (keyEvent.isRelease()) {
                int idx = 0;
                for (auto &keyHandler : keyHandlers) {
                    if (isModifier && inputState->keyReleased == idx &&
                        keyEvent.key().checkKeyList(keyHandler.list)) {
                        if (keyHandler.callback()) {
                            return keyEvent.filterAndAccept();
                        }
                    }
                    idx++;
                }
            }

            if (!keyEvent.filtered() && !keyEvent.isRelease()) {
                int idx = 0;
                for (auto &keyHandler : keyHandlers) {
                    if (keyEvent.key().checkKeyList(keyHandler.list)) {
                        if (isModifier) {
                            inputState->keyReleased = idx;
                        } else {
                            if (keyHandler.callback()) {
                                return keyEvent.filterAndAccept();
                            }
                        }
                    }
                    idx++;
                }
            }
        }));
    d->eventWatchers_.emplace_back(
        watchEvent(EventType::InputContextKeyEvent,
                   EventWatcherPhase::InputMethod, [this, d](Event &event) {
                       auto &keyEvent = static_cast<KeyEvent &>(event);
                       auto ic = keyEvent.inputContext();
                       auto engine = inputMethodEngine(ic);
                       auto entry = inputMethodEntry(ic);
                       if (!engine || !entry) {
                           return;
                       }
                       engine->keyEvent(*entry, keyEvent);
                   }));
    d->eventWatchers_.emplace_back(
        d->watchEvent(EventType::InputContextKeyEvent,
                      EventWatcherPhase::ReservedLast, [this, d](Event &event) {
                          auto &keyEvent = static_cast<KeyEvent &>(event);
                          auto ic = keyEvent.inputContext();
                          auto engine = inputMethodEngine(ic);
                          auto entry = inputMethodEntry(ic);
                          if (!engine || !entry) {
                              return;
                          }
                          engine->filterKey(*entry, keyEvent);
                          emit<Instance::KeyEventResult>(keyEvent);
                      }));
    d->eventWatchers_.emplace_back(d->watchEvent(
        EventType::InputContextFocusIn, EventWatcherPhase::ReservedFirst,
        [this, d](Event &event) {
            auto &icEvent = static_cast<InputContextEvent &>(event);
            auto ic = icEvent.inputContext();
            auto engine = inputMethodEngine(ic);
            auto entry = inputMethodEntry(ic);
            if (!engine || !entry) {
                return;
            }
            engine->activate(*entry, icEvent);
        }));
    d->eventWatchers_.emplace_back(d->watchEvent(
        EventType::InputContextFocusOut, EventWatcherPhase::ReservedFirst,
        [this, d](Event &event) {
            auto &icEvent = static_cast<InputContextEvent &>(event);
            auto ic = icEvent.inputContext();
            if (!ic->capabilityFlags().test(
                    CapabilityFlag::ClientUnfocusCommit)) {
                // do server side commit
                auto commit =
                    ic->inputPanel().clientPreedit().toStringForCommit();
                if (commit.size()) {
                    ic->commitString(commit);
                }
            }
            auto engine = inputMethodEngine(ic);
            auto entry = inputMethodEntry(ic);
            if (!engine || !entry) {
                return;
            }
            engine->deactivate(*entry, icEvent);
            ic->statusArea().clear();
        }));
    d->eventWatchers_.emplace_back(
        watchEvent(EventType::InputContextReset, EventWatcherPhase::InputMethod,
                   [this, d](Event &event) {
                       auto &icEvent = static_cast<InputContextEvent &>(event);
                       auto ic = icEvent.inputContext();
                       if (!ic->hasFocus()) {
                           return;
                       }
                       auto engine = inputMethodEngine(ic);
                       auto entry = inputMethodEntry(ic);
                       if (!engine || !entry) {
                           return;
                       }
                       engine->reset(*entry, icEvent);
                   }));
    d->eventWatchers_.emplace_back(d->watchEvent(
        EventType::InputContextSwitchInputMethod,
        EventWatcherPhase::ReservedFirst, [this, d](Event &event) {
            auto &icEvent =
                static_cast<InputContextSwitchInputMethodEvent &>(event);
            auto ic = icEvent.inputContext();
            if (!ic->hasFocus()) {
                return;
            }
            if (auto oldEntry = d->imManager_.entry(icEvent.oldInputMethod())) {
                auto oldEngine = static_cast<InputMethodEngine *>(
                    d->addonManager_.addon(oldEntry->addon()));
                if (oldEngine) {
                    oldEngine->deactivate(*oldEntry, icEvent);
                }
            }
            auto engine = inputMethodEngine(ic);
            auto entry = inputMethodEntry(ic);
            if (!engine || !entry) {
                return;
            }
            engine->activate(*entry, icEvent);
        }));
    d->eventWatchers_.emplace_back(d->watchEvent(
        EventType::InputContextUpdateUI, EventWatcherPhase::ReservedFirst,
        [this, d](Event &event) {
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
        [this, d](Event &event) {
            auto &icEvent = static_cast<InputContextEvent &>(event);
            d->uiManager_.expire(icEvent.inputContext());
        }));
    d->uiUpdateEvent_.reset(d->eventLoop_.addDeferEvent([d](EventSource *) {
        d->uiManager_.flush();
        return true;
    }));
    d->uiUpdateEvent_->setEnabled(false);
}

Instance::~Instance() {
    FCITX_D();
    d->addonManager_.unload();
    d->icManager_.setInstance(nullptr);
}

void InstanceArgument::parseOption(int argc, char **argv) {
    struct option longOptions[] = {{"enable", required_argument, nullptr, 0},
                                   {"disable", required_argument, nullptr, 0},
                                   {"keep", no_argument, nullptr, 'k'},
                                   {"ui", required_argument, nullptr, 'u'},
                                   {"replace", no_argument, nullptr, 'r'},
                                   {"version", no_argument, nullptr, 'v'},
                                   {"help", no_argument, nullptr, 'h'},
                                   {nullptr, 0, 0, 0}};

    int optionIndex = 0;
    int c;
    while ((c = getopt_long(argc, argv, "ru:dDs:hv", longOptions,
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
            default:
                quietQuit = true;
                printUsage();
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
            quitWhenMainDisplayDisconnected = false;
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
        default:
            quietQuit = true;
            printUsage();
        }
    }
}

void Instance::setSignalPipe(int fd) {
    FCITX_D();
    d->signalPipe_ = fd;
    d->signalPipeEvent_.reset(d->eventLoop_.addIOEvent(
        fd, IOEventFlag::In, [this](EventSource *, int, IOEventFlags) {
            handleSignal();
            return true;
        }));
}

bool Instance::willTryReplace() const {
    FCITX_D();
    return d->arg_.tryReplace;
}

bool Instance::quitWhenMainDisplayDisconnected() const {
    FCITX_D();
    return d->arg_.quitWhenMainDisplayDisconnected;
}

void Instance::handleSignal() {
    FCITX_D();
    uint8_t signo = 0;
    while (read(d->signalPipe_, &signo, sizeof(signo)) > 0) {
        if (signo == SIGINT || signo == SIGTERM || signo == SIGQUIT ||
            signo == SIGXCPU) {
            exit();
        } else if (signo == SIGHUP) {
            restart();
        } else if (signo == SIGUSR1) {
            reloadConfig();
        }
    }
}

void Instance::initialize() {
    FCITX_D();
    if (d->arg_.uiName.size()) {
        d->arg_.enableList.push_back(d->arg_.uiName);
    }
    d->addonManager_.load(
        {std::begin(d->arg_.enableList), std::end(d->arg_.enableList)},
        {std::begin(d->arg_.disableList), std::end(d->arg_.disableList)});
    d->imManager_.load();
    d->uiManager_.load(&d->addonManager_, d->arg_.uiName);
    d->exitEvent_.reset(d->eventLoop_.addExitEvent([this](EventSource *) {
        save();
        return false;
    }));
}

int Instance::exec() {
    FCITX_D();
    if (d->arg_.quietQuit) {
        return 0;
    }
    initialize();
    auto r = eventLoop().exec();

    return r ? 0 : 1;
}

EventLoop &Instance::eventLoop() {
    FCITX_D();
    return d->eventLoop_;
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

UserInterfaceManager &Instance::userInterfaceManager() {
    FCITX_D();
    return d->uiManager_;
}

GlobalConfig &Instance::globalConfig() {
    FCITX_D();
    return d->globalConfig_;
}

bool Instance::postEvent(Event &event) {
    FCITX_D();
    auto iter = d->eventHandlers_.find(event.type());
    if (iter != d->eventHandlers_.end()) {
        auto &handlers = iter->second;
        EventWatcherPhase phaseOrder[] = {
            EventWatcherPhase::ReservedFirst, EventWatcherPhase::PreInputMethod,
            EventWatcherPhase::InputMethod, EventWatcherPhase::PostInputMethod,
            EventWatcherPhase::ReservedLast};

        for (auto phase : phaseOrder) {
            auto iter2 = handlers.find(phase);
            if (iter2 != handlers.end()) {
                for (auto &handler : iter2->second.view()) {
                    handler(event);
                    if (event.filtered()) {
                        return event.accepted();
                    }
                }
            }
        }
    }
    return event.accepted();
}

HandlerTableEntry<EventHandler> *Instance::watchEvent(EventType type,
                                                      EventWatcherPhase phase,
                                                      EventHandler callback) {
    FCITX_D();
    if (phase == EventWatcherPhase::ReservedFirst ||
        phase == EventWatcherPhase::ReservedLast) {
        throw std::invalid_argument("Reserved Phase is only for internal use");
    }
    return d->watchEvent(type, phase, callback);
}

std::string Instance::inputMethod(InputContext *ic) {
    FCITX_D();
    auto &group = d->imManager_.currentGroup();
    auto inputState = ic->propertyAs<InputState>("inputState");
    if (inputState->active) {
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
    auto entry = inputMethodEntry(ic);
    if (!entry) {
        return nullptr;
    }
    return static_cast<InputMethodEngine *>(
        d->addonManager_.addon(entry->addon()));
}

void Instance::save() {
    FCITX_D();
    d->imManager_.save();
    d->addonManager_.saveAll();
}

void Instance::activate() {}

std::string Instance::addonForInputMethod(const std::string &imName) {

    if (auto entry = inputMethodManager().entry(imName)) {
        return entry->name();
    }
    return {};
}

void Instance::configure() {}

void Instance::configureAddon(const std::string &) {}

void Instance::configureInputMethod(const std::string &imName) {
    auto addon = addonForInputMethod(imName);
    if (!addon.empty()) {
        return configureAddon(addon);
    }
}

std::string Instance::currentInputMethod() {
    // FIXME
    return {};
}

std::string Instance::currentUI() {
    // FIXME
    return {};
}

void Instance::deactivate() {}

void Instance::exit() { eventLoop().quit(); }

void Instance::reloadAddonConfig(const std::string &addonName) {
    auto addon = addonManager().addon(addonName);
    if (addon) {
        addon->reloadConfig();
    }
}

void Instance::reloadConfig() {}

void Instance::resetInputMethodList() {}

void Instance::restart() {
    auto fcitxBinary = StandardPath::fcitxPath("bindir") + "/fcitx";
    std::vector<char> command{fcitxBinary.begin(), fcitxBinary.end()};
    command.push_back('\0');
    char *const argv[] = {command.data(), nullptr};
    execv(argv[0], argv);
    perror("Restart failed: execvp:");
    _exit(1);
}

void Instance::setCurrentInputMethod(const std::string &) {}

int Instance::state() { return 0; }

void Instance::toggle() {}

bool Instance::trigger(InputContext *ic) {
    auto &imManager = inputMethodManager();
    auto inputState = ic->propertyAs<InputState>("inputState");
    if (imManager.currentGroup().inputMethodList().size() <= 1) {
        return false;
    }
    inputState->active = !inputState->active;
    if (inputState->imChanged) {
        inputState->imChanged->setReason(InputMethodSwitchedReason::Trigger);
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
    return result;
}
}
