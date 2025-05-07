/*
 * SPDX-FileCopyrightText: 2020-2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "quickphraseprovider.h"
#include <fcntl.h>
#include <cstddef>
#include <istream>
#include <string>
#include <string_view>
#include <utility>
#include "fcitx-utils/fdstreambuf.h"
#include "fcitx-utils/fs.h"
#include "fcitx-utils/macros.h"
#include "fcitx-utils/misc.h"
#include "fcitx-utils/standardpaths.h"
#include "fcitx-utils/stringutils.h"
#include "fcitx-utils/utf8.h"
#include "fcitx/inputcontext.h"
#include "fcitx/inputmethodentry.h"
#include "quickphrase.h"
#include "quickphrase_public.h"
#include "spell_public.h"

namespace fcitx {

bool BuiltInQuickPhraseProvider::populate(
    InputContext *, const std::string &userInput,
    const QuickPhraseAddCandidateCallbackV2 &addCandidate) {
    auto start = map_.lower_bound(userInput);
    auto end = map_.end();

    for (; start != end; start++) {
        if (!stringutils::startsWith(start->first, userInput)) {
            break;
        }
        addCandidate(start->second, start->second, start->first,
                     QuickPhraseAction::Commit);
    }
    return true;
}
void BuiltInQuickPhraseProvider::reloadConfig() {

    map_.clear();
    if (auto file = StandardPaths::global().open(StandardPathsType::PkgData,
                                                 "data/QuickPhrase.mb");
        file.isValid()) {
        load(file.fd());
    }

    auto files = StandardPaths::global().locate(StandardPathsType::PkgData,
                                                "data/quickphrase.d/",
                                                pathfilter::extension(".mb"));
    auto disableFiles = StandardPaths::global().locate(
        StandardPathsType::PkgData, "data/quickphrase.d/",
        pathfilter::extension(".disable"));
    for (const auto &p : files) {
        auto path = p.first;
        // std::filesystem::path can only do +=.
        path += ".disable";
        if (disableFiles.contains(path)) {
            continue;
        }
        UnixFD fd = UnixFD::own(open(p.second.c_str(), O_RDONLY));
        if (fd.isValid()) {
            load(fd.fd());
        }
    }
}

void BuiltInQuickPhraseProvider::load(int fd) {
    IFDStreamBuf buf(fd);
    std::istream in(&buf);
    std::string line;
    while (std::getline(in, line)) {
        std::string_view text = stringutils::trimView(line);
        if (text.empty()) {
            continue;
        }
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

        std::string key(text.begin(), text.begin() + pos);
        auto wordString = stringutils::unescapeForValue(text.substr(word));

        if (!wordString) {
            continue;
        }
        map_.emplace(std::move(key), std::move(*wordString));
    }
}

SpellQuickPhraseProvider::SpellQuickPhraseProvider(QuickPhrase *parent)
    : parent_(parent), instance_(parent_->instance()) {}

bool SpellQuickPhraseProvider::populate(
    InputContext *ic, const std::string &userInput,
    const QuickPhraseAddCandidateCallbackV2 &addCandidate) {
    if (!*parent_->config().enableSpell) {
        return true;
    }
    auto *spell = this->spell();
    if (!spell) {
        return true;
    }
    std::string lang = *parent_->config().fallbackSpellLanguage;
    if (const auto *entry = instance_->inputMethodEntry(ic)) {
        if (spell->call<ISpell::checkDict>(entry->languageCode())) {
            lang = entry->languageCode();
        } else if (!spell->call<ISpell::checkDict>(lang)) {
            return true;
        }
    }

    // Do not give spell hint if input contains url like character.
    if (userInput.find_first_of(".@/+%") != std::string::npos) {
        return true;
    }

    const auto result = spell->call<ISpell::hint>(
        lang, userInput, instance_->globalConfig().defaultPageSize());
    for (const auto &word : result) {
        addCandidate(word, word, "", QuickPhraseAction::Commit);
    }
    return true;
}

bool CallbackQuickPhraseProvider::populate(
    InputContext *ic, const std::string &userInput,
    const QuickPhraseAddCandidateCallbackV2 &addCandidate) {
    for (const auto &callback : callbackV2_.view()) {
        if (!callback(ic, userInput, addCandidate)) {
            return false;
        }
    }
    for (const auto &callback : callback_.view()) {
        if (!callback(ic, userInput,
                      [&addCandidate](const std::string &word,
                                      const std::string &aux,
                                      QuickPhraseAction action) {
                          return addCandidate(word, aux, "", action);
                      })) {
            return false;
        }
    }
    return true;
}

} // namespace fcitx
