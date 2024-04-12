/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include <stdexcept>
#include "fcitx-utils/log.h"
#include "fcitx/candidatelist.h"

using namespace fcitx;
int selected = 0;

class TestCandidateWord : public CandidateWord {
public:
    TestCandidateWord(int number, Text comment = {})
        : CandidateWord(Text(std::to_string(number))), number_(number) {
        setComment(std::move(comment));
    }
    void select(InputContext *) const override { selected = number_; }

private:
    int number_;
};

class PlaceHolderCandidateWord : public CandidateWord {
public:
    PlaceHolderCandidateWord(int number)
        : CandidateWord(Text(std::to_string(number))), number_(number) {
        setPlaceHolder(true);
    }
    void select(InputContext *) const override { selected = number_; }

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

int main() {
    test_basic();
    test_faulty_placeholder();
    test_label();
    test_comment();
    test_cursor();
    return 0;
}
