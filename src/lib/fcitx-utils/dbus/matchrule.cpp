//
// Copyright (C) 2017~2017 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//

#include "matchrule.h"
#include "../stringutils.h"
#include "message.h"
#include "utils_p.h"

namespace fcitx {
namespace dbus {

static const char nullArray[] = {'\0', '\0'};

const std::string MatchRule::nullArg{nullArray, nullArray + 1};

class MatchRulePrivate {
public:
    MatchRulePrivate(std::string service, std::string path,
                     std::string interface, std::string name,
                     std::vector<std::string> argumentMatch)
        : service_(std::move(service)), path_(std::move(path)),
          interface_(std::move(interface)), name_(std::move(name)),
          argumentMatch_(std::move(argumentMatch)), rule_(buildRule()) {}

    FCITX_INLINE_DEFINE_DEFAULT_DTOR_COPY_AND_MOVE_WITHOUT_SPEC(
        MatchRulePrivate)

    std::string service_;
    std::string path_;
    std::string interface_;
    std::string name_;
    std::vector<std::string> argumentMatch_;
    std::string rule_;

    std::string buildRule() const {
        std::string result = "type='signal',";
        if (!service_.empty()) {
            result += stringutils::concat("sender='", service_, "',");
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
    : d_ptr(std::make_unique<MatchRulePrivate>(
          std::move(service), std::move(path), std::move(interface),
          std::move(name), std::move(argumentMatch))) {}

FCITX_DEFINE_DPTR_COPY_AND_DEFAULT_DTOR_AND_MOVE(MatchRule);

const std::string &MatchRule::service() const noexcept {
    FCITX_D();
    return d->service_;
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

bool MatchRule::check(const Message &_message,
                      const std::string &alterName) const {
    FCITX_D();
    Message message = _message;
    auto result = d->check(message, alterName);
    message.rewind();
    return result;
}

} // namespace dbus
} // namespace fcitx
