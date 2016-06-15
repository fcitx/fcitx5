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
#include "fcitx-utils/standardpath.h"
#include "fcitx-config/rawconfig.h"
#include "fcitx-config/iniparser.h"
#include "inputmethodconfig_p.h"
#include <fcntl.h>

namespace fcitx {

class InputMethodManagerPrivate {
public:
    int currentGroup;
    std::vector<InputMethodGroup> groups;
};

InputMethodManager::InputMethodManager() {}

InputMethodManager::~InputMethodManager() {}

void InputMethodManager::load() {
    StandardPath path;
    auto files = path.multiOpenAll(StandardPath::Type::Data, "fcitx5/addon", O_RDONLY, filter::Suffix(".conf"));
    for (const auto &file : files) {
        auto &files = file.second;
        RawConfig config;
        // reverse the order, so we end up parse user file at last.
        for (auto iter = files.rbegin(), end = files.rend(); iter != end; iter++) {
            auto fd = iter->first;
            readFromIni(config, fd);
        }

        InputMethodConfig imConfig;
        imConfig.load(config);
    }
}

int InputMethodManager::groupCount() const {
    FCITX_D();
    return d->groups.size();
}
}
