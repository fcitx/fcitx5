/*
 * SPDX-FileCopyrightText: 2020-2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "quickphraseprovider.h"
#include <fcntl.h>
#include <fcitx-utils/utf8.h>
#include <fcitx/inputmethodentry.h>
#include "fcitx-utils/standardpath.h"
#include "fcitx-utils/stringutils.h"
#include "quickphrase.h"
#include "spell_public.h"

namespace fcitx {

bool BuiltInQuickPhraseProvider::populate(
    InputContext *, const std::string &userInput,
    const QuickPhraseAddCandidateCallback &addCandidate) {
    auto start = map_.lower_bound(userInput);
    auto end = map_.end();

    for (; start != end; start++) {
        if (!stringutils::startsWith(start->first, userInput)) {
            break;
        }
        addCandidate(start->second,
                     stringutils::concat(start->second, " ",
                                         start->first.substr(userInput.size())),
                     QuickPhraseAction::Commit);
    }
    return true;
}
void BuiltInQuickPhraseProvider::reloadConfig() {

    map_.clear();
    auto file = StandardPath::global().open(StandardPath::Type::PkgData,
                                            "data/QuickPhrase.mb", O_RDONLY);
    auto files = StandardPath::global().multiOpen(
        StandardPath::Type::PkgData, "data/quickphrase.d/", O_RDONLY,
        filter::Suffix(".mb"));
    auto disableFiles = StandardPath::global().multiOpen(
        StandardPath::Type::PkgData, "data/quickphrase.d/", O_RDONLY,
        filter::Suffix(".mb.disable"));
    if (file.fd() >= 0) {
        load(file);
    }

    for (auto &p : files) {
        if (disableFiles.count(stringutils::concat(p.first, ".disable"))) {
            continue;
        }
        load(p.second);
    }
}

void BuiltInQuickPhraseProvider::load(StandardPathFile &file) {
    UniqueFilePtr fp{fdopen(file.fd(), "rb")};
    if (!fp) {
        return;
    }
    file.release();

    UniqueCPtr<char> buf;
    size_t len = 0;
    while (getline(buf, &len, fp.get()) != -1) {
        std::string strBuf(buf.get());

        auto [start, end] = stringutils::trimInplace(strBuf);
        if (start == end) {
            continue;
        }
        std::string_view text(strBuf.data() + start, end - start);
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
            (text[word] != '\"' || word + 1 == text.size())) {
            continue;
        }

        std::string key(text.begin(), text.begin() + pos);
        std::string wordString;

        bool escapeQuote;
        if (text.back() == '\"' && text[word] == '\"') {
            wordString = text.substr(word + 1, text.size() - word - 2);
            escapeQuote = true;
        } else {
            wordString = text.substr(word);
            escapeQuote = false;
        }
        stringutils::unescape(wordString, escapeQuote);

        map_.emplace(std::move(key), std::move(wordString));
    }
}

SpellQuickPhraseProvider::SpellQuickPhraseProvider(QuickPhrase *quickPhrase)
    : parent_(quickPhrase), instance_(parent_->instance()) {}

bool SpellQuickPhraseProvider::populate(
    InputContext *ic, const std::string &userInput,
    const QuickPhraseAddCandidateCallback &addCandidate) {
    if (!*parent_->config().enableSpell) {
        return true;
    }
    auto spell = this->spell();
    if (!spell) {
        return true;
    }
    std::string lang = *parent_->config().fallbackSpellLanguage;
    if (auto entry = instance_->inputMethodEntry(ic)) {
        if (spell->call<ISpell::checkDict>(entry->languageCode())) {
            lang = entry->languageCode();
        } else if (!spell->call<ISpell::checkDict>(lang)) {
            return true;
        }
    }
    const auto result = spell->call<ISpell::hint>(
        lang, userInput, instance_->globalConfig().defaultPageSize());
    for (const auto &word : result) {
        addCandidate(word, word, QuickPhraseAction::Commit);
    }
    return true;
}

bool CallbackQuickPhraseProvider::populate(
    InputContext *ic, const std::string &userInput,
    const QuickPhraseAddCandidateCallback &addCandidate) {
    for (const auto &callback : callback_.view()) {
        if (!callback(ic, userInput, addCandidate)) {
            return false;
        }
    }
    return true;
}

} // namespace fcitx
