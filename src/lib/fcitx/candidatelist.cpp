/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "candidatelist.h"
#include <functional>
#include <stdexcept>
#include <unordered_set>
#include <fcitx-utils/macros.h>
#include <fcitx-utils/utf8.h>

namespace fcitx {

namespace {

constexpr size_t regularLabelSize = 10;

template <typename Container, typename Transformer>
void fillLabels(std::vector<Text> &labels, const Container &container,
                const Transformer &trans) {
    labels.clear();
    labels.reserve(std::max(std::size(container), regularLabelSize));
    for (const auto &item : container) {
        labels.emplace_back(trans(item));
    }
    while (labels.size() < regularLabelSize) {
        labels.emplace_back();
    }
}

template <typename CandidateListType, typename InterfaceType>
class CandidateListInterfaceAdapter : public QPtrHolder<CandidateListType>,
                                      public InterfaceType {
public:
    CandidateListInterfaceAdapter(CandidateListType *q)
        : QPtrHolder<CandidateListType>(q) {}
};

#define FCITX_COMMA_IF_2 ,
#define FCITX_COMMA_IF_1
#define FCITX_COMMA_IF(X) FCITX_EXPAND(FCITX_CONCATENATE(FCITX_COMMA_IF_, X))

#define FCITX_FORWARD_METHOD_ARG(N, ARG) ARG arg##N FCITX_COMMA_IF(N)

#define FCITX_FORWARD_METHOD_ARG_NAME(N, ARG) arg##N FCITX_COMMA_IF(N)

#define FCITX_FORWARD_METHOD(RETURN_TYPE, METHOD_NAME, ARGS, ...)              \
    RETURN_TYPE METHOD_NAME(FCITX_FOR_EACH_IDX(FCITX_FORWARD_METHOD_ARG,       \
                                               FCITX_REMOVE_PARENS(ARGS)))     \
        __VA_ARGS__ override {                                                 \
        FCITX_Q();                                                             \
        return q->METHOD_NAME(FCITX_FOR_EACH_IDX(                              \
            FCITX_FORWARD_METHOD_ARG_NAME, FCITX_REMOVE_PARENS(ARGS)));        \
    }

class BulkCursorAdaptorForCommonCandidateList
    : public CandidateListInterfaceAdapter<CommonCandidateList,
                                           BulkCursorCandidateList> {
public:
    using CandidateListInterfaceAdapter::CandidateListInterfaceAdapter;

    FCITX_FORWARD_METHOD(void, setGlobalCursorIndex, (int));
    FCITX_FORWARD_METHOD(int, globalCursorIndex, (), const);
};

class CursorModifiableAdaptorForCommonCandidateList
    : public CandidateListInterfaceAdapter<CommonCandidateList,
                                           CursorModifiableCandidateList> {
public:
    using CandidateListInterfaceAdapter::CandidateListInterfaceAdapter;

    FCITX_FORWARD_METHOD(void, setCursorIndex, (int));
};

} // namespace

ActionableCandidateList::~ActionableCandidateList() = default;

class CandidateListPrivate {
public:
    BulkCandidateList *bulk_ = nullptr;
    ModifiableCandidateList *modifiable_ = nullptr;
    PageableCandidateList *pageable_ = nullptr;
    CursorMovableCandidateList *cursorMovable_ = nullptr;
    BulkCursorCandidateList *bulkCursor_ = nullptr;
    CursorModifiableCandidateList *cursorModifiable_ = nullptr;
    ActionableCandidateList *actionable_ = nullptr;
};

CandidateList::CandidateList()
    : d_ptr(std::make_unique<CandidateListPrivate>()) {}

CandidateList::~CandidateList() {}

bool CandidateList::empty() const { return size() == 0; }

BulkCandidateList *CandidateList::toBulk() const {
    FCITX_D();
    return d->bulk_;
}

ModifiableCandidateList *CandidateList::toModifiable() const {
    FCITX_D();
    return d->modifiable_;
}

PageableCandidateList *CandidateList::toPageable() const {
    FCITX_D();
    return d->pageable_;
}

CursorMovableCandidateList *CandidateList::toCursorMovable() const {
    FCITX_D();
    return d->cursorMovable_;
}

CursorModifiableCandidateList *CandidateList::toCursorModifiable() const {
    FCITX_D();
    return d->cursorModifiable_;
}

BulkCursorCandidateList *CandidateList::toBulkCursor() const {
    FCITX_D();
    return d->bulkCursor_;
}

ActionableCandidateList *CandidateList::toActionable() const {
    FCITX_D();
    return d->actionable_;
}

void CandidateList::setBulk(BulkCandidateList *list) {
    FCITX_D();
    d->bulk_ = list;
}

void CandidateList::setModifiable(ModifiableCandidateList *list) {
    FCITX_D();
    d->modifiable_ = list;
}

void CandidateList::setPageable(PageableCandidateList *list) {
    FCITX_D();
    d->pageable_ = list;
}

void CandidateList::setCursorMovable(CursorMovableCandidateList *list) {
    FCITX_D();
    d->cursorMovable_ = list;
}

void CandidateList::setCursorModifiable(CursorModifiableCandidateList *list) {
    FCITX_D();
    d->cursorModifiable_ = list;
}

void CandidateList::setBulkCursor(BulkCursorCandidateList *list) {
    FCITX_D();
    d->bulkCursor_ = list;
}

void CandidateList::setActionable(ActionableCandidateList *list) {
    FCITX_D();
    d->actionable_ = list;
}

class CandidateWordPrivate {
public:
    CandidateWordPrivate(Text &&text) : text_(std::move(text)) {}
    Text text_;
    bool isPlaceHolder_ = false;
    Text customLabel_;
    bool hasCustomLabel_ = false;
    Text comment_;
};

CandidateWord::CandidateWord(Text text)
    : d_ptr(std::make_unique<CandidateWordPrivate>(std::move(text))) {}

CandidateWord::~CandidateWord() {}

const Text &CandidateWord::text() const {
    FCITX_D();
    return d->text_;
}

void CandidateWord::setText(Text text) {
    FCITX_D();
    d->text_ = std::move(text);
}

const Text &CandidateWord::comment() const {
    FCITX_D();
    return d->comment_;
}

void CandidateWord::setComment(Text comment) {
    FCITX_D();
    d->comment_ = std::move(comment);
}

Text CandidateWord::textWithComment(std::string separator) const {
    FCITX_D();
    auto text = d->text_;
    if (!d->comment_.empty()) {
        text.append(std::move(separator));
        text.append(d->comment_);
    }
    return text;
}

bool CandidateWord::isPlaceHolder() const {
    FCITX_D();
    return d->isPlaceHolder_;
}

bool CandidateWord::hasCustomLabel() const {
    FCITX_D();
    return d->hasCustomLabel_;
}

const Text &CandidateWord::customLabel() const {
    FCITX_D();
    return d->customLabel_;
}

void CandidateWord::setPlaceHolder(bool placeHolder) {
    FCITX_D();
    d->isPlaceHolder_ = placeHolder;
}

void CandidateWord::resetCustomLabel() {
    FCITX_D();
    d->customLabel_ = Text();
    d->hasCustomLabel_ = false;
}

void CandidateWord::setCustomLabel(Text text) {
    FCITX_D();
    d->customLabel_ = std::move(text);
    d->hasCustomLabel_ = true;
}

class DisplayOnlyCandidateListPrivate {
public:
    Text emptyText_;
    int cursorIndex_ = -1;
    CandidateLayoutHint layoutHint_ = CandidateLayoutHint::Vertical;
    std::vector<std::shared_ptr<CandidateWord>> candidateWords_;

    void checkIndex(int idx) const {
        if (idx < 0 || static_cast<size_t>(idx) >= candidateWords_.size()) {
            throw std::invalid_argument(
                "DisplayOnlyCandidateList: invalid index");
        }
    }
};

DisplayOnlyCandidateList::DisplayOnlyCandidateList()
    : d_ptr(std::make_unique<DisplayOnlyCandidateListPrivate>()) {}

DisplayOnlyCandidateList::~DisplayOnlyCandidateList() = default;

void DisplayOnlyCandidateList::setContent(
    const std::vector<std::string> &content) {
    std::vector<Text> text_content;
    for (const auto &str : content) {
        text_content.emplace_back();
        text_content.back().append(str);
    }
    setContent(std::move(text_content));
}

void DisplayOnlyCandidateList::setContent(std::vector<Text> content) {
    FCITX_D();
    for (auto &text : content) {
        d->candidateWords_.emplace_back(
            std::make_shared<DisplayOnlyCandidateWord>(std::move(text)));
    }
}

void DisplayOnlyCandidateList::setLayoutHint(CandidateLayoutHint hint) {
    FCITX_D();
    d->layoutHint_ = hint;
}

void DisplayOnlyCandidateList::setCursorIndex(int index) {
    FCITX_D();
    if (index < 0) {
        d->cursorIndex_ = -1;
    } else {
        d->checkIndex(index);
        d->cursorIndex_ = index;
    }
}

const Text &DisplayOnlyCandidateList::label(int idx) const {
    FCITX_D();
    d->checkIndex(idx);
    return d->emptyText_;
}

const CandidateWord &DisplayOnlyCandidateList::candidate(int idx) const {
    FCITX_D();
    d->checkIndex(idx);
    return *d->candidateWords_[idx];
}

int DisplayOnlyCandidateList::cursorIndex() const {
    FCITX_D();
    return d->cursorIndex_;
}

int DisplayOnlyCandidateList::size() const {
    FCITX_D();
    return d->candidateWords_.size();
}

CandidateLayoutHint DisplayOnlyCandidateList::layoutHint() const {
    FCITX_D();
    return d->layoutHint_;
}

class CommonCandidateListPrivate {
public:
    CommonCandidateListPrivate(CommonCandidateList *q)
        : bulkCursor_(q), cursorModifiable_(q) {}

    BulkCursorAdaptorForCommonCandidateList bulkCursor_;
    CursorModifiableAdaptorForCommonCandidateList cursorModifiable_;
    bool usedNextBefore_ = false;
    int cursorIndex_ = -1;
    int currentPage_ = 0;
    int pageSize_ = 5;
    std::vector<Text> labels_;
    // use shared_ptr for type erasure
    std::vector<std::unique_ptr<CandidateWord>> candidateWord_;
    CandidateLayoutHint layoutHint_ = CandidateLayoutHint::NotSet;
    bool cursorIncludeUnselected_ = false;
    bool cursorKeepInSamePage_ = false;
    CursorPositionAfterPaging cursorPositionAfterPaging_ =
        CursorPositionAfterPaging::DonotChange;
    std::unique_ptr<ActionableCandidateList> actionable_;

    int size() const {
        auto start = currentPage_ * pageSize_;
        auto remain = static_cast<int>(candidateWord_.size()) - start;
        if (remain > pageSize_) {
            return pageSize_;
        }
        return remain;
    }

    int toGlobalIndex(int idx) const { return idx + currentPage_ * pageSize_; }

    void checkIndex(int idx) const {
        if (idx < 0 || idx >= size()) {
            throw std::invalid_argument("CommonCandidateList: invalid index");
        }
    }

    void checkGlobalIndex(int idx) const {
        if (idx < 0 || static_cast<size_t>(idx) >= candidateWord_.size()) {
            throw std::invalid_argument(
                "CommonCandidateList: invalid global index");
        }
    }

    void fixCursorAfterPaging(int oldIndex) {
        if (oldIndex < 0) {
            return;
        }

        switch (cursorPositionAfterPaging_) {
        case CursorPositionAfterPaging::DonotChange:
            break;
        case CursorPositionAfterPaging::ResetToFirst:
            cursorIndex_ = currentPage_ * pageSize_;
            break;
        case CursorPositionAfterPaging::SameAsLast: {
            auto currentPageSize = size();
            if (oldIndex >= currentPageSize) {
                cursorIndex_ = currentPage_ * pageSize_ + size() - 1;
            } else {
                cursorIndex_ = currentPage_ * pageSize_ + oldIndex;
            }
            break;
        }
        }
    }
};

CommonCandidateList::CommonCandidateList()
    : d_ptr(std::make_unique<CommonCandidateListPrivate>(this)) {
    FCITX_D();
    setPageable(this);
    setModifiable(this);
    setBulk(this);
    setCursorMovable(this);
    setBulkCursor(&d->bulkCursor_);
    setCursorModifiable(&d->cursorModifiable_);

    setLabels();
}

CommonCandidateList::~CommonCandidateList() {}

std::string keyToLabel(const Key &key) {
    std::string result;
    if (key.sym() == FcitxKey_None) {
        return result;
    }

#define _APPEND_MODIFIER_STRING(STR, VALUE)                                    \
    if (key.states() & KeyState::VALUE) {                                      \
        result += STR;                                                         \
    }
    _APPEND_MODIFIER_STRING("C-", Ctrl)
    _APPEND_MODIFIER_STRING("A-", Alt)
    _APPEND_MODIFIER_STRING("S-", Shift)
    _APPEND_MODIFIER_STRING("M-", Super)

#undef _APPEND_MODIFIER_STRING

    auto chr = Key::keySymToUnicode(key.sym());
    if (chr) {
        result += utf8::UCS4ToUTF8(chr);
    } else {
        result = Key::keySymToString(key.sym(), KeyStringFormat::Localized);
    }
    if (!isApple()) {
        // add a dot as separator
        result += ". ";
    }

    return result;
}

void CommonCandidateList::setLabels(const std::vector<std::string> &labels) {
    FCITX_D();
    fillLabels(
        d->labels_, labels,
        [](const std::string &str) -> const std::string & { return str; });
}

void CommonCandidateList::setSelectionKey(const KeyList &keyList) {
    FCITX_D();
    fillLabels(d->labels_, keyList,
               [](const Key &str) -> std::string { return keyToLabel(str); });
}

void CommonCandidateList::clear() {
    FCITX_D();
    d->candidateWord_.clear();
}

int CommonCandidateList::currentPage() const {
    FCITX_D();
    return d->currentPage_;
}

int CommonCandidateList::cursorIndex() const {
    FCITX_D();
    int cursorPage = d->cursorIndex_ / d->pageSize_;
    if (d->cursorIndex_ >= 0 && cursorPage == d->currentPage_) {
        return d->cursorIndex_ % d->pageSize_;
    }
    return -1;
}

bool CommonCandidateList::hasNext() const {
    // page size = 5
    // total size = 5 -> 1 page
    // total size = 6 -> 2 page
    FCITX_D();
    return d->currentPage_ + 1 < totalPages();
}

bool CommonCandidateList::hasPrev() const {
    FCITX_D();
    return d->currentPage_ > 0;
}

void CommonCandidateList::prev() {
    FCITX_D();
    if (!hasPrev()) {
        return;
    }
    setPage(d->currentPage_ - 1);
}

void CommonCandidateList::next() {
    FCITX_D();
    if (!hasNext()) {
        return;
    }
    setPage(d->currentPage_ + 1);
    d->usedNextBefore_ = true;
}

bool CommonCandidateList::usedNextBefore() const {
    FCITX_D();
    return d->usedNextBefore_;
}

void CommonCandidateList::setPageSize(int size) {
    FCITX_D();
    if (size < 1) {
        throw std::invalid_argument("CommonCandidateList: invalid page size");
    }
    d->pageSize_ = size;
    d->currentPage_ = 0;
}

int CommonCandidateList::pageSize() const {
    FCITX_D();
    return d->pageSize_;
}

int CommonCandidateList::size() const {
    FCITX_D();
    return d->size();
}

int CommonCandidateList::totalSize() const {
    FCITX_D();
    return d->candidateWord_.size();
}

const CandidateWord &CommonCandidateList::candidate(int idx) const {
    FCITX_D();
    d->checkIndex(idx);
    auto globalIndex = d->toGlobalIndex(idx);
    return *d->candidateWord_[globalIndex];
}

const Text &CommonCandidateList::label(int idx) const {
    FCITX_D();
    d->checkIndex(idx);
    if (idx < 0 || idx >= size() ||
        static_cast<size_t>(idx) >= d->labels_.size()) {
        throw std::invalid_argument("CommonCandidateList: invalid label idx");
    }

    return d->labels_[idx];
}

void CommonCandidateList::insert(int idx, std::unique_ptr<CandidateWord> word) {
    FCITX_D();
    // it's ok to insert at tail
    if (idx != static_cast<int>(d->candidateWord_.size())) {
        d->checkGlobalIndex(idx);
    }
    d->candidateWord_.insert(d->candidateWord_.begin() + idx, std::move(word));
}

void CommonCandidateList::remove(int idx) {
    FCITX_D();
    d->checkGlobalIndex(idx);
    d->candidateWord_.erase(d->candidateWord_.begin() + idx);
    fixAfterUpdate();
}

int CommonCandidateList::totalPages() const {
    FCITX_D();
    return (totalSize() + d->pageSize_ - 1) / d->pageSize_;
}

void CommonCandidateList::setLayoutHint(CandidateLayoutHint hint) {
    FCITX_D();
    d->layoutHint_ = hint;
}

void CommonCandidateList::setGlobalCursorIndex(int index) {
    FCITX_D();
    if (index < 0) {
        d->cursorIndex_ = -1;
    } else {
        d->checkGlobalIndex(index);
        d->cursorIndex_ = index;
    }
}

void CommonCandidateList::setCursorIndex(int index) {
    FCITX_D();
    d->checkIndex(index);
    auto globalIndex = d->toGlobalIndex(index);
    setGlobalCursorIndex(globalIndex);
}

int CommonCandidateList::globalCursorIndex() const {
    FCITX_D();
    return d->cursorIndex_;
}

CandidateLayoutHint CommonCandidateList::layoutHint() const {
    FCITX_D();
    return d->layoutHint_;
}

const CandidateWord &CommonCandidateList::candidateFromAll(int idx) const {
    FCITX_D();
    d->checkGlobalIndex(idx);
    return *d->candidateWord_[idx];
}

void CommonCandidateList::move(int from, int to) {
    FCITX_D();
    d->checkGlobalIndex(from);
    d->checkGlobalIndex(to);
    if (from < to) {
        // 1 2 3 4 5
        // from 2 to 5
        // 1 3 4 5 2
        std::rotate(d->candidateWord_.begin() + from,
                    d->candidateWord_.begin() + from + 1,
                    d->candidateWord_.begin() + to + 1);
    } else if (from > to) {
        // 1 2 3 4 5
        // from 5 to 2
        // 1 5 2 3 4
        std::rotate(d->candidateWord_.begin() + to,
                    d->candidateWord_.begin() + from,
                    d->candidateWord_.begin() + from + 1);
    }
}

void CommonCandidateList::moveCursor(bool prev) {
    FCITX_D();
    if (totalSize() <= 0 || size() <= 0) {
        return;
    }

    int startCursor = d->cursorIndex_;
    int startPage = d->currentPage_;
    std::unordered_set<int> deadloopDetect;
    do {
        deadloopDetect.insert(d->cursorIndex_);
        auto pageBegin = d->pageSize_ * d->currentPage_;
        if (cursorIndex() < 0) {
            setGlobalCursorIndex(pageBegin + (prev ? size() - 1 : 0));
        } else {
            int rotationBase;
            int rotationSize;
            if (d->cursorKeepInSamePage_) {
                rotationBase = pageBegin;
                rotationSize = size();
            } else {
                rotationBase = 0;
                rotationSize = totalSize();
            }
            auto newGlobalIndex = d->cursorIndex_ + (prev ? -1 : 1);
            if (newGlobalIndex < rotationBase ||
                newGlobalIndex >= rotationBase + rotationSize) {
                if (d->cursorIncludeUnselected_) {
                    d->cursorIndex_ = -1;
                } else {
                    d->cursorIndex_ =
                        prev ? (rotationBase + rotationSize - 1) : rotationBase;
                }
            } else {
                d->cursorIndex_ = newGlobalIndex;
            }
            if (!d->cursorKeepInSamePage_ && d->cursorIndex_ >= 0) {
                setPage(d->cursorIndex_ / d->pageSize_);
            }
        }
    } while (!deadloopDetect.count(d->cursorIndex_) && d->cursorIndex_ >= 0 &&
             candidateFromAll(d->cursorIndex_).isPlaceHolder());
    if (deadloopDetect.count(d->cursorIndex_)) {
        d->cursorIndex_ = startCursor;
        d->currentPage_ = startPage;
    }
}

void CommonCandidateList::prevCandidate() { moveCursor(true); }

void CommonCandidateList::nextCandidate() { moveCursor(false); }

void CommonCandidateList::setCursorIncludeUnselected(bool v) {
    FCITX_D();
    d->cursorIncludeUnselected_ = v;
}

void CommonCandidateList::setCursorKeepInSamePage(bool v) {
    FCITX_D();
    d->cursorKeepInSamePage_ = v;
}

void CommonCandidateList::setCursorPositionAfterPaging(
    CursorPositionAfterPaging afterPaging) {
    FCITX_D();
    d->cursorPositionAfterPaging_ = afterPaging;
}

void CommonCandidateList::setPage(int page) {
    FCITX_D();
    auto totalPage = totalPages();
    if (page >= 0 && page < totalPage) {
        if (d->currentPage_ != page) {
            auto oldIndex = cursorIndex();
            d->currentPage_ = page;
            d->fixCursorAfterPaging(oldIndex);
        }
    } else {
        throw std::invalid_argument("invalid page");
    }
}

void CommonCandidateList::replace(int idx,
                                  std::unique_ptr<CandidateWord> word) {
    FCITX_D();
    d->candidateWord_[idx] = std::move(word);
}

void CommonCandidateList::fixAfterUpdate() {
    FCITX_D();
    if (d->currentPage_ >= totalPages() && d->currentPage_ > 0) {
        d->currentPage_ = totalPages() - 1;
    }
    if (d->cursorIndex_ >= 0) {
        if (d->cursorIndex_ >= totalSize()) {
            d->cursorIndex_ = 0;
        }
    }
}

void CommonCandidateList::setActionableImpl(
    std::unique_ptr<ActionableCandidateList> actionable) {
    FCITX_D();
    d->actionable_ = std::move(actionable);
    setActionable(d->actionable_.get());
}

} // namespace fcitx
