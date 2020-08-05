/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "i18n.h"
#include <mutex>
#include <string>
#include <unordered_set>
#include <libintl.h>
#include "fcitxutils_export.h"
#include "log.h"
#include "standardpath.h"

namespace fcitx {

class GettextManager {
public:
    void addDomain(const char *domain, const char *dir = nullptr) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (domains_.count(domain)) {
            return;
        }
        const auto *localedir = StandardPath::fcitxPath("localedir");
        if (!dir) {
            dir = localedir;
        }
        bindtextdomain(domain, dir);
        bind_textdomain_codeset(domain, "UTF-8");
        domains_.insert(domain);
        FCITX_DEBUG() << "Add gettext domain " << domain << " at " << dir;
    }

private:
    std::mutex mutex_;
    std::unordered_set<std::string> domains_;
};

static GettextManager gettextManager;

FCITXUTILS_EXPORT std::string translate(const std::string &s) {
    return translate(s.c_str());
}

FCITXUTILS_EXPORT const char *translate(const char *s) { return ::gettext(s); }

FCITXUTILS_EXPORT std::string translateCtx(const char *ctx,
                                           const std::string &s) {
    return translateCtx(ctx, s.c_str());
}

FCITXUTILS_EXPORT const char *translateCtx(const char *ctx, const char *s) {
    auto str = stringutils::concat(ctx, "\004", s);
    const auto *p = str.c_str();
    const auto *result = ::gettext(str.c_str());
    if (p == result) {
        return s;
    }
    return result;
}

FCITXUTILS_EXPORT std::string translateDomain(const char *domain,
                                              const std::string &s) {
    return translateDomain(domain, s.c_str());
}

FCITXUTILS_EXPORT const char *translateDomain(const char *domain,
                                              const char *s) {
    gettextManager.addDomain(domain);
    return ::dgettext(domain, s);
}

FCITXUTILS_EXPORT std::string
translateDomainCtx(const char *domain, const char *ctx, const std::string &s) {
    return translateDomainCtx(domain, ctx, s.c_str());
}

FCITXUTILS_EXPORT const char *
translateDomainCtx(const char *domain, const char *ctx, const char *s) {
    gettextManager.addDomain(domain);
    auto str = stringutils::concat(ctx, "\004", s);
    const auto *p = str.c_str();
    const auto *result = ::dgettext(domain, p);
    if (p == result) {
        return s;
    }
    return result;
}

FCITXUTILS_EXPORT void registerDomain(const char *domain, const char *dir) {
    gettextManager.addDomain(domain, dir);
}
} // namespace fcitx
