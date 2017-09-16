
#include "i18n.h"
#include "log.h"
#include "fcitxutils_export.h"
#include "standardpath.h"
#include <libintl.h>
#include <mutex>
#include <string>
#include <unordered_set>

namespace fcitx {

class GettextManager {
public:
    void addDomain(const char *domain, const char* dir = nullptr) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (domains_.count(domain)) {
            return;
        }
        auto localedir = StandardPath::fcitxPath("localedir");
        if (!dir) {
            dir = localedir.data();
        }
        bindtextdomain(domain, dir);
        bind_textdomain_codeset(domain, "UTF-8");
        domains_.insert(domain);
        FCITX_LOG(Debug) << "Add gettext domain " << domain << " at " << dir;
    }

private:
    std::mutex mutex_;
    std::unordered_set<std::string> domains_;
};

static GettextManager gettextManager;

FCITXUTILS_EXPORT const char *translate(const std::string &s) {
    return ::gettext(s.c_str());
}

FCITXUTILS_EXPORT const char *translate(const char *s) { return ::gettext(s); }

FCITXUTILS_EXPORT const char *translateDomain(const char *domain,
                                              const std::string &s) {
    return translateDomain(domain, s.c_str());
}

FCITXUTILS_EXPORT const char *translateDomain(const char *domain,
                                              const char *s) {
    gettextManager.addDomain(domain);
    return ::dgettext(domain, s);
}
FCITXUTILS_EXPORT void registerDomain(const char *domain,
                                      const char *dir) {
    gettextManager.addDomain(domain, dir);
}
}
