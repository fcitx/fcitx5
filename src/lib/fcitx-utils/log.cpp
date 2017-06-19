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

#include "log.h"
#include <type_traits>

namespace fcitx {

LogLevel Log::level_ = LogLevel::Info;

bool Log::checkLogLevel(LogLevel l) {
    return l != LogLevel::None &&
           static_cast<std::underlying_type_t<LogLevel>>(l) <=
               static_cast<std::underlying_type_t<LogLevel>>(level_);
}

void Log::setLogLevel(std::underlying_type_t<LogLevel> l) {
    if (l >= 0 &&
        l <= std::underlying_type_t<LogLevel>(LogLevel::LastLogLevel)) {
        setLogLevel(static_cast<LogLevel>(l));
    }
}

void Log::setLogLevel(LogLevel l) { level_ = l; }

LogLevel Log::logLevel() { return level_; }
}
