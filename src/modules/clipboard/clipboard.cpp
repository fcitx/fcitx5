/*
 * SPDX-FileCopyrightText: 2012-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "clipboard.h"
#include <cstdint>
#include <limits>
#include <unordered_set>
#include "fcitx-utils/event.h"
#include "fcitx-utils/i18n.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/misc_p.h"
#include "fcitx-utils/utf8.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addonmanager.h"
#include "fcitx/inputcontext.h"
#include "fcitx/inputcontextmanager.h"
#include "fcitx/inputpanel.h"
#include "clipboardentry.h"

namespace fcitx {

namespace {

constexpr uint64_t oneSecond = 1000000ULL;

bool shouldClearPassword(const ClipboardEntry &entry, uint64_t life) {
    if (entry.passwordTimestamp == 0 || life == 0) {
        // Not password.
        return false;
    }
    // Allow 0.5 second skew.
    return (entry.passwordTimestamp + life * oneSecond - oneSecond / 2) <=
           now(CLOCK_MONOTONIC);
}

} // namespace

FCITX_DEFINE_LOG_CATEGORY(clipboard_log, "clipboard");

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
    ClipboardCandidateWord(Clipboard *q, const std::string &str, bool password)
        : q_(q), str_(str) {
        Text text;
        if (password && !*q->config().showPassword) {
            auto length = utf8::length(str);
            length = std::min(length, static_cast<size_t>(8));
            std::string dot;
            dot.reserve(length * 3);
            while (length != 0) {
                dot += "\xe2\x80\xa2";
                length -= 1;
            }
            text.append(dot);
            setComment(Text{_("<Passowrd>")});
        } else {
            text.append(ClipboardSelectionStrip(str));
        }
        setText(std::move(text));
    }

    void select(InputContext *inputContext) const override {
        auto *state = inputContext->propertyFor(&q_->factory());
        inputContext->commitString(str_);
        state->reset(inputContext);
    }

    Clipboard *q_;
    std::string str_;
};

Clipboard::Clipboard(Instance *instance)
    : instance_(instance),
      factory_([this](InputContext &) { return new ClipboardState(this); }) {
    instance_->inputContextManager().registerProperty("clipboardState",
                                                      &factory_);
#ifdef ENABLE_X11
    if (auto *xcb = this->xcb()) {
        xcbCreatedCallback_ =
            xcb->call<IXCBModule::addConnectionCreatedCallback>(
                [this](const std::string &name, xcb_connection_t *, int,
                       FocusGroup *) {
                    xcbClipboards_[name].reset(new XcbClipboard(this, name));
                });
        xcbClosedCallback_ = xcb->call<IXCBModule::addConnectionClosedCallback>(
            [this](const std::string &name, xcb_connection_t *) {
                xcbClipboards_.erase(name);
            });
    }
#endif
#ifdef WAYLAND_FOUND
    if (auto *wayland = this->wayland()) {
        waylandCreatedCallback_ =
            wayland->call<IWaylandModule::addConnectionCreatedCallback>(
                [this](const std::string &name, wl_display *display,
                       FocusGroup *) {
                    waylandClipboards_[name].reset(
                        new WaylandClipboard(this, name, display));
                });
        waylandClosedCallback_ =
            wayland->call<IWaylandModule::addConnectionClosedCallback>(
                [this](const std::string &name, wl_display *) {
                    waylandClipboards_.erase(name);
                });
    }
#endif

    constexpr KeySym syms[] = {
        FcitxKey_1, FcitxKey_2, FcitxKey_3, FcitxKey_4, FcitxKey_5,
        FcitxKey_6, FcitxKey_7, FcitxKey_8, FcitxKey_9, FcitxKey_0,
    };

    KeyStates states = KeyState::NoState;

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
            if (keyEvent.key().checkKeyList(config_.pastePrimaryKey.value())) {
                keyEvent.inputContext()->commitString(
                    primary(keyEvent.inputContext()));
                keyEvent.filterAndAccept();
                return;
            }
        }));

    auto reset = [this](Event &event) {
        auto &icEvent = static_cast<InputContextEvent &>(event);
        auto *state = icEvent.inputContext()->propertyFor(&factory_);
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
            auto *inputContext = keyEvent.inputContext();
            auto *state = inputContext->propertyFor(&factory_);
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
                int idx = keyEvent.key().digitSelection();
                if (idx >= 0) {
                    keyEvent.accept();
                    if (idx < candidateList->size()) {
                        candidateList->candidate(idx).select(inputContext);
                    }
                    return;
                }
                if (keyEvent.key().check(FcitxKey_space) ||
                    keyEvent.key().check(FcitxKey_Return) ||
                    keyEvent.key().check(FcitxKey_KP_Enter)) {
                    keyEvent.accept();
                    if (!candidateList->empty() &&
                        candidateList->cursorIndex() >= 0) {
                        candidateList->candidate(candidateList->cursorIndex())
                            .select(inputContext);
                    }
                    return;
                }

                if (keyEvent.key().checkKeyList(
                        instance_->globalConfig().defaultPrevPage())) {
                    auto *pageable = candidateList->toPageable();
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

                if (keyEvent.key().checkKeyList(
                        instance_->globalConfig().defaultPrevCandidate())) {
                    keyEvent.filterAndAccept();
                    candidateList->toCursorMovable()->prevCandidate();
                    inputContext->updateUserInterface(
                        UserInterfaceComponent::InputPanel);
                    return;
                }

                if (keyEvent.key().checkKeyList(
                        instance_->globalConfig().defaultNextCandidate())) {
                    keyEvent.filterAndAccept();
                    candidateList->toCursorMovable()->nextCandidate();
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
            if (keyEvent.key().check(FcitxKey_Delete) ||
                keyEvent.key().check(FcitxKey_BackSpace)) {
                keyEvent.accept();
                history_.clear();
                primary_.clear();
                state->reset(inputContext);
                return;
            }
            event.accept();

            updateUI(inputContext);
        }));
    clearPasswordTimer_ = instance_->eventLoop().addTimeEvent(
        CLOCK_MONOTONIC, now(CLOCK_MONOTONIC), 0,
        [this](EventSourceTime *, uint64_t) {
            refreshPasswordTimer();
            return true;
        });
    reloadConfig();
}

Clipboard::~Clipboard() {}

void Clipboard::reloadConfig() {
    readAsIni(config_, configFile);
    refreshPasswordTimer();
}

void Clipboard::trigger(InputContext *inputContext) {
    auto *state = inputContext->propertyFor(&factory_);
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
        candidateList->append<ClipboardCandidateWord>(this, iter->text,
                                                      iter->passwordTimestamp);
        iter++;
    }
    // Append primary_, but check duplication first.
    if (!primary_.empty()) {
        if (!history_.contains(primary_)) {
            candidateList->append<ClipboardCandidateWord>(
                this, primary_.text, primary_.passwordTimestamp);
        }
    }
    // If primary_ is appended, it might squeeze one space out.
    for (; iter != history_.end(); iter++) {
        if (candidateList->totalSize() >= config_.numOfEntries.value()) {
            break;
        }
        candidateList->append<ClipboardCandidateWord>(this, iter->text,
                                                      iter->passwordTimestamp);
    }
    candidateList->setSelectionKey(selectionKeys_);
    candidateList->setLayoutHint(CandidateLayoutHint::Vertical);

    Text auxUp(_("Clipboard (Press BackSpace/Delete to clear history):"));
    if (!candidateList->totalSize()) {
        Text auxDown(_("No clipboard history."));
        inputContext->inputPanel().setAuxDown(auxDown);
    } else {
        candidateList->setGlobalCursorIndex(0);
    }
    inputContext->inputPanel().setCandidateList(std::move(candidateList));
    inputContext->inputPanel().setAuxUp(auxUp);
    inputContext->updatePreedit();
    inputContext->updateUserInterface(UserInterfaceComponent::InputPanel);
}

void Clipboard::setPrimary(const std::string &name, const std::string &str) {
    setPrimaryV2(name, str, false);
}

void Clipboard::setClipboard(const std::string &name, const std::string &str) {
    setClipboardV2(name, str, false);
}

void Clipboard::setPrimaryV2(const std::string &name, const std::string &str,
                             bool password) {
    setPrimaryEntry(name,
                    ClipboardEntry{.text = str,
                                   .passwordTimestamp =
                                       (password ? now(CLOCK_MONOTONIC) : 0)});
}

void Clipboard::setClipboardV2(const std::string &name, const std::string &str,
                               bool password) {
    setClipboardEntry(
        name, ClipboardEntry{.text = str,
                             .passwordTimestamp =
                                 (password ? now(CLOCK_MONOTONIC) : 0)});
}

void Clipboard::setPrimaryEntry(const std::string &name, ClipboardEntry entry) {
    FCITX_UNUSED(name);
    if (!utf8::validate(entry.text)) {
        return;
    }
    primary_ = std::move(entry);
    if (entry.passwordTimestamp) {
        refreshPasswordTimer();
    }
}

void Clipboard::setClipboardEntry(const std::string &name,
                                  const ClipboardEntry &entry) {
    FCITX_UNUSED(name);
    if (entry.text.empty() || !utf8::validate(entry.text)) {
        return;
    }

    if (!history_.pushFront(entry)) {
        history_.moveToTop(entry);
    }
    if (history_.front().passwordTimestamp || entry.passwordTimestamp) {
        history_.front().passwordTimestamp = std::max(
            entry.passwordTimestamp, history_.front().passwordTimestamp);
    }
    while (!history_.empty() &&
           static_cast<int>(history_.size()) > config_.numOfEntries.value()) {
        history_.pop();
    }
    if (entry.passwordTimestamp) {
        refreshPasswordTimer();
    }
}

std::string Clipboard::primary(const InputContext * /*unused*/) const {
    // TODO: per ic
    return primary_.text;
}

std::string Clipboard::clipboard(const InputContext * /*unused*/) const {
    // TODO: per ic
    if (history_.empty()) {
        return "";
    }
    return history_.front().text;
}

void Clipboard::refreshPasswordTimer() {
    if (*config_.clearPasswordAfter == 0) {
        FCITX_CLIPBOARD_DEBUG() << "Disable Password Clearing Timer.";
        clearPasswordTimer_->setEnabled(false);
        return;
    }

    uint64_t minTimestamp = std::numeric_limits<uint64_t>::max();

    if (shouldClearPassword(primary_, *config_.clearPasswordAfter)) {
        FCITX_CLIPBOARD_DEBUG() << "Clear password in primary.";
        primary_.clear();
    } else if (primary_.passwordTimestamp) {
        minTimestamp = std::min(minTimestamp, primary_.passwordTimestamp);
    }

    // Not efficient, but we don't have lots of entries anyway.
    std::unordered_set<ClipboardEntry> needRemove;
    for (const auto &entry : history_) {
        if (shouldClearPassword(entry, *config_.clearPasswordAfter)) {
            needRemove.insert(entry);
        } else if (entry.passwordTimestamp) {
            minTimestamp = std::min(minTimestamp, entry.passwordTimestamp);
        }
    }
    FCITX_CLIPBOARD_DEBUG() << "Clear " << needRemove.size()
                            << " password(s) in clipboard history.";
    for (const auto &entry : needRemove) {
        history_.remove(entry);
    }

    if (minTimestamp != std::numeric_limits<uint64_t>::max()) {
        clearPasswordTimer_->setTime(minTimestamp +
                                     oneSecond * (*config_.clearPasswordAfter));
        FCITX_CLIPBOARD_DEBUG()
            << "Password Clearing Timer will be triggered after: "
            << clearPasswordTimer_->time() - now(CLOCK_MONOTONIC);
        clearPasswordTimer_->setOneShot();
    }
}

class ClipboardModuleFactory : public AddonFactory {
    AddonInstance *create(AddonManager *manager) override {
        return new Clipboard(manager->instance());
    }
};
} // namespace fcitx

FCITX_ADDON_FACTORY_V2(clipboard, fcitx::ClipboardModuleFactory);
