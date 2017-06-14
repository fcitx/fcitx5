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
#ifndef _FCITX_UTILS_LOG_H_
#define _FCITX_UTILS_LOG_H_

#include "fcitxutils_export.h"
#include <fcitx-utils/fs.h>
#include <fcitx-utils/key.h>
#include <iostream>
#include <string>
#include <type_traits>

namespace fcitx {

enum LogLevel : int { None = 0, Error = 1, Warn = 2, Info = 3, Debug = 4 };

#define FCITX_SIMPLE_LOG(TYPE)                                                 \
    inline LogMessageBuilder &operator<<(TYPE v) {                             \
        if (writeLog_) {                                                       \
            out_ << v;                                                         \
        }                                                                      \
        return *this;                                                          \
    }

class FCITXUTILS_EXPORT Log {
public:
    static void setLogLevel(LogLevel l);
    static void setLogLevel(std::underlying_type_t<LogLevel> l);
    static LogLevel logLevel();
    static bool checkLogLevel(LogLevel l);

private:
    static LogLevel level_;
};

class FCITXUTILS_EXPORT LogMessageBuilder {
public:
    inline LogMessageBuilder(std::ostream &out, LogLevel l)
        : out_(out), writeLog_(Log::checkLogLevel(l)) {
        if (!writeLog_) {
            return;
        }
        switch (l) {
        case LogLevel::Debug:
            out_ << "D";
            break;
        case LogLevel::Info:
            out_ << "I";
            break;
        case LogLevel::Warn:
            out_ << "W";
            break;
        case LogLevel::Error:
            out_ << "E";
            break;
        default:
            break;
        }
        out_ << " ";
    }
    inline ~LogMessageBuilder() {
        if (writeLog_) {
            out_ << std::endl;
        }
    }

    inline LogMessageBuilder &operator<<(const std::string &s) {
        *this << s.c_str();
        return *this;
    }

    inline LogMessageBuilder &operator<<(const Key &key) {
        if (writeLog_) {
            out_ << "Key(" << key.toString()
                 << " states=" << key.states().toInteger() << ")";
        }
        return *this;
    }

    FCITX_SIMPLE_LOG(char)
    FCITX_SIMPLE_LOG(bool)
    FCITX_SIMPLE_LOG(signed short)
    FCITX_SIMPLE_LOG(unsigned short)
    FCITX_SIMPLE_LOG(signed int)
    FCITX_SIMPLE_LOG(unsigned int)
    FCITX_SIMPLE_LOG(signed long)
    FCITX_SIMPLE_LOG(unsigned long)
    FCITX_SIMPLE_LOG(float)
    FCITX_SIMPLE_LOG(double)
    FCITX_SIMPLE_LOG(char *)
    FCITX_SIMPLE_LOG(const char *)
    FCITX_SIMPLE_LOG(const void *)
    FCITX_SIMPLE_LOG(long double)
    FCITX_SIMPLE_LOG(signed long long)
    FCITX_SIMPLE_LOG(unsigned long long)

private:
    std::ostream &out_;
    bool writeLog_;
};
}

#define FCITX_LOG(LEVEL)                                                       \
    ::fcitx::LogMessageBuilder(std::cerr, ::fcitx::LogLevel::LEVEL)            \
        << ::fcitx::fs::baseName(__FILE__) << ":" << __LINE__ << "] "

#endif // _FCITX_UTILS_LOG_H_
