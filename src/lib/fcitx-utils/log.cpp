/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "log.h"
#include <chrono>
#include <mutex>
#include <type_traits>
#include <unordered_set>
#include <fmt/format.h>
#if FMT_VERSION >= 50300
#include <fmt/chrono.h>
#endif
#include "fs.h"
#include "stringutils.h"

namespace fcitx {

namespace {

FCITX_DEFINE_LOG_CATEGORY(defaultCategory, "default");

bool validateLogLevel(std::underlying_type_t<LogLevel> l) {
    return (l >= 0 &&
            l <= std::underlying_type_t<LogLevel>(LogLevel::LastLogLevel));
}

class LogRegistry {
public:
    static LogRegistry &instance() {
        static LogRegistry instance_;
        return instance_;
    }

    void registerCategory(LogCategory &category) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!categories_.count(&category)) {
            categories_.insert(&category);
            applyRule(&category);
        }
    }

    void unregisterCategory(LogCategory &category) {
        std::lock_guard<std::mutex> lock(mutex_);
        categories_.erase(&category);
    }

    void setLogRule(const std::string &ruleString) {
        std::lock_guard<std::mutex> lock(mutex_);

        rules_.clear();
        auto rules = stringutils::split(ruleString, ",");
        rules_.reserve(rules.size());
        for (const auto &rule : rules) {
            auto ruleItem = stringutils::split(rule, "=");
            if (ruleItem.size() != 2) {
                continue;
            }
            auto &name = ruleItem[0];
            try {
                auto level = std::stoi(ruleItem[1]);
                if (validateLogLevel(level)) {
                    rules_.emplace_back(name, static_cast<LogLevel>(level));
                }
            } catch (const std::exception &) {
                continue;
            }
        }

        for (auto *category : categories_) {
            applyRule(category);
        }
    }

    void applyRule(LogCategory *category) {
        category->resetLogLevel();
        for (auto &rule : rules_) {
            if (rule.first == "*" || rule.first == category->name()) {
                category->setLogLevel(rule.second);
            }
        }
    }

private:
    std::unordered_set<LogCategory *> categories_;
    std::vector<std::pair<std::string, LogLevel>> rules_;
    std::mutex mutex_;
};
} // namespace

class LogCategoryPrivate {
public:
    LogCategoryPrivate(const char *name, LogLevel level)
        : name_(name), level_(level), defaultLevel_(level) {}

    std::string name_;
    LogLevel level_;
    LogLevel defaultLevel_;
};

LogCategory::LogCategory(const char *name, LogLevel level)
    : d_ptr(std::make_unique<LogCategoryPrivate>(name, level)) {
    LogRegistry::instance().registerCategory(*this);
}

LogCategory::~LogCategory() {
    LogRegistry::instance().unregisterCategory(*this);
}

bool LogCategory::checkLogLevel(LogLevel l) const {
    FCITX_D();
    return l != LogLevel::NoLog &&
           static_cast<std::underlying_type_t<LogLevel>>(l) <=
               static_cast<std::underlying_type_t<LogLevel>>(d->level_);
}

void LogCategory::resetLogLevel() {
    FCITX_D();
    d->level_ = d->defaultLevel_;
}

void LogCategory::setLogLevel(std::underlying_type_t<LogLevel> l) {
    if (validateLogLevel(l)) {
        setLogLevel(static_cast<LogLevel>(l));
    }
}

void LogCategory::setLogLevel(LogLevel l) {
    FCITX_D();
    d->level_ = l;
}

LogLevel LogCategory::logLevel() const {
    FCITX_D();
    return d->level_;
}

const std::string &LogCategory::name() const {
    FCITX_D();
    return d->name_;
}

bool LogCategory::fatalWrapper(LogLevel level) const {
    // If level if fatal and we don't write fatal log, abort right away.
    bool needLog = checkLogLevel(level);
    if (level == LogLevel::Fatal && !needLog) {
        std::abort();
    }
    return needLog;
}

bool LogCategory::fatalWrapper2(LogLevel level) {
    if (level == LogLevel::Fatal) {
        std::abort();
    }
    return false;
}

const LogCategory &Log::defaultCategory() { return fcitx::defaultCategory(); }

void Log::setLogRule(const std::string &ruleString) {
    LogRegistry::instance().setLogRule(ruleString);
}

LogMessageBuilder::LogMessageBuilder(std::ostream &out, LogLevel l,
                                     const char *filename, int lineNumber)
    : out_(out) {
    switch (l) {
    case LogLevel::Fatal:
        out_ << "F";
        break;
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

#if FMT_VERSION >= 50300
    auto now = std::chrono::system_clock::now();
    auto floor = std::chrono::floor<std::chrono::seconds>(now);
    auto micro =
        std::chrono::duration_cast<std::chrono::microseconds>(now - floor);
    auto t = fmt::localtime(std::chrono::system_clock::to_time_t(now));
    out_ << fmt::format("{:%F %T}.{:06d}", t, micro.count()) << " ";
#endif
    out_ << filename << ":" << lineNumber << "] ";
}

LogMessageBuilder::~LogMessageBuilder() { out_ << std::endl; }
} // namespace fcitx
