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

#include "text.h"
#include <sstream>
#include <vector>
namespace fcitx {

class TextPrivate {
public:
    std::vector<std::pair<std::string, TextFormatFlags>> texts_;
    int cursor_ = 0;
};

Text::Text() : d_ptr(std::make_unique<TextPrivate>()) {}

Text::~Text() {}

void Text::clear() {
    FCITX_D();
    d->texts_.clear();
    setCursor();
}

int Text::cursor() const {
    FCITX_D();
    return d->cursor_;
}

void Text::setCursor(int pos) {
    FCITX_D();
    d->cursor_ = pos;
}

void Text::append(const std::string &str, TextFormatFlags flag) {
    FCITX_D();
    d->texts_.emplace_back(str, flag);
}

const std::string &Text::stringAt(int idx) const {
    FCITX_D();
    return d->texts_[idx].first;
}

TextFormatFlags Text::formatAt(int idx) const {
    FCITX_D();
    return d->texts_[idx].second;
}

size_t Text::size() const {
    FCITX_D();
    return d->texts_.size();
}

std::string Text::toString() const {
    FCITX_D();
    std::stringstream ss;
    for (auto &p : d->texts_) {
        ss << p.first;
    }

    return ss.str();
}
}
