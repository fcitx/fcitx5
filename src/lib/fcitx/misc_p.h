//
// Copyright (C) 2016~2016 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
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
#ifndef _FCITX_MISC_P_H_
#define _FCITX_MISC_P_H_

#include "fcitx-utils/misc_p.h"
#include "fcitx/candidatelist.h"
#include <string>
#include <type_traits>

namespace fcitx {

static inline std::pair<std::string, std::string>
parseLayout(const std::string &layout) {
    auto pos = layout.find('-');
    if (pos == std::string::npos) {
        return {layout, ""};
    }
    return {layout.substr(0, pos), layout.substr(pos + 1)};
}

static inline const CandidateWord *
nthCandidateIgnorePlaceholder(const CandidateList &candidateList, int idx) {
    int total = 0;
    if (idx < 0 || idx >= candidateList.size()) {
        return nullptr;
    }
    for (int i = 0, e = candidateList.size(); i < e; i++) {
        auto &candidate = candidateList.candidate(i);
        if (candidate.isPlaceHolder()) {
            continue;
        }
        if (idx == total) {
            return &candidate;
        }
        ++total;
    }
    return nullptr;
}

} // namespace fcitx

#endif // _FCITX_MISC_P_H_
