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

#include <list>
#include "inputmethodmanager.h"

namespace fcitx {

class InputMethodManagerPrivate {
public:
    int currentGroup;
    std::vector<InputMethodGroup> groups;
};

InputMethodManager::InputMethodManager() {}

InputMethodManager::~InputMethodManager() {}

int InputMethodManager::groupCount() const {
    FCITX_D();
    return d->groups.size();
}
}
