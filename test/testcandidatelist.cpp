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

#include "fcitx-utils/log.h"
#include "fcitx/candidatelist.h"

using namespace fcitx;
int selected = 0;

class TestCandidateWord : public CandidateWord {
public:
    TestCandidateWord(int number)
        : CandidateWord(Text(std::to_string(number))), number_(number) {}
    void select(InputContext *) const override { selected = number_; }

private:
    int number_;
};

int main() {
    CommonCandidateList candidatelist;
    candidatelist.setSelectionKey(
        Key::keyListFromString("1 2 3 4 5 6 7 8 9 0"));
    candidatelist.setPageSize(3);
    for (int i = 0; i < 10; i++) {
        candidatelist.append(new TestCandidateWord(i));
    }

    FCITX_ASSERT(candidatelist.size() == 3);
    FCITX_ASSERT(candidatelist.label(0).toString() == "1. ");
    FCITX_ASSERT(candidatelist.candidate(0)->text().toString() == "0");
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
    FCITX_ASSERT(candidatelist.candidate(0)->text().toString() == "3");
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
    FCITX_ASSERT(candidatelist.candidate(0)->text().toString() == "9");
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
    FCITX_ASSERT(candidatelist.candidate(0)->text().toString() == "7");
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

    return 0;
}
