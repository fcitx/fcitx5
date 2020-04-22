//
// Copyright (C) 2020~2020 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2 of the
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
#include "quickphraseprovider.h"
#include <fcntl.h>
#include <fcitx-utils/utf8.h>
#include "fcitx-utils/standardpath.h"
#include "fcitx-utils/stringutils.h"

namespace fcitx {

bool BuiltInQuickPhraseProvider::populate(
    InputContext *, const std::string &userInput,
    QuickPhraseAddCandidateCallback addCandidate) {
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
}

typedef std::unique_ptr<FILE, decltype(&fclose)> ScopedFILE;
void BuiltInQuickPhraseProvider::load(StandardPathFile &file) {
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
        stringutils::unescape(wordString, escapeQuote);

        map_.emplace(std::move(key), std::move(wordString));
    }

    free(buf);
}

bool CallbackQuickPhraseProvider::populate(
    InputContext *ic, const std::string &userInput,
    QuickPhraseAddCandidateCallback addCandidate) {
    for (auto callback : callback_.view()) {
        if (!callback(ic, userInput, addCandidate)) {
            return false;
        }
    }
    return true;
}

} // namespace fcitx
