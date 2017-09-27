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
#ifndef _FCITX_UTILS_LOG_H_
#define _FCITX_UTILS_LOG_H_

/// \addtogroup FcitxUtils
/// \{
/// \file
/// \brief Log utilities.

#include "fcitxutils_export.h"
#include <cstdlib>
#include <fcitx-utils/key.h>
#include <iostream>
#include <memory>
#include <string>
#include <type_traits>

namespace fcitx {

/// \brief LogLevel from high to low.
enum LogLevel : int {
    None = 0,
    Fatal = 1, /// Fatal will always abort regardless of log or not.
    Error = 2,
    Warn = 3,
    Info = 4,
    Debug = 5,
    LastLogLevel = Debug
};

#define FCITX_SIMPLE_LOG(TYPE)                                                 \
    inline LogMessageBuilder &operator<<(TYPE v) {                             \
        out_ << v;                                                             \
        return *this;                                                          \
    }

class LogCategoryPrivate;
class FCITXUTILS_EXPORT LogCategory {
public:
    LogCategory(const char *name, LogLevel level = LogLevel::Info);
    ~LogCategory();

    LogLevel logLevel() const;
    bool checkLogLevel(LogLevel l) const;
    void setLogLevel(LogLevel l);
    void setLogLevel(std::underlying_type_t<LogLevel> l);
    void resetLogLevel();
    const std::string &name() const;

    // Helper function
    bool fatalWrapper(LogLevel l) const;
    bool fatalWrapper2(LogLevel l) const;

private:
    FCITX_DECLARE_PRIVATE(LogCategory);
    std::unique_ptr<LogCategoryPrivate> d_ptr;
};

class FCITXUTILS_EXPORT Log {
public:
    static const LogCategory &defaultCategory();
    static void setLogRule(const std::string &rule);
};

class FCITXUTILS_EXPORT LogMessageBuilder {
public:
    LogMessageBuilder(std::ostream &out, LogLevel l,
                      const std::string &filename, int lineNumber);
    ~LogMessageBuilder();

    inline LogMessageBuilder &operator<<(const std::string &s) {
        *this << s.c_str();
        return *this;
    }

    inline LogMessageBuilder &operator<<(const Key &key) {
        out_ << "Key(" << key.toString()
             << " states=" << key.states().toInteger() << ")";
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

    // For some random type, use ostream.
    template <typename T>
    FCITX_SIMPLE_LOG(T)

    private : std::ostream &out_;
    LogLevel level_;
};
}

#define FCITX_LOGC_IF(CATEGORY, LEVEL, CONDITION)                              \
    for (bool fcitxLogEnabled =                                                \
             (CONDITION) && CATEGORY().fatalWrapper(::fcitx::LogLevel::LEVEL); \
         fcitxLogEnabled;                                                      \
         fcitxLogEnabled = CATEGORY().fatalWrapper2(::fcitx::LogLevel::LEVEL)) \
    ::fcitx::LogMessageBuilder(std::cerr, ::fcitx::LogLevel::LEVEL, __FILE__,  \
                               __LINE__)

#define FCITX_LOGC(CATEGORY, LEVEL)                                            \
    for (bool fcitxLogEnabled =                                                \
             CATEGORY().fatalWrapper(::fcitx::LogLevel::LEVEL);                \
         fcitxLogEnabled;                                                      \
         fcitxLogEnabled = CATEGORY().fatalWrapper2(::fcitx::LogLevel::LEVEL)) \
    ::fcitx::LogMessageBuilder(std::cerr, ::fcitx::LogLevel::LEVEL, __FILE__,  \
                               __LINE__)

#define FCITX_LOG(LEVEL) FCITX_LOGC(::fcitx::Log::defaultCategory, LEVEL)

#define FCITX_LOG_IF(LEVEL, CONDITION)                                         \
    FCITX_LOGC_IF(::fcitx::Log::defaultCategory, LEVEL, CONDITION)

#define FCITX_ASSERT(EXPR) FCITX_LOG_IF(Fatal, !(EXPR)) << #EXPR << " failed"

#define FCITX_DEFINE_LOG_CATEGORY(name, ...)                                   \
    const ::fcitx::LogCategory &name() {                                       \
        static const ::fcitx::LogCategory category(__VA_ARGS__);               \
        return category;                                                       \
    }

#endif // _FCITX_UTILS_LOG_H_
