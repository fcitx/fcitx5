/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_LOG_H_
#define _FCITX_UTILS_LOG_H_

/// \addtogroup FcitxUtils
/// \{
/// \file
/// \brief Log utilities.

#include <cstdlib>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <fcitx-utils/fs.h>
#include <fcitx-utils/key.h>
#include <fcitx-utils/metastring.h>
#include <fcitx-utils/misc.h>
#include <fcitx-utils/tuplehelpers.h>
#include "fcitxutils_export.h"

namespace fcitx {

/// \brief LogLevel from high to low.
enum LogLevel : int {
    NoLog = 0,
    /// Fatal will always abort regardless of log or not.
    Fatal = 1,
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
    static bool fatalWrapper2(LogLevel l);

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
    LogMessageBuilder(std::ostream &out, LogLevel l, const char *filename,
                      int lineNumber);
    ~LogMessageBuilder();

    LogMessageBuilder &self() { return *this; }

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

    template <typename T>
    inline LogMessageBuilder &operator<<(const std::vector<T> &vec) {
        *this << "[";
        printRange(vec.begin(), vec.end());
        *this << "]";
        return *this;
    }

    template <typename T>
    inline LogMessageBuilder &operator<<(const std::list<T> &lst) {
        *this << "list[";
        printRange(lst.begin(), lst.end());
        *this << "]";
        return *this;
    }

    template <typename K, typename V>
    inline LogMessageBuilder &operator<<(const std::pair<K, V> &pair) {
        *this << "(" << pair.first << ", " << pair.second << ")";
        return *this;
    }

    template <typename... Args>
    inline LogMessageBuilder &operator<<(const std::tuple<Args...> &tuple) {
        typename MakeSequence<sizeof...(Args)>::type a;
        *this << "(";
        printWithIndices(a, tuple);
        *this << ")";
        return *this;
    }

    template <typename K, typename V>
    inline LogMessageBuilder &operator<<(const std::unordered_map<K, V> &vec) {
        *this << "{";
        printRange(vec.begin(), vec.end());
        *this << "}";
        return *this;
    }

    template <typename V>
    inline LogMessageBuilder &operator<<(const std::unordered_set<V> &vec) {
        *this << "{";
        printRange(vec.begin(), vec.end());
        *this << "}";
        return *this;
    }

    template <typename K, typename V>
    inline LogMessageBuilder &operator<<(const std::map<K, V> &vec) {
        *this << "{";
        printRange(vec.begin(), vec.end());
        *this << "}";
        return *this;
    }

    template <typename V>
    inline LogMessageBuilder &operator<<(const std::set<V> &vec) {
        *this << "{";
        printRange(vec.begin(), vec.end());
        *this << "}";
        return *this;
    }

private:
    template <typename Iterator>
    void printRange(Iterator begin, Iterator end) {
        bool first = true;
        for (auto &item : MakeIterRange(begin, end)) {
            if (first) {
                first = false;
            } else {
                *this << ", ";
            }
            *this << item;
        }
    }

    template <typename... Args, int... S>
    void printWithIndices(Sequence<S...>, const std::tuple<Args...> &tuple) {
        using swallow = int[];
        (void)swallow{
            0,
            (void(*this << (S == 0 ? "" : ", ") << std::get<S>(tuple)), 0)...};
    }

    std::ostream &out_;
};
} // namespace fcitx

#ifdef FCITX_USE_NO_METASTRING_FILENAME
#define FCITX_LOG_FILENAME_WRAP ::fcitx::fs::baseName(__FILE__).data()
#else
#define FCITX_LOG_FILENAME_WRAP                                                \
    fcitx::MetaStringBasenameType<fcitxMakeMetaString(__FILE__)>::data()
#endif

#define FCITX_LOGC_IF(CATEGORY, LEVEL, CONDITION)                              \
    for (bool fcitxLogEnabled =                                                \
             (CONDITION) && CATEGORY().fatalWrapper(::fcitx::LogLevel::LEVEL); \
         fcitxLogEnabled;                                                      \
         fcitxLogEnabled = CATEGORY().fatalWrapper2(::fcitx::LogLevel::LEVEL)) \
    ::fcitx::LogMessageBuilder(std::cerr, ::fcitx::LogLevel::LEVEL,            \
                               FCITX_LOG_FILENAME_WRAP, __LINE__)              \
        .self()

#define FCITX_LOGC(CATEGORY, LEVEL)                                            \
    for (bool fcitxLogEnabled =                                                \
             CATEGORY().fatalWrapper(::fcitx::LogLevel::LEVEL);                \
         fcitxLogEnabled;                                                      \
         fcitxLogEnabled = CATEGORY().fatalWrapper2(::fcitx::LogLevel::LEVEL)) \
    ::fcitx::LogMessageBuilder(std::cerr, ::fcitx::LogLevel::LEVEL,            \
                               FCITX_LOG_FILENAME_WRAP, __LINE__)              \
        .self()

#define FCITX_LOG(LEVEL) FCITX_LOGC(::fcitx::Log::defaultCategory, LEVEL)

#define FCITX_DEBUG() FCITX_LOG(Debug)
#define FCITX_WARN() FCITX_LOG(Warn)
#define FCITX_INFO() FCITX_LOG(Info)
#define FCITX_ERROR() FCITX_LOG(Error)
#define FCITX_FATAL() FCITX_LOG(Fatal)

#define FCITX_LOG_IF(LEVEL, CONDITION)                                         \
    FCITX_LOGC_IF(::fcitx::Log::defaultCategory, LEVEL, CONDITION)

#define FCITX_ASSERT(...)                                                      \
    FCITX_LOG_IF(Fatal, !(__VA_ARGS__)) << #__VA_ARGS__ << " failed."

#define FCITX_DEFINE_LOG_CATEGORY(name, ...)                                   \
    const ::fcitx::LogCategory &name() {                                       \
        static const ::fcitx::LogCategory category(__VA_ARGS__);               \
        return category;                                                       \
    }

#define FCITX_DECLARE_LOG_CATEGORY(name) const ::fcitx::LogCategory &name()

#endif // _FCITX_UTILS_LOG_H_
