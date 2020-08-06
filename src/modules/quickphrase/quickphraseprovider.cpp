/*
 * SPDX-FileCopyrightText: 2020-2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "quickphraseprovider.h"
#include <fcntl.h>
#include <fcitx-utils/utf8.h>
#include "fcitx-utils/standardpath.h"
#include "fcitx-utils/stringutils.h"

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
        stringutils::unescape(wordString, escapeQuote);

        map_.emplace(std::move(key), std::move(wordString));
    }
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
