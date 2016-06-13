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
#include "fcitx-utils/event.h"
#include "globalconfig.h"
#include "fcitx/inputcontextmanager.h"
#include "fcitx/addonmanager.h"
#include "inputstate_p.h"
#include "fcitx-utils/stringutils.h"
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <sys/wait.h>
#include <iostream>

namespace {

void initAsDaemon() {
    pid_t pid;
    if ((pid = fork()) > 0) {
        waitpid(pid, NULL, 0);
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

struct enum_hash {
    template <typename T>
    inline auto operator()(T const value) const {
        return std::hash<std::underlying_type_t<T>>()(static_cast<std::underlying_type_t<T>>(value));
    }
};

struct InstanceArgument {
    InstanceArgument() {}
    void parseOption(int argc, char *argv[]);
    void printVersion() {}
    void printUsage() {}

    int overrideDelay = -1;
    bool tryReplace = false;
    bool quietQuit = false;
    bool runAsDaemon = true;
    std::string uiName;
    std::vector<std::string> enableList;
    std::vector<std::string> disableList;
};

class InstancePrivate {
public:
    InstancePrivate(Instance *) {}

    InstanceArgument arg;
    bool initialized = false;

    int signalPipe = -1;
    EventLoop eventLoop;
    std::unique_ptr<EventSourceIO> signalPipeEvent;
    InputContextManager icManager;
    AddonManager addonManager;
    GlobalConfig globalConfig;
    std::unordered_map<EventType, std::unordered_map<EventWatcherPhase, HandlerTable<EventHandler>, enum_hash>,
                       enum_hash> eventHandlers;
    int inputStateSlot = -1;
    std::vector<std::unique_ptr<HandlerTableEntry<EventHandler>>> eventWatchers;
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
    d->addonManager.setInstance(this);
    d->icManager.setInstance(this);

    d->inputStateSlot = d->icManager.registerProperty([] (InputContext&) { return new InputState; });

    d->eventWatchers.emplace_back(watchEvent(EventType::InputContextKeyEvent, EventWatcherPhase::PreInputMethod, [this, d] (Event &event) {
        auto &keyEvent = static_cast<KeyEvent &>(event);
        struct {
            const KeyList &list;
            std::function<void()> callback;
        } keyHandlers [] = {
            { d->globalConfig.triggerKeys(), [] () { } },
        };

        auto ic = keyEvent.inputContext();

        auto inputState = ic->propertyAs<InputState>(d->inputStateSlot);
        const bool isModifier = keyEvent.key().isModifier();
        if (keyEvent.isRelease()) {
            int idx = 0;
            for (auto &keyHandler : keyHandlers) {
                if (isModifier && inputState->keyReleased == idx && Key::keyListCheck(keyHandler.list, keyEvent.key())) {
                    keyHandler.callback();
                }
                idx++;
            }
        }

        if (!keyEvent.filtered() && !keyEvent.isRelease()) {
            int idx = 0;
            for (auto &keyHandler : keyHandlers) {
                if (Key::keyListCheck(keyHandler.list, keyEvent.key())) {
                    if (isModifier) {
                        inputState->keyReleased = idx;
                    } else {
                        keyHandler.callback();
                    }
                }
                idx++;
            }
        }
    }));
}

Instance::~Instance() {}

void InstanceArgument::parseOption(int argc, char **argv) {
    struct option longOptions[] = {{"ui", 1, 0, 0},      {"replace", 0, 0, 0}, {"enable", 1, 0, 0},
                                   {"disable", 1, 0, 0}, {"version", 0, 0, 0}, {"help", 0, 0, 0},
                                   {NULL, 0, 0, 0}};

    int optionIndex = 0;
    int c;
    while ((c = getopt_long(argc, argv, "ru:dDs:hv", longOptions, &optionIndex)) != EOF) {
        switch (c) {
        case 0: {
            switch (optionIndex) {
            case 0:
                uiName = optarg;
                break;
            case 1:
                tryReplace = true;
                break;
            case 2:
                enableList = stringutils::split(optarg, ",");
                break;
            case 3:
                disableList = stringutils::split(optarg, ",");
                break;
            case 4:
                quietQuit = true;
                printVersion();
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
    d->signalPipe = fd;
    d->signalPipeEvent.reset(d->eventLoop.addIOEvent(fd, IOEventFlag::In, [this](EventSource *, int, IOEventFlags) {
        handleSignal();
        return true;
    }));
}

bool Instance::willTryReplace() const {
    FCITX_D();
    return d->arg.tryReplace;
}

void Instance::handleSignal() {
    FCITX_D();
    uint8_t signo = 0;
    while (read(d->signalPipe, &signo, sizeof(signo)) > 0) {
        if (signo == SIGINT || signo == SIGTERM || signo == SIGQUIT || signo == SIGXCPU) {
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
    d->addonManager.load();
}

int Instance::exec() {
    FCITX_D();
    if (d->arg.quietQuit) {
        return 0;
    }
    initialize();
    auto r = eventLoop().exec();

    return r ? 0 : 1;
}

EventLoop &Instance::eventLoop() {
    FCITX_D();
    return d->eventLoop;
}

InputContextManager &Instance::inputContextManager() {
    FCITX_D();
    return d->icManager;
}

AddonManager &Instance::addonManager() {
    FCITX_D();
    return d->addonManager;
}

GlobalConfig &Instance::globalConfig() {
    FCITX_D();
    return d->globalConfig;
}

bool Instance::postEvent(Event &event) {
    FCITX_D();
    auto iter = d->eventHandlers.find(event.type());
    if (iter != d->eventHandlers.end()) {
        auto &handlers = iter->second;
        EventWatcherPhase phaseOrder[] = {EventWatcherPhase::PreInputMethod, EventWatcherPhase::InputMethod,
                                          EventWatcherPhase::PostInputMethod};

        for (auto phase : phaseOrder) {
            auto iter2 = handlers.find(phase);
            if (iter2 != handlers.end()) {
                for (auto &handler : iter2->second) {
                    handler.handler()(event);
                    if (event.filtered()) {
                        return event.accepted();
                    }
                }
            }
        }
    }
    return event.accepted();
}

HandlerTableEntry<EventHandler> *Instance::watchEvent(EventType type, EventWatcherPhase phase, EventHandler callback) {
    FCITX_D();
    return d->eventHandlers[type][phase].add(callback);
}

void Instance::activate() {}

std::string Instance::addonForInputMethod(const std::string &imName) { return {}; }

void Instance::configure() {}

void Instance::configureAddon(const std::string &addon) {}

void Instance::configureInputMethod(const std::string &imName) {}

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

void Instance::reloadAddonConfig(const std::string &addonName) {}

void Instance::reloadConfig() {}

void Instance::resetInputMethodList() {}

void Instance::restart() {}

void Instance::setCurrentInputMethod(const std::string &imName) {}

int Instance::state() { return 0; }

void Instance::toggle() {}
}
