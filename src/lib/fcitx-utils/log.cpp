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

#include "log.h"
#include "fs.h"
#include "stringutils.h"
#include <mutex>
#include <type_traits>
#include <unordered_set>

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

        for (auto category : categories_) {
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
}

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
    return l != LogLevel::None &&
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

bool LogCategory::fatalWrapper2(LogLevel level) const {
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
                                     const std::string &filename,
                                     int lineNumber)
    : out_(out) {
    switch (l) {
    case LogLevel::Fatal:
        out_ << "D";
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
    out_ << " ";
    out_ << fs::baseName(filename) << ":" << lineNumber << "] ";
}

LogMessageBuilder::~LogMessageBuilder() { out_ << std::endl; }
}
