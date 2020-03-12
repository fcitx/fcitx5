//
// Copyright (C) 2012~2017 by CSSlayer
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
#include <fcntl.h>

#include "clipboard.h"
#include "fcitx-config/iniparser.h"
#include "fcitx-utils/i18n.h"
#include "fcitx-utils/inputbuffer.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/misc_p.h"
#include "fcitx-utils/utf8.h"
#include "fcitx/addonmanager.h"
#include "fcitx/inputcontext.h"
#include "fcitx/inputcontextmanager.h"
#include "fcitx/inputpanel.h"

namespace fcitx {

class ClipboardState : public InputContextProperty {
public:
    ClipboardState(Clipboard *q) : q_(q) {}

    bool enabled_ = false;
    Clipboard *q_;

    void reset(InputContext *ic) {
        enabled_ = false;
        ic->inputPanel().reset();
        ic->updatePreedit();
        ic->updateUserInterface(UserInterfaceComponent::InputPanel);
    }
};

constexpr char threeDot[] = "\xe2\x80\xa6";

std::string ClipboardSelectionStrip(const std::string &text) {
    if (!utf8::validate(text)) {
        return text;
    }
    std::string result;
    result.reserve(text.size());
    auto iter = text.begin();
    constexpr int maxCharCount = 43;
    int count = 0;
    while (iter != text.end()) {
        auto next = utf8::nextChar(iter);
        if (std::distance(iter, next) == 1) {
            switch (*iter) {
            case '\t':
            case '\b':
            case '\f':
            case '\v':
                result += ' ';
                break;
            case '\n':
                result += "\xe2\x8f\x8e";
            case '\r':
                break;
            default:
                result += *iter;
                break;
            }
        } else {
            result.append(iter, next);
        }
        count++;
        if (count > maxCharCount) {
            result += threeDot;
            break;
        }

        iter = next;
    }
    return result;
}

class ClipboardCandidateWord : public CandidateWord {
public:
    ClipboardCandidateWord(Clipboard *q, const std::string str)
        : CandidateWord(), q_(q), str_(str) {
        Text text;
        text.append(ClipboardSelectionStrip(str));
        setText(std::move(text));
    }

    void select(InputContext *inputContext) const override {
        auto commit = str_;
        auto state = inputContext->propertyFor(&q_->factory());
        state->reset(inputContext);
        inputContext->commitString(commit);
    }

    Clipboard *q_;
    std::string str_;
};

Clipboard::Clipboard(Instance *instance)
    : instance_(instance),
      factory_([this](InputContext &) { return new ClipboardState(this); }),
      xcb_(instance_->addonManager().addon("xcb")) {
    instance_->inputContextManager().registerProperty("clipboardState",
                                                      &factory_);

    xcbCreatedCallback_ = xcb_->call<IXCBModule::addConnectionCreatedCallback>(
        [this](const std::string &name, xcb_connection_t *, int, FocusGroup *) {
            auto &callbacks = selectionCallbacks_[name];

            callbacks.emplace_back(xcb_->call<IXCBModule::addSelection>(
                name, "PRIMARY",
                [this, name](xcb_atom_t) { primaryChanged(name); }));
            callbacks.emplace_back(xcb_->call<IXCBModule::addSelection>(
                name, "CLIPBOARD",
                [this, name](xcb_atom_t) { clipboardChanged(name); }));
            primaryChanged(name);
            clipboardChanged(name);
        });
    xcbClosedCallback_ = xcb_->call<IXCBModule::addConnectionClosedCallback>(
        [this](const std::string &name, xcb_connection_t *) {
            selectionCallbacks_.erase(name);
        });

    constexpr KeySym syms[] = {
        FcitxKey_1, FcitxKey_2, FcitxKey_3, FcitxKey_4, FcitxKey_5,
        FcitxKey_6, FcitxKey_7, FcitxKey_8, FcitxKey_9, FcitxKey_0,
    };

    KeyStates states = KeyState::None;

    for (auto sym : syms) {
        selectionKeys_.emplace_back(sym, states);
    }
    eventHandlers_.emplace_back(instance_->watchEvent(
        EventType::InputContextKeyEvent, EventWatcherPhase::Default,
        [this](Event &event) {
            auto &keyEvent = static_cast<KeyEvent &>(event);
            if (keyEvent.isRelease()) {
                return;
            }
            if (keyEvent.key().checkKeyList(config_.triggerKey.value())) {
                trigger(keyEvent.inputContext());
                keyEvent.filterAndAccept();
                return;
            }
        }));

    auto reset = [this](Event &event) {
        auto &icEvent = static_cast<InputContextEvent &>(event);
        auto state = icEvent.inputContext()->propertyFor(&factory_);
        if (state->enabled_) {
            state->reset(icEvent.inputContext());
        }
    };
    eventHandlers_.emplace_back(instance_->watchEvent(
        EventType::InputContextFocusOut, EventWatcherPhase::Default, reset));
    eventHandlers_.emplace_back(instance_->watchEvent(
        EventType::InputContextReset, EventWatcherPhase::Default, reset));
    eventHandlers_.emplace_back(
        instance_->watchEvent(EventType::InputContextSwitchInputMethod,
                              EventWatcherPhase::Default, reset));
    eventHandlers_.emplace_back(instance_->watchEvent(
        EventType::InputContextKeyEvent, EventWatcherPhase::PreInputMethod,
        [this](Event &event) {
            auto &keyEvent = static_cast<KeyEvent &>(event);
            auto inputContext = keyEvent.inputContext();
            auto state = inputContext->propertyFor(&factory_);
            if (!state->enabled_) {
                return;
            }

            // make sure no one else will handle it
            keyEvent.filter();
            if (keyEvent.isRelease()) {
                return;
            }

            auto candidateList = inputContext->inputPanel().candidateList();
            if (candidateList) {
                int idx = keyEvent.key().keyListIndex(selectionKeys_);
                if (idx >= 0) {
                    keyEvent.accept();
                    if (idx < candidateList->size()) {
                        candidateList->candidate(idx).select(inputContext);
                    }
                    return;
                }
                if (keyEvent.key().check(FcitxKey_space)) {
                    keyEvent.accept();
                    if (candidateList->size() > 0) {
                        candidateList->candidate(0).select(inputContext);
                    }
                    return;
                }

                if (keyEvent.key().checkKeyList(
                        instance_->globalConfig().defaultPrevPage())) {
                    auto pageable = candidateList->toPageable();
                    if (!pageable->hasPrev()) {
                        if (pageable->usedNextBefore()) {
                            event.accept();
                            return;
                        }
                    } else {
                        event.accept();
                        pageable->prev();
                        inputContext->updateUserInterface(
                            UserInterfaceComponent::InputPanel);
                        return;
                    }
                }

                if (keyEvent.key().checkKeyList(
                        instance_->globalConfig().defaultNextPage())) {
                    keyEvent.filterAndAccept();
                    candidateList->toPageable()->next();
                    inputContext->updateUserInterface(
                        UserInterfaceComponent::InputPanel);
                    return;
                }
            }

            // and by pass all modifier
            if (keyEvent.key().isModifier() || keyEvent.key().hasModifier()) {
                return;
            }
            if (keyEvent.key().check(FcitxKey_Escape)) {
                keyEvent.accept();
                state->reset(inputContext);
                return;
            }
            event.accept();

            updateUI(inputContext);
        }));
    reloadConfig();
}

Clipboard::~Clipboard() {}

void Clipboard::trigger(InputContext *inputContext) {
    auto state = inputContext->propertyFor(&factory_);
    state->enabled_ = true;
    updateUI(inputContext);
}
void Clipboard::updateUI(InputContext *inputContext) {
    inputContext->inputPanel().reset();

    auto candidateList = std::make_unique<CommonCandidateList>();
    candidateList->setPageSize(instance_->globalConfig().defaultPageSize());

    // Append first item from history_.
    auto iter = history_.begin();
    if (iter != history_.end()) {
        candidateList->append<ClipboardCandidateWord>(this, *iter);
        iter++;
    }
    // Append primary_, but check duplication first.
    if (!primary_.empty()) {
        bool dup = false;
        for (const auto &s : history_) {
            if (s == primary_) {
                dup = true;
                break;
            }
        }
        if (!dup) {
            candidateList->append<ClipboardCandidateWord>(this, primary_);
        }
    }
    // If primary_ is appended, it might squeeze one space out.
    for (; iter != history_.end(); iter++) {
        if (candidateList->totalSize() >= config_.numOfEntries.value()) {
            break;
        }
        candidateList->append<ClipboardCandidateWord>(this, *iter);
    }
    candidateList->setSelectionKey(selectionKeys_);
    candidateList->setLayoutHint(CandidateLayoutHint::Vertical);

    Text auxUp(_("Clipboard:"));
    if (!candidateList->totalSize()) {
        Text auxDown(_("No clipboard history."));
        inputContext->inputPanel().setAuxDown(auxDown);
    }
    inputContext->inputPanel().setCandidateList(std::move(candidateList));
    inputContext->inputPanel().setAuxUp(auxUp);
    inputContext->updatePreedit();
    inputContext->updateUserInterface(UserInterfaceComponent::InputPanel);
}

void Clipboard::reloadConfig() { readAsIni(config_, "conf/clipboard.conf"); }

void Clipboard::primaryChanged(const std::string &name) {
    primaryCallback_ = xcb_->call<IXCBModule::convertSelection>(
        name, "PRIMARY", "",
        [this](xcb_atom_t, const char *data, size_t length) {
            if (!data) {
                primary_.clear();
            } else {
                std::string str(data, length);
                primary_ = std::move(str);
            }
            primaryCallback_.reset();
        });
}

void Clipboard::clipboardChanged(const std::string &name) {
    clipboardCallback_ = xcb_->call<IXCBModule::convertSelection>(
        name, "CLIPBOARD", "",
        [this](xcb_atom_t, const char *data, size_t length) {
            if (data) {
                std::string str(data, length);
                if (!history_.insert(str)) {
                    history_.moveToTop(str);
                }
                while (history_.size() && static_cast<int>(history_.size()) >
                                              config_.numOfEntries.value()) {
                    history_.pop();
                }
            }
            clipboardCallback_.reset();
        });
}

std::string Clipboard::primary(const InputContext *) {
    // TODO: per ic
    return primary_;
}

std::string Clipboard::clipboard(const InputContext *) {
    // TODO: per ic
    if (history_.empty()) {
        return "";
    }
    return history_.front();
}

class ClipboardModuleFactory : public AddonFactory {
    AddonInstance *create(AddonManager *manager) override {
        return new Clipboard(manager->instance());
    }
};
} // namespace fcitx

FCITX_ADDON_FACTORY(fcitx::ClipboardModuleFactory);
