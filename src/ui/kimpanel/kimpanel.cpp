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

#include "kimpanel.h"
#include "dbus_public.h"
#include "fcitx-utils/dbus/objectvtable.h"
#include "fcitx-utils/dbus/servicewatcher.h"
#include "fcitx-utils/utf8.h"
#include "fcitx/addonmanager.h"
#include "fcitx/inputcontext.h"
#include "fcitx/instance.h"
#include "fcitx/userinterfacemanager.h"

namespace fcitx {

class KimpanelProxy : public dbus::ObjectVTable<KimpanelProxy> {

public:
    KimpanelProxy(Kimpanel *parent, dbus::Bus *bus) : bus_(bus) {
        slot_.reset(bus_->addMatch("type='signal',sender='org.kde.impanel',"
                                   "interface='org.kde.impanel'",
                                   [parent](dbus::Message msg) {
                                       parent->msgV1Handler(msg);
                                       return true;
                                   }));
        slot2_.reset(bus_->addMatch("type='signal',sender='org.kde.impanel',"
                                    "interface='org.kde.impanel2'",
                                    [parent](dbus::Message msg) {
                                        parent->msgV2Handler(msg);
                                        return true;
                                    }));
    }

    void updateCursor(InputContext *inputContext) {
        auto msg = bus_->createMethodCall("org.kde.impanel", "/org/kde/impanel",
                                          "org.kde.impanel2", "SetSpotRect");

        int32_t x = inputContext->cursorRect().left(),
                y = inputContext->cursorRect().top(),
                w = inputContext->cursorRect().width(),
                h = inputContext->cursorRect().height();

        msg << x << y << w << h;
        msg.send();
    }

public:
    FCITX_OBJECT_VTABLE_SIGNAL(execDialog, "ExecDialog", "s");
    FCITX_OBJECT_VTABLE_SIGNAL(execMenu, "ExecMenu", "as");
    FCITX_OBJECT_VTABLE_SIGNAL(registerProperties, "RegisterProperties", "as");
    FCITX_OBJECT_VTABLE_SIGNAL(updateProperty, "UpdateProperty", "s");
    FCITX_OBJECT_VTABLE_SIGNAL(removeProperty, "RemoveProperty", "s");
    FCITX_OBJECT_VTABLE_SIGNAL(showAux, "ShowAux", "b");
    FCITX_OBJECT_VTABLE_SIGNAL(showPreedit, "ShowPreedit", "b");
    FCITX_OBJECT_VTABLE_SIGNAL(showLookupTable, "ShowLookupTable", "b");
    FCITX_OBJECT_VTABLE_SIGNAL(updateLookupTable, "UpdateLookupTable",
                               "asasbb");
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
    : UserInterface(), instance_(instance),
      bus_(instance_->addonManager().addon("dbus")->call<IDBusModule::bus>()),
      watcher_(*bus_) {
    entry_.reset(watcher_.watchService(
        "org.kde.impanel", [this](const std::string &, const std::string &,
                                  const std::string &newOwner) {
            setAvailable(!newOwner.empty());
        }));

    if (!bus_->serviceOwner("org.kde.impanel", 0).empty()) {
        available_ = true;
    }
}

Kimpanel::~Kimpanel() {}

void Kimpanel::suspend() {
    proxy_.reset();
    eventHandlers_.clear();
}

void Kimpanel::init() {
    std::vector<std::string> props;
    props.push_back("/Fcitx/im:Fcitx:fcitx:");
    proxy_->registerProperties(props);
}

void Kimpanel::resume() {
    proxy_ = std::make_unique<KimpanelProxy>(this, bus_);
    bus_->addObjectVTable("/kimpanel", "org.kde.kimpanel.inputmethod", *proxy_);
    if (available_) {
        init();
    }

    auto check = [this](Event &event) {
        if (!proxy_) {
            return;
        }
        auto &icEvent = static_cast<InputContextEvent &>(event);
        auto inputContext = icEvent.inputContext();
        if (inputContext->hasFocus()) {
            proxy_->updateCursor(inputContext);
        }
    };
    eventHandlers_.emplace_back(
        instance_->watchEvent(EventType::InputContextCursorRectChanged,
                              EventWatcherPhase::Default, check));
    eventHandlers_.emplace_back(instance_->watchEvent(
        EventType::InputContextFocusIn, EventWatcherPhase::Default, check));
}

void Kimpanel::update(UserInterfaceComponent component,
                      InputContext *inputContext) {
    if (component == UserInterfaceComponent::InputPanel) {
        updateInputPanel(inputContext);
    }
}

void Kimpanel::updateInputPanel(InputContext *inputContext) {
    lastInputContext_ = inputContext->watch();
    auto instance = this->instance();
    auto &inputPanel = inputContext->inputPanel();

    auto preedit = instance->outputFilter(inputContext, inputPanel.preedit());
    auto auxUp = instance->outputFilter(inputContext, inputPanel.auxUp());
    auto preeditString = preedit.toString();
    auto auxUpString = auxUp.toString();
    if (preeditString.size() || auxUpString.size()) {
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
        auxDownString.size() || (candidateList && candidateList->size());
    if (visible) {
        std::vector<std::string> labels;
        std::vector<std::string> texts;
        std::vector<std::string> attrs;
        if (auxDownString.size()) {
            labels.emplace_back("");
            texts.push_back(auxDownString);
            attrs.emplace_back("");
        }
        for (int i = 0, e = candidateList->size(); i < e; i++) {
            auto candidate = candidateList->candidate(i);
            if (candidate->isPlaceHolder()) {
                continue;
            }
            Text labelText = candidate->hasCustomLabel()
                                 ? candidate->customLabel()
                                 : candidateList->label(i);

            labelText = instance->outputFilter(inputContext, labelText);
            labels.push_back(labelText.toString());
            auto candidateText =
                instance->outputFilter(inputContext, candidate->text());
            texts.push_back(candidateText.toString());
            attrs.emplace_back("");
        }
        msg << labels << texts << attrs;
        bool hasPrev = false, hasNext = false;
        if (auto pageable = candidateList->toPageable()) {
            hasPrev = pageable->hasPrev();
            hasNext = pageable->hasNext();
        }
        int pos = candidateList->cursorIndex();
        int layout = static_cast<int>(candidateList->layoutHint());

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

void Kimpanel::msgV1Handler(dbus::Message &msg) {
    if (msg.member() == "Exit") {
        instance_->exit();
    } else if (msg.member() == "ReloadConfig") {
        instance_->reloadConfig();
    } else if (msg.member() == "Restart") {
        instance_->restart();
    } else if (msg.member() == "Configure") {
        instance_->configure();
    } else if (msg.member() == "TriggerProperty") {
        // TODO
    } else if (msg.member() == "LookupTablePageUp") {
        if (auto inputContext = lastInputContext_.get()) {
            if (auto candidateList =
                    inputContext->inputPanel().candidateList()) {
                if (auto pageable = candidateList->toPageable()) {
                    if (pageable->hasPrev()) {
                        pageable->prev();
                        inputContext->updateUserInterface(
                            UserInterfaceComponent::InputPanel);
                    }
                }
            }
        }
    } else if (msg.member() == "LookupTablePageDown") {
        if (auto inputContext = lastInputContext_.get()) {
            if (auto candidateList =
                    inputContext->inputPanel().candidateList()) {
                if (auto pageable = candidateList->toPageable()) {
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
        if (auto inputContext = lastInputContext_.get()) {
            if (auto candidateList =
                    inputContext->inputPanel().candidateList()) {
                if (idx >= 0 && idx < candidateList->size()) {
                    candidateList->candidate(idx)->select(inputContext);
                }
            }
        }
    }
}

void Kimpanel::msgV2Handler(dbus::Message &) {}

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
}

FCITX_ADDON_FACTORY(fcitx::KimpanelFactory);
