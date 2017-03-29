/*
 * Copyright (C) 2017~2017 by CSSlayer
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

#include "candidatelist.h"

namespace fcitx {

class CandidateListPrivate {
public:
    CandidateListPrivate(CandidateList *q) : CandidateListUpdateAdaptor(q) {}

    BulkCandidateList *bulk = nullptr;
    ModifiableCandidateList *modifiable = nullptr;
    PageableCandidateList *pageable = nullptr;

    FCITX_DEFINE_SIGNAL_PRIVATE(CandidateList, Update);
};

CandidateList::CandidateList()
    : d_ptr(std::make_unique<CandidateListPrivate>(this)) {}

CandidateList::~CandidateList() {}

BulkCandidateList *CandidateList::toBulk() const {
    FCITX_D();
    return d->bulk;
}

ModifiableCandidateList *CandidateList::toModifiable() const {
    FCITX_D();
    return d->modifiable;
}

PageableCandidateList *CandidateList::toPageable() const {
    FCITX_D();
    return d->pageable;
}

void CandidateList::setBulk(BulkCandidateList *list) {
    FCITX_D();
    d->bulk = list;
}

void CandidateList::setModifiable(ModifiableCandidateList *list) {
    FCITX_D();
    d->modifiable = list;
}

void CandidateList::setPageable(PageableCandidateList *list) {
    FCITX_D();
    d->pageable = list;
}

class CandidateWordPrivate {
public:
    CandidateWordPrivate(Text &&text) : text_(std::move(text)) {}
    Text text_;
};

CandidateWord::CandidateWord(Text text)
    : d_ptr(std::make_unique<CandidateWordPrivate>(std::move(text))) {}

CandidateWord::~CandidateWord() {}

const Text &CandidateWord::text() const {
    FCITX_D();
    return d->text_;
}

class CommonCandidateListPrivate {
public:
    int cursorIndex = -1;
    int currentPage = 0;
    int pageSize = 5;
    std::vector<Text> labels;
    // use shared_ptr for type erasure
    std::vector<std::shared_ptr<CandidateWord>> candidateWord;
    CandidateLayoutHint layoutHint;

    int size() const {
        auto start = currentPage * pageSize;
        auto remain = static_cast<int>(candidateWord.size()) - start;
        if (remain > pageSize) {
            return pageSize;
        }
        return remain;
    }

    int toGlobalIndex(int idx) const { return idx + currentPage * pageSize; }

    void checkIndex(int idx) const {
        if (idx < 0 && idx >= size()) {
            throw std::invalid_argument("invalid index");
        }
    }

    void checkGlobalIndex(int idx) const {
        if (idx < 0 || static_cast<size_t>(idx) > candidateWord.size()) {
            throw std::invalid_argument("invalid index");
        }
    }
};

CommonCandidateList::CommonCandidateList()
    : d_ptr(std::make_unique<CommonCandidateListPrivate>()) {
    setPageable(this);
    setModifiable(this);
    setBulk(this);
}

CommonCandidateList::~CommonCandidateList() {}

std::string keyToLabel(const Key &key) {
    std::string result;

#define _APPEND_MODIFIER_STRING(STR, VALUE)                                    \
    if (key.states() & KeyState::VALUE) {                                      \
        result += STR;                                                         \
    }
    _APPEND_MODIFIER_STRING("C-", Ctrl)
    _APPEND_MODIFIER_STRING("A-", Alt)
    _APPEND_MODIFIER_STRING("S-", Shift)
    _APPEND_MODIFIER_STRING("M-", Super)

#undef _APPEND_MODIFIER_STRING

    result += Key::keySymToUnicode(key.sym());

    return result;
}

void CommonCandidateList::setSelectionKey(const KeyList &keyList) {
    FCITX_D();
    d->labels.clear();
    d->labels.reserve(keyList.size());
    for (auto &key : keyList) {
        d->labels.emplace_back(keyToLabel(key));
    }
}

void CommonCandidateList::clear() {
    FCITX_D();
    d->candidateWord.clear();
}

int CommonCandidateList::currentPage() const {
    FCITX_D();
    return d->currentPage;
}

int CommonCandidateList::cursorIndex() const {
    FCITX_D();
    return d->cursorIndex;
}

bool CommonCandidateList::hasNext() const {
    // page size = 5
    // total size = 5 -> 1 page
    // total size = 6 -> 2 page
    FCITX_D();
    return d->currentPage + 1 < totalPages();
}

bool CommonCandidateList::hasPrev() const {
    FCITX_D();
    return d->currentPage > 0;
}

void CommonCandidateList::prev() {
    FCITX_D();
    if (!hasPrev()) {
        return;
    }
    d->currentPage--;
}

void CommonCandidateList::next() {
    FCITX_D();
    if (!hasNext()) {
        return;
    }
    d->currentPage++;
}

void CommonCandidateList::setPageSize(int size) {
    FCITX_D();
    if (size < 1) {
        throw std::invalid_argument("invalid page size");
    }
    d->pageSize = size;
    d->currentPage = 0;
}

int CommonCandidateList::size() const {
    FCITX_D();
    return d->size();
}

int CommonCandidateList::totalSize() const {
    FCITX_D();
    return d->candidateWord.size();
}

const CandidateWord &CommonCandidateList::candidate(int idx) const {
    FCITX_D();
    d->checkIndex(idx);
    auto globalIndex = d->toGlobalIndex(idx);
    return *d->candidateWord[globalIndex];
}

const Text &CommonCandidateList::label(int idx) const {
    FCITX_D();
    d->checkIndex(idx);
    if (idx < 0 || idx >= size() ||
        static_cast<size_t>(idx) >= d->labels.size()) {
        throw std::invalid_argument("invalid idx");
    }

    return d->labels[idx];
}

void CommonCandidateList::insert(int idx, CandidateWord *word) {
    FCITX_D();
    d->checkGlobalIndex(idx);
    d->candidateWord.insert(d->candidateWord.begin() + idx,
                            std::shared_ptr<CandidateWord>(word));
}

void CommonCandidateList::remove(int idx) {
    FCITX_D();
    d->checkGlobalIndex(idx);
    d->candidateWord.erase(d->candidateWord.begin() + idx);
    fixAfterUpdate();
}

int CommonCandidateList::totalPages() const {
    FCITX_D();
    return (totalSize() + d->pageSize - 1) / d->pageSize;
}

void CommonCandidateList::setLayoutHint(CandidateLayoutHint hint) {
    FCITX_D();
    d->layoutHint = hint;
}

void CommonCandidateList::setCursorIndex(int index) {
    FCITX_D();
    if (index < 0) {
        d->cursorIndex = -1;
    } else {
        d->checkIndex(index);
        d->cursorIndex = index;
    }
}

CandidateLayoutHint CommonCandidateList::layoutHint() const {
    FCITX_D();
    return d->layoutHint;
}

const CandidateWord &CommonCandidateList::candidateFromAll(int idx) const {
    FCITX_D();
    d->checkGlobalIndex(idx);
    return *d->candidateWord[idx];
}

void CommonCandidateList::move(int from, int to) {
    FCITX_D();
    d->checkGlobalIndex(from);
    d->checkGlobalIndex(to);
    if (from < to) {
        // 1 2 3 4 5
        // from 2 to 5
        // 1 3 4 5 2
        std::rotate(d->candidateWord.begin() + from,
                    d->candidateWord.begin() + from + 1,
                    d->candidateWord.begin() + to + 1);
    } else if (from > to) {
        // 1 2 3 4 5
        // from 5 to 2
        // 1 5 2 3 4
        std::rotate(d->candidateWord.begin() + to,
                    d->candidateWord.begin() + from,
                    d->candidateWord.begin() + from + 1);
    }
}

void CommonCandidateList::setPage(int page) {
    FCITX_D();
    auto totalPage = totalPages();
    if (page >= 0 && page < totalPage) {
        d->currentPage = page;
    } else {
        throw std::invalid_argument("invalid page");
    }
}

void CommonCandidateList::replace(int idx, CandidateWord *word) {
    FCITX_D();
    d->candidateWord[idx].reset(word);
}

void CommonCandidateList::fixAfterUpdate() {
    FCITX_D();
    if (d->currentPage >= totalPages() && d->currentPage > 0) {
        d->currentPage = totalPages() - 1;
    }
    if (d->cursorIndex >= 0) {
        if (d->cursorIndex >= size()) {
            d->cursorIndex = 0;
        }
    }
}
}
