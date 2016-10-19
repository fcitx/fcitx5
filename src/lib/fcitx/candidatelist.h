/*
 * Copyright (C) 2016~2016 by CSSlayer
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
#ifndef _FCITX_CANDIDATELIST_H_
#define _FCITX_CANDIDATELIST_H_

#include <fcitx-utils/dynamictrackableobject.h>
#include <fcitx/text.h>

namespace fcitx {

class CandidateWord : public DynamicTrackableObject {
public:
    CandidateWord();

    void remove();

    bool isPlaceHolder() const;
    void setPlaceHolder(bool isPlaceHolder);

    Text &text();

    FCITX_DECLARE_SIGNAL(CandidateWord, Selected, void());

private:
};

class CandidateListPrivate;

class CandidateList {
public:
    CandidateList();
    ~CandidateList();

    CandidateWord &append();
    CandidateWord &insert(int idx);
    void clear();

private:
    std::unique_ptr<CandidateListPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(CandidateList);
};
}

#endif // _FCITX_CANDIDATELIST_H_
