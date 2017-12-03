/*
 * Copyright (C) 2017~2017 by CSSlayer
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
#include "config.h"

#include "fcitx-config/iniparser.h"
#include "fcitx-utils/charutils.h"
#include "fcitx-utils/i18n.h"
#include "fcitx-utils/inputbuffer.h"
#include "fcitx-utils/standardpath.h"
#include "fcitx-utils/utf8.h"
#include "fcitx/addonmanager.h"
#include "fcitx/candidatelist.h"
#include "fcitx/inputcontextmanager.h"
#include "fcitx/inputpanel.h"
#include "quickphrase.h"
#include <fcntl.h>

namespace fcitx {

class QuickPhraseState : public InputContextProperty {
public:
    QuickPhraseState(QuickPhrase *q) : q_(q) { buffer_.setMaxSize(30); }

    bool enabled_ = false;
    InputBuffer buffer_;
    QuickPhrase *q_;

    bool typed_ = false;
    std::string text_;
    std::string prefix_;
    std::string str_;
    std::string alt_;
    Key key_;

    void reset(InputContext *ic) {
        enabled_ = false;
        typed_ = false;
        text_.clear();
        buffer_.clear();
        buffer_.shrinkToFit();
        prefix_.clear();
        str_.clear();
        alt_.clear();
        key_ = Key(FcitxKey_None);
        ic->inputPanel().reset();
        ic->updatePreedit();
        ic->updateUserInterface(UserInterfaceComponent::InputPanel);
    }
};

QuickPhrase::QuickPhrase(Instance *instance)
    : instance_(instance),
      factory_([this](InputContext &) { return new QuickPhraseState(this); }) {
    instance_->inputContextManager().registerProperty("quickphraseState",
                                                      &factory_);
    eventHandlers_.emplace_back(instance_->watchEvent(
        EventType::InputContextKeyEvent, EventWatcherPhase::Default,
        [this](Event &event) {
            auto &keyEvent = static_cast<KeyEvent &>(event);
            if (keyEvent.isRelease()) {
                return;
            }
            if (keyEvent.key().checkKeyList(config_.triggerKey.value())) {
                trigger(keyEvent.inputContext(), "", "", "", "",
                        Key{FcitxKey_None});
                keyEvent.filterAndAccept();
                updateUI(keyEvent.inputContext());
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
                        candidateList->candidate(idx)->select(inputContext);
                    }
                    return;
                }

                if (keyEvent.key().check(FcitxKey_space) &&
                    candidateList->size()) {
                    keyEvent.accept();
                    candidateList->candidate(0)->select(inputContext);
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
            } else if (keyEvent.key().check(FcitxKey_Return)) {
                keyEvent.accept();
                if (!state->typed_ && state->buffer_.empty() &&
                    state->str_.size() && state->alt_.size()) {
                    inputContext->commitString(state->alt_);
                } else {
                    if (state->buffer_.size() + state->prefix_.size()) {
                        inputContext->commitString(state->prefix_ +
                                                   state->buffer_.userInput());
                    }
                }
                state->reset(inputContext);
                return;
            } else if (keyEvent.key().check(FcitxKey_BackSpace)) {
                if (state->buffer_.empty()) {
                    state->reset(inputContext);
                } else {
                    if (state->buffer_.backspace()) {
                        if (state->buffer_.empty()) {
                            state->reset(inputContext);
                        } else {
                            updateUI(inputContext);
                        }
                    }
                }
                keyEvent.accept();
                return;
            } else if (!state->typed_ && state->str_.size() &&
                       state->buffer_.empty() &&
                       keyEvent.key().check(state->key_)) {
                keyEvent.accept();
                inputContext->commitString(state->str_);
                state->reset(inputContext);
                return;
            }

            // check compose first.
            auto compose =
                instance_->processCompose(inputContext, keyEvent.key().sym());

            // compose is invalid, ignore it.
            if (compose == FCITX_INVALID_COMPOSE_RESULT) {
                return event.accept();
            }

            if (compose) {
                state->buffer_.type(compose);
            } else {
                state->buffer_.type(Key::keySymToUnicode(keyEvent.key().sym()));
            }
            state->typed_ = true;
            event.accept();

            updateUI(inputContext);
        }));

    reloadConfig();
}

QuickPhrase::~QuickPhrase() {}

class QuickPhraseCandidateWord : public CandidateWord {
public:
    QuickPhraseCandidateWord(QuickPhrase *q, Text text)
        : CandidateWord(std::move(text)), q_(q) {}

    void select(InputContext *inputContext) const override {
        auto commit = text().stringAt(0);
        auto state = inputContext->propertyFor(&q_->factory());
        state->reset(inputContext);
        inputContext->inputPanel().reset();
        inputContext->updatePreedit();
        inputContext->updateUserInterface(UserInterfaceComponent::InputPanel,
                                          true);
        inputContext->commitString(commit);
    }

    QuickPhrase *q_;
};

void QuickPhrase::updateUI(InputContext *inputContext) {
    auto state = inputContext->propertyFor(&factory_);
    inputContext->inputPanel().reset();
    if (!state->buffer_.empty()) {
        auto start = map_.lower_bound(state->buffer_.userInput());
        auto end = map_.end();
        auto candidateList = std::make_unique<CommonCandidateList>();
        candidateList->setPageSize(instance_->globalConfig().defaultPageSize());
        for (; start != end; start++) {
            if (!stringutils::startsWith(start->first,
                                         state->buffer_.userInput())) {
                break;
            }
            Text text;
            text.append(start->second);
            text.append(" ");
            text.append(start->first.substr(state->buffer_.userInput().size()));
            candidateList->append(
                new QuickPhraseCandidateWord(this, std::move(text)));
        }
        candidateList->setSelectionKey(selectionKeys_);
        inputContext->inputPanel().setCandidateList(std::move(candidateList));
    }
    Text preedit;
    if (state->prefix_.size()) {
        preedit.append(state->prefix_);
    }
    preedit.append(state->buffer_.userInput());
    if (state->buffer_.size()) {
        preedit.setCursor(state->prefix_.size() +
                          state->buffer_.cursorByChar());
    }

    Text auxUp(_("Quick Phrase: "));
    if (!state->typed_) {
        auxUp.append(state->text_);
    }
    // inputContext->inputPanel().setClientPreedit(preedit);
    inputContext->inputPanel().setAuxUp(auxUp);
    inputContext->inputPanel().setPreedit(preedit);
    inputContext->updatePreedit();
    inputContext->updateUserInterface(UserInterfaceComponent::InputPanel);
}

void QuickPhrase::reloadConfig() {
    map_.clear();
    auto file = StandardPath::global().open(StandardPath::Type::PkgData,
                                            "data/QuickPhrase.mb", O_RDONLY);
    auto files = StandardPath::global().multiOpen(
        StandardPath::Type::PkgData, "data/quickphrase.d/", O_RDONLY,
        filter::Suffix(".mb"));
    auto disableFiles = StandardPath::global().multiOpen(
        StandardPath::Type::PkgData, "quickphrase.d/", O_RDONLY,
        filter::Suffix(".mb.disable"));
    if (file.fd() >= 0) {
        load(file);
    }

    for (auto &p : files) {
        if (disableFiles.count(p.first)) {
            continue;
        }
        load(p.second);
    }
    readAsIni(config_, "conf/quickphrase.conf");

    selectionKeys_.clear();
    KeySym syms[] = {
        FcitxKey_1, FcitxKey_2, FcitxKey_3, FcitxKey_4, FcitxKey_5,
        FcitxKey_6, FcitxKey_7, FcitxKey_8, FcitxKey_9, FcitxKey_0,
    };

    KeyStates states;
    switch (config_.chooseModifier.value()) {
    case QuickPhraseChooseModifier::Alt:
        states = KeyState::Alt;
        break;
    case QuickPhraseChooseModifier::Control:
        states = KeyState::Ctrl;
        break;
    case QuickPhraseChooseModifier::Super:
        states = KeyState::Super;
        break;
    default:
        break;
    }

    for (auto sym : syms) {
        selectionKeys_.emplace_back(sym, states);
    }
}

enum class UnescapeState { NORMAL, ESCAPE };

bool _unescape_string(std::string &str, bool unescapeQuote) {
    if (str.empty()) {
        return true;
    }

    size_t i = 0;
    size_t j = 0;
    UnescapeState state = UnescapeState::NORMAL;
    do {
        switch (state) {
        case UnescapeState::NORMAL:
            if (str[i] == '\\') {
                state = UnescapeState::ESCAPE;
            } else {
                str[j] = str[i];
                j++;
            }
            break;
        case UnescapeState::ESCAPE:
            if (str[i] == '\\') {
                str[j] = '\\';
                j++;
            } else if (str[i] == 'n') {
                str[j] = '\n';
                j++;
            } else if (str[i] == '\"' && unescapeQuote) {
                str[j] = '\"';
                j++;
            } else {
                return false;
            }
            state = UnescapeState::NORMAL;
            break;
        }
    } while (str[i++]);
    str.resize(j - 1);
    return true;
}

typedef std::unique_ptr<FILE, decltype(&fclose)> ScopedFILE;
void QuickPhrase::load(StandardPathFile &file) {
    FILE *f = fdopen(file.fd(), "rb");
    if (!f) {
        return;
    }
    ScopedFILE fp{f, fclose};
    file.release();

    char *buf = nullptr;
    size_t len = 0;
    while (getline(&buf, &len, fp.get()) != -1) {
        std::string strBuf(buf);

        auto pair = stringutils::trimInplace(strBuf);
        std::string::size_type start = pair.first, end = pair.second;
        if (start == end) {
            continue;
        }
        std::string text(strBuf.begin() + start, strBuf.begin() + end);
        if (!utf8::validate(text)) {
            continue;
        }

        auto pos = text.find_first_of(FCITX_WHITESPACE);
        if (pos == std::string::npos) {
            continue;
        }

        auto word = text.find_first_not_of(FCITX_WHITESPACE, pos);
        if (word == std::string::npos) {
            continue;
        }

        if (text.back() == '\"' &&
            (text[word] != '\"' || word + 1 != text.size())) {
            continue;
        }

        std::string key(text.begin(), text.begin() + pos);
        std::string wordString;

        bool escapeQuote;
        if (text.back() == '\"' && text[word] == '\"') {
            wordString = text.substr(word + 1, text.size() - word - 1);
            escapeQuote = true;
        } else {
            wordString = text.substr(word);
            escapeQuote = false;
        }
        _unescape_string(wordString, escapeQuote);

        map_.emplace(std::move(key), std::move(wordString));
    }

    free(buf);
}

void QuickPhrase::trigger(InputContext *ic, const std::string &text,
                          const std::string &prefix, const std::string &str,
                          const std::string &alt, const Key &key) {
    auto state = ic->propertyFor(&factory_);
    state->typed_ = false;
    state->enabled_ = true;
    state->text_ = text;
    state->prefix_ = prefix;
    state->str_ = str;
    state->alt_ = alt;
    state->key_ = key;
    updateUI(ic);
}

class QuickPhraseModuleFactory : public AddonFactory {
    AddonInstance *create(AddonManager *manager) override {
        return new QuickPhrase(manager->instance());
    }
};
}

FCITX_ADDON_FACTORY(fcitx::QuickPhraseModuleFactory)
