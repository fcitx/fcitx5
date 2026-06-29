/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <span>
#include "fcitx-utils/key.h"
#include "fcitx-utils/log.h"
#include "fcitx/candidateaction.h"
#include "fcitx/candidatelist.h"
#include "fcitx/text.h"

namespace {

using namespace fcitx;
int selected = 0;

class TestCandidateWord : public CandidateWord {
public:
    TestCandidateWord(int number, Text comment = {})
        : CandidateWord(Text(std::to_string(number))), number_(number) {
        setComment(std::move(comment));
    }
    void select(InputContext * /*inputContext*/) const override {
        selected = number_;
    }

    using CandidateWord::setSpaceBetweenComment;

private:
    int number_;
};

class PlaceHolderCandidateWord : public CandidateWord {
public:
    PlaceHolderCandidateWord(int number)
        : CandidateWord(Text(std::to_string(number))), number_(number) {
        setPlaceHolder(true);
    }
    void select(InputContext * /*inputContext*/) const override {
        selected = number_;
    }

private:
    int number_;
};

void test_basic() {
    CommonCandidateList candidatelist;
    candidatelist.setSelectionKey(
        Key::keyListFromString("1 2 3 4 5 6 7 8 9 0"));
    candidatelist.setPageSize(3);
    for (int i = 0; i < 10; i++) {
        candidatelist.append<TestCandidateWord>(i);
    }

    FCITX_ASSERT(candidatelist.size() == 3);
    FCITX_ASSERT(candidatelist.label(0).toString() == "1. ");
    FCITX_ASSERT(candidatelist.candidate(0).text().toString() == "0");
    FCITX_ASSERT(!candidatelist.hasPrev());
    FCITX_ASSERT(candidatelist.hasNext());

    FCITX_ASSERT(candidatelist.totalPages() == 4);
    FCITX_ASSERT(candidatelist.currentPage() == 0);

    // BulkCandidateList
    for (int i = 0; i < 10; i++) {
        FCITX_ASSERT(candidatelist.candidateFromAll(i).text().toString() ==
                     std::to_string(i));
    }

    FCITX_ASSERT(candidatelist.totalSize() == 10);

    candidatelist.next();

    FCITX_ASSERT(candidatelist.size() == 3);
    FCITX_ASSERT(candidatelist.label(0).toString() == "1. ");
    FCITX_ASSERT(candidatelist.candidate(0).text().toString() == "3");
    FCITX_ASSERT(candidatelist.hasPrev());
    FCITX_ASSERT(candidatelist.hasNext());

    FCITX_ASSERT(candidatelist.totalPages() == 4);
    FCITX_ASSERT(candidatelist.currentPage() == 1);

    // BulkCandidateList
    for (int i = 0; i < 10; i++) {
        FCITX_ASSERT(candidatelist.candidateFromAll(i).text().toString() ==
                     std::to_string(i));
    }

    FCITX_ASSERT(candidatelist.totalSize() == 10);

    candidatelist.next();
    candidatelist.next();

    FCITX_ASSERT(candidatelist.size() == 1);
    FCITX_ASSERT(candidatelist.label(0).toString() == "1. ");
    FCITX_ASSERT(candidatelist.candidate(0).text().toString() == "9");
    FCITX_ASSERT(candidatelist.hasPrev());
    FCITX_ASSERT(!candidatelist.hasNext());

    FCITX_ASSERT(candidatelist.totalPages() == 4);
    FCITX_ASSERT(candidatelist.currentPage() == 3);

    // BulkCandidateList
    for (int i = 0; i < 10; i++) {
        FCITX_ASSERT(candidatelist.candidateFromAll(i).text().toString() ==
                     std::to_string(i));
    }

    FCITX_ASSERT(candidatelist.totalSize() == 10);

    candidatelist.remove(0);
    FCITX_ASSERT(candidatelist.size() == 3);
    FCITX_ASSERT(candidatelist.label(0).toString() == "1. ");
    FCITX_ASSERT(candidatelist.candidate(0).text().toString() == "7");
    FCITX_ASSERT(candidatelist.hasPrev());
    FCITX_ASSERT(!candidatelist.hasNext());

    FCITX_ASSERT(candidatelist.totalPages() == 3);
    FCITX_ASSERT(candidatelist.currentPage() == 2);

    // BulkCandidateList
    for (int i = 0; i < 9; i++) {
        FCITX_ASSERT(candidatelist.candidateFromAll(i).text().toString() ==
                     std::to_string(i + 1));
    }

    FCITX_ASSERT(candidatelist.totalSize() == 9);

    candidatelist.move(0, 4);
    FCITX_ASSERT(candidatelist.totalSize() == 9);

    // result
    int expect1[] = {2, 3, 4, 5, 1, 6, 7, 8, 9};
    for (int i = 0; i < 9; i++) {
        FCITX_ASSERT(candidatelist.candidateFromAll(i).text().toString() ==
                     std::to_string(expect1[i]));
    }
    FCITX_ASSERT(candidatelist.totalSize() == 9);

    candidatelist.move(8, 3);
    int expect2[] = {2, 3, 4, 9, 5, 1, 6, 7, 8};
    for (int i = 0; i < 9; i++) {
        FCITX_ASSERT(candidatelist.candidateFromAll(i).text().toString() ==
                     std::to_string(expect2[i]));
    }
    FCITX_ASSERT(candidatelist.totalSize() == 9);

    bool hitException = false;
    try {
        candidatelist.insert(10, std::make_unique<TestCandidateWord>(10));
    } catch (const std::invalid_argument &e) {
        hitException = true;
    }
    FCITX_ASSERT(hitException);

    candidatelist.insert(0, std::make_unique<TestCandidateWord>(10));
    candidatelist.insert(10, std::make_unique<TestCandidateWord>(11));
    candidatelist.insert(5, std::make_unique<TestCandidateWord>(12));
    int expect3[] = {10, 2, 3, 4, 9, 12, 5, 1, 6, 7, 8, 11};
    for (int i = 0; i < 12; i++) {
        FCITX_ASSERT(candidatelist.candidateFromAll(i).text().toString() ==
                     std::to_string(expect3[i]));
    }
    FCITX_ASSERT(candidatelist.totalSize() == 12);

    candidatelist.setCursorKeepInSamePage(true);
    candidatelist.setGlobalCursorIndex(0);
    candidatelist.setPage(0);
    candidatelist.prevCandidate();
    FCITX_ASSERT(candidatelist.cursorIndex() == candidatelist.pageSize() - 1)
        << candidatelist.cursorIndex();
    FCITX_ASSERT(candidatelist.currentPage() == 0)
        << candidatelist.currentPage();

    candidatelist.setCursorKeepInSamePage(false);
    candidatelist.nextCandidate();
    FCITX_ASSERT(candidatelist.cursorIndex() == 0)
        << candidatelist.cursorIndex();
    FCITX_ASSERT(candidatelist.currentPage() == 1)
        << candidatelist.currentPage();

    candidatelist.setCursorPositionAfterPaging(
        CursorPositionAfterPaging::DonotChange);
    candidatelist.setPage(2);
    FCITX_ASSERT(candidatelist.cursorIndex() == -1)
        << candidatelist.cursorIndex();
    FCITX_ASSERT(candidatelist.currentPage() == 2)
        << candidatelist.currentPage();

    candidatelist.setCursorPositionAfterPaging(
        CursorPositionAfterPaging::ResetToFirst);
    candidatelist.setPage(1);
    FCITX_ASSERT(candidatelist.cursorIndex() == 0)
        << candidatelist.cursorIndex();
    FCITX_ASSERT(candidatelist.currentPage() == 1)
        << candidatelist.currentPage();

    candidatelist.nextCandidate();
    FCITX_ASSERT(candidatelist.cursorIndex() == 1)
        << candidatelist.cursorIndex();

    candidatelist.setCursorPositionAfterPaging(
        CursorPositionAfterPaging::SameAsLast);
    candidatelist.setPage(0);
    FCITX_ASSERT(candidatelist.cursorIndex() == 1)
        << candidatelist.cursorIndex();
    FCITX_ASSERT(candidatelist.currentPage() == 0)
        << candidatelist.currentPage();

    TestCandidateWord candidate(1, Text("comment"));
    FCITX_ASSERT(candidate.spaceBetweenComment());
    candidate.setSpaceBetweenComment(false);
    FCITX_ASSERT(!candidate.spaceBetweenComment());
}

void test_faulty_placeholder() {
    CommonCandidateList candidatelist;
    candidatelist.setSelectionKey(
        Key::keyListFromString("1 2 3 4 5 6 7 8 9 0"));
    candidatelist.setPageSize(3);
    candidatelist.append<PlaceHolderCandidateWord>(3);
    FCITX_ASSERT(candidatelist.cursorIndex() == -1)
        << candidatelist.cursorIndex();

    candidatelist.prevCandidate();
    FCITX_ASSERT(candidatelist.cursorIndex() == -1)
        << candidatelist.cursorIndex();

    candidatelist.nextCandidate();
    FCITX_ASSERT(candidatelist.cursorIndex() == -1)
        << candidatelist.cursorIndex();
    candidatelist.append<TestCandidateWord>(3);

    candidatelist.prevCandidate();
    FCITX_ASSERT(candidatelist.cursorIndex() == 1)
        << candidatelist.cursorIndex();

    candidatelist.setGlobalCursorIndex(-1);
    FCITX_ASSERT(candidatelist.cursorIndex() == -1)
        << candidatelist.cursorIndex();

    candidatelist.nextCandidate();
    FCITX_ASSERT(candidatelist.cursorIndex() == 1)
        << candidatelist.cursorIndex();

    candidatelist.append<TestCandidateWord>(3);
    candidatelist.append<PlaceHolderCandidateWord>(3);
    candidatelist.append<PlaceHolderCandidateWord>(3);
    candidatelist.append<PlaceHolderCandidateWord>(3);
    // Two page, second page is empty.

    candidatelist.nextCandidate();
    FCITX_ASSERT(candidatelist.cursorIndex() == 2)
        << candidatelist.cursorIndex();

    candidatelist.setCursorIncludeUnselected(true);

    candidatelist.nextCandidate();
    FCITX_ASSERT(candidatelist.cursorIndex() == -1)
        << candidatelist.cursorIndex();

    candidatelist.setGlobalCursorIndex(4);
    candidatelist.setPage(1);

    candidatelist.nextCandidate();
    FCITX_ASSERT(candidatelist.cursorIndex() == -1)
        << candidatelist.cursorIndex();
    candidatelist.append<TestCandidateWord>(3);
    candidatelist.setPage(0);
    candidatelist.prevCandidate();
    FCITX_ASSERT(candidatelist.cursorIndex() == 2)
        << candidatelist.cursorIndex();
    candidatelist.prevCandidate();
    FCITX_ASSERT(candidatelist.cursorIndex() == 1)
        << candidatelist.cursorIndex();
    candidatelist.nextCandidate();
    FCITX_ASSERT(candidatelist.cursorIndex() == 2)
        << candidatelist.cursorIndex();
    candidatelist.nextCandidate();
    FCITX_ASSERT(candidatelist.cursorIndex() == 0)
        << candidatelist.cursorIndex();
    FCITX_ASSERT(candidatelist.currentPage() == 2)
        << candidatelist.currentPage();
    candidatelist.nextCandidate();
    FCITX_ASSERT(candidatelist.cursorIndex() == -1)
        << candidatelist.cursorIndex();
    FCITX_INFO() << candidatelist.currentPage();
    candidatelist.setPage(2);
    FCITX_INFO() << candidatelist.currentPage();
    candidatelist.nextCandidate();
    FCITX_ASSERT(candidatelist.cursorIndex() == 0)
        << candidatelist.cursorIndex();
}

void test_label() {
    CommonCandidateList candidatelist;
    candidatelist.setPageSize(10);
    candidatelist.setSelectionKey(
        Key::keyListFromString("1 2 3 4 5 6 7 8 9 0"));
    for (int i = 0; i < 10; i++) {
        candidatelist.append<TestCandidateWord>(i);
    }

    FCITX_ASSERT(candidatelist.label(0).toString() == "1. ")
        << candidatelist.label(0).toString();
    FCITX_ASSERT(candidatelist.label(5).toString() == "6. ");
    FCITX_ASSERT(candidatelist.label(9).toString() == "0. ");
    candidatelist.setSelectionKey(
        Key::keyListFromString("F1 F2 F3 F4 F5 F6 F7 F8 F9 F10"));
    FCITX_ASSERT(candidatelist.label(5).toString() == "F6. ");
    candidatelist.setSelectionKey(Key::keyListFromString(
        "a Control+a Control+Shift+A F4 F5 Page_Up F7 F8 F9 comma"));
    FCITX_ASSERT(candidatelist.label(0).toString() == "a. ");
    FCITX_ASSERT(candidatelist.label(1).toString() == "C-a. ");
    FCITX_ASSERT(candidatelist.label(2).toString() == "C-S-A. ");
    FCITX_ASSERT(candidatelist.label(5).toString() == "Page Up. ");
    FCITX_ASSERT(candidatelist.label(9).toString() == ",. ");
}

void test_comment() {
    TestCandidateWord candidate(1, Text("comment"));
    FCITX_ASSERT(candidate.text().toString() == "1");
    FCITX_ASSERT(candidate.comment().toString() == "comment");
    FCITX_ASSERT(candidate.textWithComment().toString() == "1 comment");
}

void test_cursor() {
    CommonCandidateList candidatelist;
    candidatelist.setPageSize(5);
    candidatelist.setSelectionKey(
        Key::keyListFromString("1 2 3 4 5 6 7 8 9 0"));
    for (int i = 0; i < 10; i++) {
        candidatelist.append<TestCandidateWord>(i);
    }
    candidatelist.setPage(1);
    candidatelist.toCursorModifiable()->setCursorIndex(3);
    FCITX_ASSERT(candidatelist.toBulkCursor()->globalCursorIndex(), 8);
    candidatelist.setCursorIndex(0);
    FCITX_ASSERT(candidatelist.toBulkCursor()->globalCursorIndex(), 5);
}

void test_candidateaction() {
    CandidateAction action;
    action.setText("Test");
    action.setId(1);
    action.setCheckable(true);
    action.setChecked(false);
    action.setSeparator(false);
    action.setIcon("Icon");

    CandidateAction action2;
    action2 = std::move(action);

    FCITX_ASSERT(action2.text() == "Test");
    FCITX_ASSERT(action2.id() == 1);
    FCITX_ASSERT(action2.isCheckable());
    FCITX_ASSERT(!action2.isChecked());
    FCITX_ASSERT(!action2.isSeparator());
    FCITX_ASSERT(action2.icon() == "Icon");

    CandidateAction action3(action2);

    for (const auto &action : {action2, action3}) {
        FCITX_ASSERT(action.text() == "Test");
        FCITX_ASSERT(action.id() == 1);
        FCITX_ASSERT(action.isCheckable());
        FCITX_ASSERT(!action.isChecked());
        FCITX_ASSERT(!action.isSeparator());
        FCITX_ASSERT(action.icon() == "Icon");
    }
}

class TestTabbedCandidateList : public TabbedCandidateList {
public:
    TestTabbedCandidateList() {
        CandidateAction action;
        action.setText("Tab1");
        action.setId(0);
        actions_.push_back(std::move(action));
    }

    std::span<const CandidateAction> tabActions() override { return actions_; }

    void triggerTabAction(int id) override { triggeredId_ = id; }

    int triggeredId() const { return triggeredId_; }

private:
    std::vector<CandidateAction> actions_;
    int triggeredId_ = -1;
};

void test_tabbed() {
    CommonCandidateList candidatelist;
    candidatelist.setSelectionKey(
        Key::keyListFromString("1 2 3 4 5 6 7 8 9 0"));
    candidatelist.setPageSize(5);
    for (int i = 0; i < 10; i++) {
        candidatelist.append<TestCandidateWord>(i);
    }

    FCITX_ASSERT(candidatelist.toTabbed() == nullptr);

    auto tabbed = std::make_unique<TestTabbedCandidateList>();
    CandidateList *list = &candidatelist;
    FCITX_ASSERT(list->toTabbed() == nullptr);

    candidatelist.setTabbedImpl(std::move(tabbed));
    FCITX_ASSERT(candidatelist.toTabbed() != nullptr);
    FCITX_ASSERT(list->toTabbed() != nullptr);

    auto actions = candidatelist.toTabbed()->tabActions();
    FCITX_ASSERT(actions.size() == 1);
    FCITX_ASSERT(actions[0].text() == "Tab1");
    FCITX_ASSERT(actions[0].id() == 0);

    candidatelist.toTabbed()->triggerTabAction(0);
    FCITX_ASSERT(
        static_cast<TestTabbedCandidateList *>(candidatelist.toTabbed())
            ->triggeredId() == 0);
}

void testFilter() {
    CommonCandidateList candidatelist;
    candidatelist.setSelectionKey(
        Key::keyListFromString("1 2 3 4 5 6 7 8 9 0"));
    candidatelist.setPageSize(3);
    for (int i = 0; i < 10; i++) {
        candidatelist.append<TestCandidateWord>(i);
    }

    FCITX_ASSERT(candidatelist.totalSize() == 10);
    FCITX_ASSERT(candidatelist.size() == 3);

    // Filter to even numbers
    candidatelist.setFilter([](const CandidateWord &word) {
        return std::stoi(word.text().toString()) % 2 == 0;
    });

    FCITX_ASSERT(candidatelist.totalSize() == 5);
    FCITX_ASSERT(candidatelist.size() == 3);
    FCITX_ASSERT(candidatelist.totalPages() == 2);
    FCITX_ASSERT(candidatelist.currentPage() == 0);
    FCITX_ASSERT(!candidatelist.hasPrev());
    FCITX_ASSERT(candidatelist.hasNext());

    // Check filtered candidates: 0, 2, 4, 6, 8
    FCITX_ASSERT(candidatelist.candidate(0).text().toString() == "0");
    FCITX_ASSERT(candidatelist.candidate(1).text().toString() == "2");
    FCITX_ASSERT(candidatelist.candidate(2).text().toString() == "4");
    FCITX_ASSERT(candidatelist.candidateFromAll(3).text().toString() == "6");
    FCITX_ASSERT(candidatelist.candidateFromAll(4).text().toString() == "8");

    // Page navigation with filter
    candidatelist.next();
    FCITX_ASSERT(candidatelist.currentPage() == 1);
    FCITX_ASSERT(candidatelist.size() == 2);
    FCITX_ASSERT(candidatelist.candidate(0).text().toString() == "6");
    FCITX_ASSERT(candidatelist.candidate(1).text().toString() == "8");
    FCITX_ASSERT(candidatelist.hasNext() == false);

    candidatelist.prev();
    FCITX_ASSERT(candidatelist.currentPage() == 0);

    // clearFilter restores everything
    candidatelist.clearFilter();
    FCITX_ASSERT(candidatelist.totalSize() == 10);
    FCITX_ASSERT(candidatelist.size() == 3);
    FCITX_ASSERT(candidatelist.candidate(0).text().toString() == "0");
    FCITX_ASSERT(candidatelist.candidateFromAll(9).text().toString() == "9");

    // Re-apply filter and verify modification clears it
    candidatelist.setFilter([](const CandidateWord &word) {
        return std::stoi(word.text().toString()) % 2 == 0;
    });
    FCITX_ASSERT(candidatelist.totalSize() == 5);

    // insert clears filter
    candidatelist.insert(0, std::make_unique<TestCandidateWord>(99));
    FCITX_ASSERT(candidatelist.totalSize() == 11);
    FCITX_ASSERT(candidatelist.candidateFromAll(0).text().toString() == "99");

    // Re-apply and test remove clears filter
    candidatelist.setFilter([](const CandidateWord &word) {
        return std::stoi(word.text().toString()) % 2 == 0;
    });
    FCITX_ASSERT(candidatelist.totalSize() == 5);
    candidatelist.remove(1);
    FCITX_ASSERT(candidatelist.totalSize() == 10);

    // Re-apply filter: even items among [99, 1, 2, 3, 4, 5, 6, 7, 8, 9] = 4
    candidatelist.setFilter([](const CandidateWord &word) {
        return std::stoi(word.text().toString()) % 2 == 0;
    });
    FCITX_ASSERT(candidatelist.totalSize() == 4);

    // replace clears filter
    candidatelist.replace(0, std::make_unique<TestCandidateWord>(77));
    FCITX_ASSERT(candidatelist.totalSize() == 10);
    FCITX_ASSERT(candidatelist.candidateFromAll(0).text().toString() == "77");

    // Re-apply filter
    candidatelist.setFilter([](const CandidateWord &word) {
        return std::stoi(word.text().toString()) % 2 == 0;
    });
    FCITX_ASSERT(candidatelist.totalSize() == 4);

    // move clears filter
    candidatelist.move(0, 2);
    FCITX_ASSERT(candidatelist.totalSize() == 10);

    // Re-apply filter
    candidatelist.setFilter([](const CandidateWord &word) {
        return std::stoi(word.text().toString()) % 2 == 0;
    });
    FCITX_ASSERT(candidatelist.totalSize() == 4);

    candidatelist.setPage(1);
    candidatelist.setFilter(
        [](const CandidateWord & /*word*/) { return false; });
    FCITX_ASSERT(candidatelist.size() == 0);
    FCITX_ASSERT(candidatelist.totalSize() == 0);

    // clear clears filter
    candidatelist.clear();
    FCITX_ASSERT(candidatelist.totalSize() == 0);
}

} // namespace

int main() {
    test_basic();
    test_faulty_placeholder();
    test_label();
    test_comment();
    test_cursor();
    test_candidateaction();
    test_tabbed();
    testFilter();
    return 0;
}
