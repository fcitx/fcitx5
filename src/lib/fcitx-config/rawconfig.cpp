/*
 * SPDX-FileCopyrightText: 2015-2015 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "rawconfig.h"
#include <list>
#include <unordered_map>
#include <utility>
#include "fcitx-utils/misc_p.h"
#include "fcitx-utils/stringutils.h"

namespace fcitx {

class RawConfigPrivate : public QPtrHolder<RawConfig> {
public:
    RawConfigPrivate(RawConfig *q, std::string _name)
        : QPtrHolder(q), name_(std::move(_name)), lineNumber_(0) {}
    RawConfigPrivate(RawConfig *q, const RawConfigPrivate &other)
        : QPtrHolder(q), value_(other.value_), comment_(other.comment_),
          lineNumber_(other.lineNumber_) {}

    RawConfigPrivate &operator=(const RawConfigPrivate &other) {
        if (&other == this) {
            return *this;
        }
        // There is no need to copy "name_", because name refers to its parent.
        // Make a copy of value, comment, and lineNumber, because this "other"
        // might be our parent.
        auto value = other.value_;
        auto comment = other.comment_;
        auto lineNumber = other.lineNumber_;
        OrderedMap<std::string, std::shared_ptr<RawConfig>> newSubItems;
        for (const auto &item : other.subItems_) {
            auto result = newSubItems[item.first] =
                q_func()->createSub(item.second->name());
            *result = *item.second;
        }
        value_ = std::move(value);
        comment_ = std::move(comment);
        lineNumber_ = lineNumber;
        detachSubItems();
        subItems_ = std::move(newSubItems);
        return *this;
    }

    std::shared_ptr<RawConfig> getNonexistentRawConfig(RawConfig *q,
                                                       const std::string &key) {
        auto result = subItems_[key] = q->createSub(key);
        return result;
    }

    static std::shared_ptr<const RawConfig>
    getNonexistentRawConfig(const RawConfig *, const std::string &) {
        return nullptr;
    }

    template <typename T, typename U>
    static std::shared_ptr<T>
    getRawConfigHelper(T &that, const std::string &path, U callback) {
        auto *cur = &that;
        std::shared_ptr<T> result;
        for (std::string::size_type pos = 0, new_pos = path.find('/', pos);
             pos != std::string::npos && cur;
             pos = ((std::string::npos == new_pos) ? new_pos : (new_pos + 1)),
                                    new_pos = path.find('/', pos)) {
            auto key = path.substr(pos, (std::string::npos == new_pos)
                                            ? new_pos
                                            : (new_pos - pos));
            auto iter = cur->d_func()->subItems_.find(key);
            if (iter == cur->d_func()->subItems_.end()) {
                result = cur->d_func()->getNonexistentRawConfig(cur, key);
            } else {
                result = iter->second;
            }
            cur = result.get();

            if (cur) {
                callback(*cur, path.substr(0, new_pos));
            }
        }
        return result;
    }

    template <typename T>
    static bool
    visitHelper(T &that,
                std::function<bool(T &, const std::string &path)> callback,
                bool recursive, const std::string &pathPrefix) {
        auto d = that.d_func();
        for (const auto &pair : d->subItems_) {
            std::shared_ptr<T> item = pair.second;
            auto newPathPrefix = pathPrefix.empty()
                                     ? item->name()
                                     : pathPrefix + "/" + item->name();
            if (!callback(*item, newPathPrefix)) {
                return false;
            }
            if (recursive) {
                if (!visitHelper(*item, callback, recursive, newPathPrefix)) {
                    return false;
                }
            }
        }
        return true;
    }

    void detachSubItems() {
        for (const auto &pair : subItems_) {
            pair.second->d_func()->parent_ = nullptr;
        }
    }

    RawConfig *parent_ = nullptr;
    const std::string name_;
    std::string value_;
    std::string comment_;
    OrderedMap<std::string, std::shared_ptr<RawConfig>> subItems_;
    unsigned int lineNumber_;
};

RawConfig::RawConfig() : RawConfig("") {}

RawConfig::RawConfig(std::string name)
    : d_ptr(std::make_unique<RawConfigPrivate>(this, std::move(name))) {}

RawConfig::~RawConfig() {
    FCITX_D();
    d->detachSubItems();
}

RawConfig::RawConfig(const RawConfig &other)
    : d_ptr(std::make_unique<RawConfigPrivate>(this, *other.d_ptr)) {
    for (const auto &item : other.d_func()->subItems_) {
        *get(item.first, true) = *item.second;
    }
}
RawConfig &RawConfig::operator=(const RawConfig &other) {
    *d_ptr = *other.d_ptr;
    return *this;
}

std::shared_ptr<RawConfig> RawConfig::get(const std::string &path,
                                          bool create) {
    auto dummy = [](const RawConfig &, const std::string &) {};
    if (create) {
        return RawConfigPrivate::getRawConfigHelper(*this, path, dummy);
    }
    return std::const_pointer_cast<RawConfig>(
        RawConfigPrivate::getRawConfigHelper<const RawConfig>(*this, path,
                                                              dummy));
}

std::shared_ptr<const RawConfig> RawConfig::get(const std::string &path) const {
    auto dummy = [](const RawConfig &, const std::string &) {};
    return RawConfigPrivate::getRawConfigHelper(*this, path, dummy);
}

bool RawConfig::remove(const std::string &path) {
    auto pos = path.rfind('/');
    auto *root = this;
    if (pos == 0 || pos + 1 == path.size()) {
        return false;
    }

    if (pos != std::string::npos) {
        root = get(path.substr(0, pos)).get();
    }
    return root->d_func()->subItems_.erase(path.substr(pos + 1)) > 0;
}

void RawConfig::removeAll() {
    FCITX_D();
    d->subItems_.clear();
}

void RawConfig::setValue(std::string value) {
    FCITX_D();
    d->value_ = std::move(value);
}

void RawConfig::setComment(std::string comment) {
    FCITX_D();
    d->comment_ = std::move(comment);
}

void RawConfig::setLineNumber(unsigned int lineNumber) {
    FCITX_D();
    d->lineNumber_ = lineNumber;
}

const std::string &RawConfig::name() const {
    FCITX_D();
    return d->name_;
}

const std::string &RawConfig::comment() const {
    FCITX_D();
    return d->comment_;
}

const std::string &RawConfig::value() const {
    FCITX_D();
    return d->value_;
}

unsigned int RawConfig::lineNumber() const {
    FCITX_D();
    return d->lineNumber_;
}

bool RawConfig::hasSubItems() const {
    FCITX_D();
    return !d->subItems_.empty();
}

size_t RawConfig::subItemsSize() const {
    FCITX_D();
    return d->subItems_.size();
}

std::vector<std::string> RawConfig::subItems() const {
    FCITX_D();
    std::vector<std::string> result;
    result.reserve(d->subItems_.size());
    for (const auto &pair : d->subItems_) {
        result.push_back(pair.first);
    }
    return result;
}

RawConfig *RawConfig::parent() const {
    FCITX_D();
    return d->parent_;
}

std::shared_ptr<RawConfig> RawConfig::detach() {
    FCITX_D();
    if (!d->parent_) {
        return {};
    }
    auto ref = d->parent_->get(d->name_);
    d->parent_->d_func()->subItems_.erase(d->name_);
    d->parent_ = nullptr;
    return ref;
}

bool RawConfig::visitSubItems(
    std::function<bool(RawConfig &, const std::string &path)> callback,
    const std::string &path, bool recursive, const std::string &pathPrefix) {
    auto *root = this;
    std::shared_ptr<RawConfig> subItem;
    if (!path.empty()) {
        subItem = get(path);
        root = subItem.get();
    }

    if (!root) {
        return true;
    }

    return RawConfigPrivate::visitHelper(*root, std::move(callback), recursive,
                                         pathPrefix);
}

bool RawConfig::visitSubItems(
    std::function<bool(const RawConfig &, const std::string &path)> callback,
    const std::string &path, bool recursive,
    const std::string &pathPrefix) const {
    const auto *root = this;
    std::shared_ptr<const RawConfig> subItem;
    if (!path.empty()) {
        subItem = get(path);
        root = subItem.get();
    }

    if (!root) {
        return true;
    }

    return RawConfigPrivate::visitHelper(*root, std::move(callback), recursive,
                                         pathPrefix);
}

void RawConfig::visitItemsOnPath(
    std::function<void(RawConfig &, const std::string &path)> callback,
    const std::string &path) {
    RawConfigPrivate::getRawConfigHelper(*this, path, std::move(callback));
}
void RawConfig::visitItemsOnPath(
    std::function<void(const RawConfig &, const std::string &path)> callback,
    const std::string &path) const {
    RawConfigPrivate::getRawConfigHelper(*this, path, std::move(callback));
}

std::shared_ptr<RawConfig> RawConfig::createSub(std::string name) {
    struct RawSubConfig : public RawConfig {
        RawSubConfig(RawConfig *parent, std::string name)
            : RawConfig(std::move(name)) {
            FCITX_D();
            d->parent_ = parent;
        }
    };
    return std::make_shared<RawSubConfig>(this, std::move(name));
}

LogMessageBuilder &operator<<(LogMessageBuilder &log, const RawConfig &config) {
    log << "RawConfig(=" << config.value();
    config.visitSubItems(
        [&log](const RawConfig &subConfig, const std::string &path) {
            log << ", " << path << "=" << subConfig.value();
            return true;
        },
        "", true);
    log << ")";
    return log;
}
} // namespace fcitx
