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

#include "fcitx/candidatelist.h"
#include <cassert>
#include <iostream>

using namespace fcitx;
int selected = 0;

class TestCandidateWord : public CandidateWord {
public:
    TestCandidateWord(int number) : CandidateWord(Text(std::to_string(number))), number_(number) {}
    void select() const override { selected = number_; }

private:
    int number_;
};

int main() {
    CommonCandidateList candidatelist;
    candidatelist.setSelectionKey(Key::keyListFromString("1 2 3 4 5 6 7 8 9 0"));
    candidatelist.setPageSize(3);
    for (int i = 0; i < 10; i++) {
        candidatelist.append(new TestCandidateWord(i));
    }

    assert(candidatelist.size() == 3);
    assert(candidatelist.label(0).toString() == "1");
    assert(candidatelist.candidate(0).text().toString() == "0");
    assert(!candidatelist.hasPrev());
    assert(candidatelist.hasNext());

    assert(candidatelist.totalPages() == 4);
    assert(candidatelist.currentPage() == 0);

    // BulkCandidateList
    for (int i = 0; i < 10; i++) {
        assert(candidatelist.candidateFromAll(i).text().toString() == std::to_string(i));
    }

    assert(candidatelist.totalSize() == 10);

    candidatelist.next();

    assert(candidatelist.size() == 3);
    assert(candidatelist.label(0).toString() == "1");
    assert(candidatelist.candidate(0).text().toString() == "3");
    assert(candidatelist.hasPrev());
    assert(candidatelist.hasNext());

    assert(candidatelist.totalPages() == 4);
    assert(candidatelist.currentPage() == 1);

    // BulkCandidateList
    for (int i = 0; i < 10; i++) {
        assert(candidatelist.candidateFromAll(i).text().toString() == std::to_string(i));
    }

    assert(candidatelist.totalSize() == 10);

    candidatelist.next();
    candidatelist.next();

    assert(candidatelist.size() == 1);
    assert(candidatelist.label(0).toString() == "1");
    assert(candidatelist.candidate(0).text().toString() == "9");
    assert(candidatelist.hasPrev());
    assert(!candidatelist.hasNext());

    assert(candidatelist.totalPages() == 4);
    assert(candidatelist.currentPage() == 3);

    // BulkCandidateList
    for (int i = 0; i < 10; i++) {
        assert(candidatelist.candidateFromAll(i).text().toString() == std::to_string(i));
    }

    assert(candidatelist.totalSize() == 10);

    candidatelist.remove(0);
    assert(candidatelist.size() == 3);
    assert(candidatelist.label(0).toString() == "1");
    assert(candidatelist.candidate(0).text().toString() == "7");
    assert(candidatelist.hasPrev());
    assert(!candidatelist.hasNext());

    assert(candidatelist.totalPages() == 3);
    assert(candidatelist.currentPage() == 2);

    // BulkCandidateList
    for (int i = 0; i < 9; i++) {
        assert(candidatelist.candidateFromAll(i).text().toString() == std::to_string(i + 1));
    }

    assert(candidatelist.totalSize() == 9);

    candidatelist.move(0, 4);
    assert(candidatelist.totalSize() == 9);

    // result
    int expect1[] = {2, 3, 4, 5, 1, 6, 7, 8, 9};
    for (int i = 0; i < 9; i++) {
        assert(candidatelist.candidateFromAll(i).text().toString() == std::to_string(expect1[i]));
    }
    assert(candidatelist.totalSize() == 9);

    candidatelist.move(8, 3);
    int expect2[] = {2, 3, 4, 9, 5, 1, 6, 7, 8};
    for (int i = 0; i < 9; i++) {
        assert(candidatelist.candidateFromAll(i).text().toString() == std::to_string(expect2[i]));
    }
    assert(candidatelist.totalSize() == 9);

    return 0;
}
