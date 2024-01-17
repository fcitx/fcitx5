/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "matchrule.h"
#include "../stringutils.h"
#include "message.h"
#include "utils_p.h"

namespace fcitx::dbus {

static const char nullArray[] = {'\0', '\0'};

const std::string MatchRule::nullArg{nullArray, nullArray + 1};

class MatchRulePrivate {
public:
    MatchRulePrivate(MessageType type, std::string service,
                     std::string destination, std::string path,
                     std::string interface, std::string name,
                     std::vector<std::string> argumentMatch, bool eavesdrop)
        : type_(type), service_(std::move(service)),
          destination_(std::move(destination)), path_(std::move(path)),
          interface_(std::move(interface)), name_(std::move(name)),
          argumentMatch_(std::move(argumentMatch)), eavesdrop_(eavesdrop),
          rule_(buildRule()) {}

    FCITX_INLINE_DEFINE_DEFAULT_DTOR_COPY_AND_MOVE_WITHOUT_SPEC(
        MatchRulePrivate)

    MessageType type_;
    std::string service_;
    std::string destination_;
    std::string path_;
    std::string interface_;
    std::string name_;
    std::vector<std::string> argumentMatch_;
    bool eavesdrop_ = false;
    std::string rule_;

    std::string buildRule() const {
        std::string result;
        switch (type_) {
        case MessageType::Signal:
            result = "type='signal',";
            break;
        case MessageType::MethodCall:
            result = "type='method_call',";
            break;
        case MessageType::Reply:
            result = "type='method_return',";
            break;
        case MessageType::Error:
            result = "type='error',";
            break;
        default:
            break;
        }
        if (!service_.empty()) {
            result += stringutils::concat("sender='", service_, "',");
        }
        if (!destination_.empty()) {
            result += stringutils::concat("destination='", destination_, "',");
        }
        if (!path_.empty()) {
            result += stringutils::concat("path='", path_, "',");
        }
        if (!interface_.empty()) {
            result += stringutils::concat("interface='", interface_, "',");
        }
        if (!name_.empty()) {
            result += stringutils::concat("member='", name_, "',");
        }
        for (size_t i = 0; i < argumentMatch_.size(); i++) {
            if (argumentMatch_[i] == MatchRule::nullArg) {
                continue;
            }
            result +=
                stringutils::concat("arg", i, "='", argumentMatch_[i], "',");
        }
        if (eavesdrop_) {
            result += "eavesdrop='true',";
        }
        // remove trailing comma.
        result.pop_back();
        return result;
    }

    bool check(Message &message, const std::string &alterName) const {

        if (!service_.empty() && service_ != message.sender() &&
            alterName != message.sender()) {
            return false;
        }
        if (!path_.empty() && path_ != message.path()) {
            return false;
        }
        if (!interface_.empty() && interface_ != message.interface()) {
            return false;
        }
        if (!name_.empty() && name_ != message.member()) {
            return false;
        }
        if (!argumentMatch_.empty()) {
            auto sig = message.signature();
            auto args = splitDBusSignature(sig);
            for (size_t i = 0; i < argumentMatch_.size(); i++) {
                if (argumentMatch_[i] == MatchRule::nullArg) {
                    if (i < args.size()) {
                        message.skip();
                    }
                    continue;
                }
                if (i >= args.size() || args[i] != "s") {
                    return false;
                }
                std::string arg;
                if (message >> arg) {
                    if (arg != argumentMatch_[i]) {
                        return false;
                    }
                } else {
                    return false;
                }
            }
        }
        return true;
    }
};

MatchRule::MatchRule(std::string service, std::string path,
                     std::string interface, std::string name,
                     std::vector<std::string> argumentMatch)
    : MatchRule(MessageType::Signal, service, "", std::move(path),
                std::move(interface), std::move(name), std::move(argumentMatch),
                false) {}

MatchRule::MatchRule(MessageType type, std::string service,
                     std::string destination, std::string path,
                     std::string interface, std::string name,
                     std::vector<std::string> argumentMatch, bool eavesdrop)
    : d_ptr(std::make_unique<MatchRulePrivate>(
          type, std::move(service), std::move(destination), std::move(path),
          std::move(interface), std::move(name), std::move(argumentMatch),
          eavesdrop)) {}

FCITX_DEFINE_DPTR_COPY_AND_DEFAULT_DTOR_AND_MOVE(MatchRule);

const std::string &MatchRule::service() const noexcept {
    FCITX_D();
    return d->service_;
}
const std::string &MatchRule::destination() const noexcept {
    FCITX_D();
    return d->destination_;
}

const std::string &MatchRule::path() const noexcept {
    FCITX_D();
    return d->path_;
}

const std::string &MatchRule::interface() const noexcept {
    FCITX_D();
    return d->interface_;
}

const std::string &MatchRule::name() const noexcept {
    FCITX_D();
    return d->name_;
}

const std::vector<std::string> &MatchRule::argumentMatch() const noexcept {
    FCITX_D();
    return d->argumentMatch_;
}

const std::string &MatchRule::rule() const noexcept {
    FCITX_D();
    return d->rule_;
}

bool MatchRule::eavesdrop() const noexcept {
    FCITX_D();
    return d->eavesdrop_;
}

bool MatchRule::check(Message &message, const std::string &alterName) const {
    FCITX_D();
    auto result = d->check(message, alterName);
    message.rewind();
    return result;
}

} // namespace fcitx::dbus
