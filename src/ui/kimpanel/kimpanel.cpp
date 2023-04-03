/*
 * SPDX-FileCopyrightText: 2016-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "kimpanel.h"
#include <fcitx/inputmethodengine.h>
#include "fcitx-utils/dbus/objectvtable.h"
#include "fcitx-utils/dbus/servicewatcher.h"
#include "fcitx-utils/i18n.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/stringutils.h"
#include "fcitx-utils/utf8.h"
#include "fcitx/action.h"
#include "fcitx/addonmanager.h"
#include "fcitx/inputcontext.h"
#include "fcitx/inputmethodentry.h"
#include "fcitx/inputmethodmanager.h"
#include "fcitx/instance.h"
#include "fcitx/menu.h"
#include "fcitx/misc_p.h"
#include "fcitx/userinterfacemanager.h"
#include "dbus_public.h"

namespace fcitx {

enum class CursorRectMethod {
    SetSpotRect,
    SetRelativeSpotRect,
    SetRelativeSpotRectV2
};

class KimpanelProxy : public dbus::ObjectVTable<KimpanelProxy> {

public:
    KimpanelProxy(Kimpanel *parent, dbus::Bus *bus)
        : bus_(bus), slot_(bus_->addMatch(dbus::MatchRule("org.kde.impanel", "",
                                                          "org.kde.impanel"),
                                          [parent](dbus::Message &msg) {
                                              parent->msgV1Handler(msg);
                                              return true;
                                          })),
          slot2_(bus_->addMatch(
              dbus::MatchRule("org.kde.impanel", "", "org.kde.impanel2"),
              [parent](dbus::Message &msg) {
                  parent->msgV2Handler(msg);
                  return true;
              })) {}

    void updateCursor(InputContext *inputContext, CursorRectMethod method) {
        const char *name = nullptr;
        switch (method) {
        case CursorRectMethod::SetSpotRect:
            name = "SetSpotRect";
            break;
        case CursorRectMethod::SetRelativeSpotRect:
            name = "SetRelativeSpotRect";
            break;
        case CursorRectMethod::SetRelativeSpotRectV2:
            name = "SetRelativeSpotRectV2";
            break;
        }
        if (!name) {
            return;
        }
        auto msg = bus_->createMethodCall("org.kde.impanel", "/org/kde/impanel",
                                          "org.kde.impanel2", name);

        int32_t x = inputContext->cursorRect().left(),
                y = inputContext->cursorRect().top(),
                w = inputContext->cursorRect().width(),
                h = inputContext->cursorRect().height();

        msg << x << y << w << h;
        if (method == CursorRectMethod::SetRelativeSpotRectV2) {
            msg << inputContext->scaleFactor();
        }
        msg.send();
    }

    FCITX_OBJECT_VTABLE_SIGNAL(execDialog, "ExecDialog", "s");
    FCITX_OBJECT_VTABLE_SIGNAL(execMenu, "ExecMenu", "as");
    FCITX_OBJECT_VTABLE_SIGNAL(registerProperties, "RegisterProperties", "as");
    FCITX_OBJECT_VTABLE_SIGNAL(updateProperty, "UpdateProperty", "s");
    FCITX_OBJECT_VTABLE_SIGNAL(removeProperty, "RemoveProperty", "s");
    FCITX_OBJECT_VTABLE_SIGNAL(showAux, "ShowAux", "b");
    FCITX_OBJECT_VTABLE_SIGNAL(showPreedit, "ShowPreedit", "b");
    FCITX_OBJECT_VTABLE_SIGNAL(showLookupTable, "ShowLookupTable", "b");
    FCITX_OBJECT_VTABLE_SIGNAL(updateLookupTableCursor,
                               "UpdateLookupTableCursor", "i");
    FCITX_OBJECT_VTABLE_SIGNAL(updatePreeditCaret, "UpdatePreeditCaret", "i");
    FCITX_OBJECT_VTABLE_SIGNAL(updatePreeditText, "UpdatePreeditText", "ss");
    FCITX_OBJECT_VTABLE_SIGNAL(updateAux, "UpdateAux", "ss");
    FCITX_OBJECT_VTABLE_SIGNAL(updateSpotLocation, "UpdateSpotLocation", "ii");
    FCITX_OBJECT_VTABLE_SIGNAL(updateScreen, "UpdateScreen", "i");
    FCITX_OBJECT_VTABLE_SIGNAL(enable, "Enable", "b");

private:
    dbus::Bus *bus_;
    std::unique_ptr<dbus::Slot> slot_;
    std::unique_ptr<dbus::Slot> slot2_;
};

Kimpanel::Kimpanel(Instance *instance)
    : instance_(instance), bus_(dbus()->call<IDBusModule::bus>()),
      watcher_(*bus_) {
    entry_ = watcher_.watchService(
        "org.kde.impanel", [this](const std::string &, const std::string &,
                                  const std::string &newOwner) {
            FCITX_INFO() << "Kimpanel new owner: " << newOwner;
            setAvailable(!newOwner.empty());
        });
}

Kimpanel::~Kimpanel() {}

void Kimpanel::suspend() {
    eventHandlers_.clear();
    proxy_.reset();
    bus_->releaseName("org.kde.kimpanel.inputmethod");
    hasRelative_ = false;
    hasRelativeV2_ = false;
}

void Kimpanel::registerAllProperties(InputContext *ic) {
    std::vector<std::string> props;
    if (!ic) {
        ic = instance_->lastFocusedInputContext();
    }
    if (ic) {
        for (auto *action :
             ic->statusArea().actions(StatusGroup::BeforeInputMethod)) {
            props.push_back(actionToStatus(action, ic));
        }
    }

    const auto imStatus = inputMethodStatus(ic);
    props.push_back(imStatus);

    if (ic) {
        for (auto group :
             {StatusGroup::InputMethod, StatusGroup::AfterInputMethod}) {
            for (auto *action : ic->statusArea().actions(group)) {
                props.push_back(actionToStatus(action, ic));
            }
        }
    }

    proxy_->registerProperties(props);
    proxy_->updateProperty(imStatus);
    proxy_->enable(true);

    bus_->flush();
}

std::string Kimpanel::actionToStatus(Action *action, InputContext *ic) {
    // Path : Short Text : icon : Long text : special
    const char *type = "";
    if (action->menu()) {
        type = "menu";
    }
    return stringutils::concat("/Fcitx/", action->name(), ":",
                               action->shortText(ic), ":",
                               IconTheme::iconName(action->icon(ic)), ":",
                               action->longText(ic), ":", type);
}

void Kimpanel::resume() {
    proxy_ = std::make_unique<KimpanelProxy>(this, bus_);
    bus_->addObjectVTable("/kimpanel", "org.kde.kimpanel.inputmethod", *proxy_);
    bus_->requestName(
        "org.kde.kimpanel.inputmethod",
        Flags<dbus::RequestNameFlag>{dbus::RequestNameFlag::ReplaceExisting,
                                     dbus::RequestNameFlag::Queue});
    bus_->flush();
    if (available_) {
        registerAllProperties();

        auto msg = bus_->createMethodCall("org.kde.impanel", "/org/kde/impanel",
                                          "org.freedesktop.DBus.Introspectable",
                                          "Introspect");
        relativeQuery_ = msg.callAsync(0, [this](dbus::Message &reply) {
            std::string s;
            if (reply >> s) {
                if (s.find("SetRelativeSpotRect") != std::string::npos) {
                    hasRelative_ = true;
                }
                if (s.find("SetRelativeSpotRectV2") != std::string::npos) {
                    hasRelativeV2_ = true;
                }
            }
            return true;
        });
    }

    auto check = [this](Event &event) {
        if (!proxy_) {
            return;
        }
        auto &icEvent = static_cast<InputContextEvent &>(event);
        auto *inputContext = icEvent.inputContext();
        if (inputContext->hasFocus()) {
            CursorRectMethod method = CursorRectMethod::SetSpotRect;
            if (inputContext->capabilityFlags().test(
                    CapabilityFlag::RelativeRect)) {
                if (hasRelativeV2_) {
                    method = CursorRectMethod::SetRelativeSpotRectV2;
                } else if (hasRelative_) {
                    method = CursorRectMethod::SetRelativeSpotRect;
                }
            }
            proxy_->updateCursor(inputContext, method);
        }
    };
    eventHandlers_.emplace_back(
        instance_->watchEvent(EventType::InputContextCursorRectChanged,
                              EventWatcherPhase::Default, check));
    eventHandlers_.emplace_back(instance_->watchEvent(
        EventType::InputContextFocusIn, EventWatcherPhase::Default, check));
    eventHandlers_.emplace_back(instance_->watchEvent(
        EventType::InputContextSwitchInputMethod, EventWatcherPhase::Default,
        [this](Event &event) {
            auto &icEvent = static_cast<InputContextEvent &>(event);
            updateCurrentInputMethod(icEvent.inputContext());
        }));
    eventHandlers_.emplace_back(instance_->watchEvent(
        EventType::InputMethodGroupChanged, EventWatcherPhase::Default,
        [this](Event &) {
            if (auto *ic = instance_->lastFocusedInputContext()) {
                updateCurrentInputMethod(ic);
            }
        }));
    eventHandlers_.emplace_back(instance_->watchEvent(
        EventType::InputContextFocusIn, EventWatcherPhase::Default,
        [this](Event &event) {
            // Difference IC has difference set of actions.
            auto &icEvent = static_cast<InputContextEvent &>(event);
            registerAllProperties(icEvent.inputContext());
            updateCurrentInputMethod(icEvent.inputContext());
        }));
    eventHandlers_.emplace_back(instance_->watchEvent(
        EventType::FocusGroupFocusChanged, EventWatcherPhase::Default,
        [this](Event &event) {
            auto &focusEvent =
                static_cast<FocusGroupFocusChangedEvent &>(event);
            if (!focusEvent.newFocus() &&
                lastInputContext_.get() == focusEvent.oldFocus()) {
                proxy_->showAux(false);
                proxy_->showPreedit(false);
                proxy_->showLookupTable(false);
                bus_->flush();
            }
        }));
}

void Kimpanel::update(UserInterfaceComponent component,
                      InputContext *inputContext) {
    if (component == UserInterfaceComponent::InputPanel) {
        updateInputPanel(inputContext);
    } else if (component == UserInterfaceComponent::StatusArea) {
        registerAllProperties(inputContext);
    }
}

void Kimpanel::updateInputPanel(InputContext *inputContext) {
    lastInputContext_ = inputContext->watch();
    auto *instance = this->instance();
    auto &inputPanel = inputContext->inputPanel();

    auto preedit = instance->outputFilter(inputContext, inputPanel.preedit());
    auto auxUp = instance->outputFilter(inputContext, inputPanel.auxUp());
    auto preeditString = preedit.toString();
    auto auxUpString = auxUp.toString();
    if (!preeditString.empty() || !auxUpString.empty()) {
        auto text = auxUpString + preeditString;
        if (preedit.cursor() >= 0 &&
            static_cast<size_t>(preedit.cursor()) <= preeditString.size()) {
            auto cursor = preedit.cursor() + auxUpString.size();
            auto utf8Cursor = utf8::lengthValidated(
                text.begin(), std::next(text.begin(), cursor));
            proxy_->updateAux("", "");
            proxy_->updatePreeditText(text, "");
            if (utf8Cursor != utf8::INVALID_LENGTH) {
                proxy_->updatePreeditCaret(utf8Cursor);
            } else {
                proxy_->updatePreeditCaret(0);
            }
            proxy_->showPreedit(true);
            proxy_->showAux(false);
        } else {
            proxy_->updateAux(text, "");
            proxy_->updatePreeditText("", "");
            proxy_->showPreedit(false);
            proxy_->showAux(true);
        }
    } else {
        proxy_->showAux(false);
        proxy_->showPreedit(false);
    }

    auto auxDown = instance->outputFilter(inputContext, inputPanel.auxDown());
    auto auxDownString = auxDown.toString();
    auto candidateList = inputPanel.candidateList();

    auto msg = bus_->createMethodCall("org.kde.impanel", "/org/kde/impanel",
                                      "org.kde.impanel2", "SetLookupTable");
    auto visible =
        !auxDownString.empty() || (candidateList && candidateList->size());
    if (visible) {
        std::vector<std::string> labels;
        std::vector<std::string> texts;
        std::vector<std::string> attrs;
        bool hasPrev = false, hasNext = false;
        int pos = -1;
        int layout = static_cast<int>(CandidateLayoutHint::NotSet);
        if (!auxDownString.empty()) {
            labels.emplace_back("");
            texts.push_back(auxDownString);
            attrs.emplace_back("");
        }
        auxDownIsEmpty_ = auxDownString.empty();
        if (candidateList) {
            for (int i = 0, e = candidateList->size(); i < e; i++) {
                const auto &candidate = candidateList->candidate(i);
                if (candidate.isPlaceHolder()) {
                    continue;
                }
                Text labelText = candidate.hasCustomLabel()
                                     ? candidate.customLabel()
                                     : candidateList->label(i);

                labelText = instance->outputFilter(inputContext, labelText);
                labels.push_back(labelText.toString());
                auto candidateText =
                    instance->outputFilter(inputContext, candidate.text());
                texts.push_back(candidateText.toString());
                attrs.emplace_back("");
            }
            if (auto *pageable = candidateList->toPageable()) {
                hasPrev = pageable->hasPrev();
                hasNext = pageable->hasNext();
            }
            pos = candidateList->cursorIndex();
            if (pos >= 0) {
                pos += (auxDownString.empty() ? 0 : 1);
            }
            layout = static_cast<int>(candidateList->layoutHint());
        }

        msg << labels << texts << attrs;
        msg << hasPrev << hasNext << pos << layout;
    } else {
        std::vector<std::string> labels;
        std::vector<std::string> texts;
        std::vector<std::string> attrs;
        msg << labels << texts << attrs;
        msg << false << false << -1 << 0;
    }
    msg.send();
    proxy_->showLookupTable(visible);
    bus_->flush();
}

// This is heuristic, but we guranteed that we don't do crazy things with label.
std::string extractTextForLabel(const std::string &label) {
    if (label.empty()) {
        return "";
    }
    auto texts = stringutils::split(label, FCITX_WHITESPACE);
    if (texts.empty()) {
        return "";
    }

    return texts[0];
}

std::string Kimpanel::inputMethodStatus(InputContext *ic) {
    std::string label;
    std::string description = _("Not available");
    std::string altDescription = "";
    std::string icon = "input-keyboard";
    if (ic) {
        icon = instance_->inputMethodIcon(ic);
        if (auto entry = instance_->inputMethodEntry(ic)) {
            label = entry->label();
            if (auto engine = instance_->inputMethodEngine(ic)) {
                auto subModeLabel = engine->subModeLabel(*entry, *ic);
                if (!subModeLabel.empty()) {
                    label = subModeLabel;
                }
                altDescription = engine->subMode(*entry, *ic);
            }
            description = entry->name();
        }
    }

    label = extractTextForLabel(label);

    static const bool preferSymbolic = !isKDE();
    if (preferSymbolic && icon == "input-keyboard") {
        icon = "input-keyboard-symbolic";
    }

    return stringutils::concat("/Fcitx/im:", description, ":",
                               IconTheme::iconName(icon), ":", altDescription,
                               ":menu,label=", label);
}

void Kimpanel::updateCurrentInputMethod(InputContext *ic) {
    if (!proxy_) {
        return;
    }
    proxy_->updateProperty(inputMethodStatus(ic));
    proxy_->enable(true);
}

void Kimpanel::msgV1Handler(dbus::Message &msg) {
    if (msg.member() == "Exit") {
        instance_->exit();
    } else if (msg.member() == "ReloadConfig") {
        instance_->restart();
    } else if (msg.member() == "Restart") {
        instance_->restart();
    } else if (msg.member() == "Configure") {
        instance_->configure();
    } else if (msg.member() == "TriggerProperty" && msg.signature() == "s") {
        std::string property;
        msg >> property;
        if (property == "/Fcitx/im") {
            auto &imManager = instance_->inputMethodManager();
            std::vector<std::string> menuitems;
            for (const auto &item :
                 imManager.currentGroup().inputMethodList()) {
                const auto *entry = imManager.entry(item.name());
                if (!entry) {
                    continue;
                }
                menuitems.push_back(stringutils::concat(
                    "/Fcitx/im/", entry->uniqueName(), ":", entry->name(), ":",
                    IconTheme::iconName(entry->icon()), "::"));
            }
            proxy_->execMenu(menuitems);
        } else if (stringutils::startsWith(property, "/Fcitx/im/")) {
            auto imName = property.substr(10);
            timeEvent_ = instance_->eventLoop().addTimeEvent(
                CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 30000, 0,
                [this, imName](EventSourceTime *, uint64_t) {
                    instance_->setCurrentInputMethod(imName);
                    timeEvent_.reset();
                    return true;
                });
        } else if (stringutils::startsWith(property, "/Fcitx/")) {
            auto actionName = property.substr(7);
            auto *action =
                instance_->userInterfaceManager().lookupAction(actionName);
            if (!action) {
                return;
            }
            auto *ic = instance_->mostRecentInputContext();
            if (!ic) {
                return;
            }
            if (auto *menu = action->menu()) {
                std::vector<std::string> menuitems;
                for (auto *menuAction : menu->actions()) {
                    if (menuAction->isSeparator()) {
                        continue;
                    }
                    menuitems.push_back(actionToStatus(menuAction, ic));
                }
                proxy_->execMenu(menuitems);
            } else {
                // Why we need to delay the event, because we want to
                // make ic has focus.
                timeEvent_ = instance_->eventLoop().addTimeEvent(
                    CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 30000, 0,
                    [this, actionName](EventSourceTime *, uint64_t) {
                        if (auto *action =
                                instance_->userInterfaceManager().lookupAction(
                                    actionName)) {
                            if (auto *ic =
                                    instance_->mostRecentInputContext()) {
                                action->activate(ic);
                            }
                        }
                        timeEvent_.reset();
                        return true;
                    });
            }
        }
    } else if (msg.member() == "LookupTablePageUp") {
        if (auto *inputContext = lastInputContext_.get()) {
            if (auto candidateList =
                    inputContext->inputPanel().candidateList()) {
                if (auto *pageable = candidateList->toPageable()) {
                    if (pageable->hasPrev()) {
                        pageable->prev();
                        inputContext->updateUserInterface(
                            UserInterfaceComponent::InputPanel);
                    }
                }
            }
        }
    } else if (msg.member() == "LookupTablePageDown") {
        if (auto *inputContext = lastInputContext_.get()) {
            if (auto candidateList =
                    inputContext->inputPanel().candidateList()) {
                if (auto *pageable = candidateList->toPageable()) {
                    if (pageable->hasNext()) {
                        pageable->next();
                        inputContext->updateUserInterface(
                            UserInterfaceComponent::InputPanel);
                    }
                }
            }
        }
    } else if (msg.member() == "SelectCandidate" && msg.signature() == "i") {
        int idx;
        msg >> idx;
        if (!auxDownIsEmpty_) {
            idx -= 1;
        }
        if (auto *inputContext = lastInputContext_.get()) {
            if (auto candidateList =
                    inputContext->inputPanel().candidateList()) {
                const auto *candidate =
                    nthCandidateIgnorePlaceholder(*candidateList, idx);
                if (candidate) {
                    candidate->select(inputContext);
                }
            }
        }
    } else if (msg.member() == "PanelCreated") {
        if (!available_) {
            setAvailable(true);
        }
        registerAllProperties();
    }
}

void Kimpanel::msgV2Handler(dbus::Message &msg) {
    if (msg.member() == "PanelCreated2") {
        if (!available_) {
            setAvailable(true);
        }
        registerAllProperties();
    }
}

void Kimpanel::setAvailable(bool available) {
    if (available != available_) {
        available_ = available;
        instance()->userInterfaceManager().updateAvailability();
    }
}

class KimpanelFactory : public AddonFactory {
public:
    AddonInstance *create(AddonManager *manager) override {
        return new Kimpanel(manager->instance());
    }
};
} // namespace fcitx

FCITX_ADDON_FACTORY(fcitx::KimpanelFactory);
