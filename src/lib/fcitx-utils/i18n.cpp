/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "i18n.h"
#include <cstddef>
#include <filesystem>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <libintl.h>
#include "fcitx-utils/fcitxutils_export.h"
#include "log.h"
#include "standardpaths.h"
#include "stringutils.h"

namespace fcitx {

struct string_hash {
    using is_transparent = void;
    [[nodiscard]] size_t operator()(const char *txt) const {
        return std::hash<std::string_view>{}(txt);
    }
    [[nodiscard]] size_t operator()(std::string_view txt) const {
        return std::hash<std::string_view>{}(txt);
    }
    [[nodiscard]] size_t operator()(const std::string &txt) const {
        return std::hash<std::string>{}(txt);
    }
};

class GettextManager {
public:
    void addDomain(const char *domain,
                   std::optional<std::filesystem::path> dir = std::nullopt) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (domains_.contains(domain)) {
            return;
        }
        std::filesystem::path path;
        if (dir) {
            path = std::move(*dir);
        } else {
            path = StandardPaths::fcitxPath("localedir");
        }
        bindtextdomain(domain, path.string().c_str());
        bind_textdomain_codeset(domain, "UTF-8");
        domains_.insert(domain);
        FCITX_DEBUG() << "Add gettext domain " << domain << " at " << dir;
    }

private:
    std::mutex mutex_;
    std::unordered_set<std::string, string_hash, std::equal_to<>> domains_;
};

static GettextManager gettextManager;

std::string translate(const std::string &s) { return translate(s.c_str()); }

const char *translate(const char *s) { return ::gettext(s); }

std::string translateCtx(const char *ctx, const std::string &s) {
    return translateCtx(ctx, s.c_str());
}

const char *translateCtx(const char *ctx, const char *s) {
    auto str = stringutils::concat(ctx, "\004", s);
    const auto *p = str.c_str();
    const auto *result = ::gettext(str.c_str());
    if (p == result) {
        return s;
    }
    return result;
}

std::string translateDomain(const char *domain, const std::string &s) {
    return translateDomain(domain, s.c_str());
}

const char *translateDomain(const char *domain, const char *s) {
    gettextManager.addDomain(domain);
    return ::dgettext(domain, s);
}

std::string translateDomainCtx(const char *domain, const char *ctx,
                               const std::string &s) {
    return translateDomainCtx(domain, ctx, s.c_str());
}

const char *translateDomainCtx(const char *domain, const char *ctx,
                               const char *s) {
    gettextManager.addDomain(domain);
    auto str = stringutils::concat(ctx, "\004", s);
    const auto *p = str.c_str();
    const auto *result = ::dgettext(domain, p);
    if (p == result) {
        return s;
    }
    return result;
}

FCITXUTILS_DEPRECATED_EXPORT void registerDomain(const char *domain,
                                                 const char *dir) {
    gettextManager.addDomain(
        domain,
        dir ? std::make_optional<std::filesystem::path>(dir) : std::nullopt);
}

void registerDomain(const char *domain, const std::filesystem::path &dir) {
    gettextManager.addDomain(domain, dir);
}

} // namespace fcitx
