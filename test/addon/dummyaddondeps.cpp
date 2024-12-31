#include "fcitx-utils/log.h"

FCITX_DEFINE_LOG_CATEGORY(dummyaddondeps, "dummyaddondeps");

static int init = []() {
    fcitx::Log::setLogRule("default=3");
    return 0;
}();
