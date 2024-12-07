/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include <locale.h>
#include <sys/stat.h>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include "fcitx-utils/fs.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/misc.h"
#include "fcitx-utils/misc_p.h"
#include "fcitx-utils/standardpath.h"
#include "fcitx-utils/stringutils.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addoninstance.h"
#include "fcitx/addonloader.h"
#include "fcitx/addonmanager.h"
#include "fcitx/instance.h"
#include "errorhandler.h"

using namespace fcitx;
int selfpipe[2];
std::string crashlog;

FCITX_DEFINE_STATIC_ADDON_REGISTRY(getStaticAddon)
#ifdef ENABLE_KEYBOARD
FCITX_IMPORT_ADDON_FACTORY(getStaticAddon, keyboard);
#endif

int main(int argc, char *argv[]) {
    umask(077);
    StandardPath::global().syncUmask();
    if (safePipe(selfpipe) < 0) {
        fprintf(stderr, "Could not create self-pipe.\n");
        return 1;
    }

    auto *home = getenv("HOME");
    if (!home || home[0] == '\0') {
        fprintf(stderr, "Please set HOME.\n");
        return 1;
    }

    auto userDir =
        StandardPath::global().userDirectory(StandardPath::Type::PkgConfig);
    if (!userDir.empty()) {
        if (fs::makePath(userDir)) {
            crashlog = stringutils::joinPath(userDir, "crash.log");
        }
    }

    SetMyExceptionHandler();

    setlocale(LC_ALL, "");

    // Log::setLogRule("wayland=5");
    int ret = 0;
    bool restart = false;
    bool canRestart = false;
    try {
        FCITX_LOG_IF(Info, isInFlatpak()) << "Running inside flatpak.";
        Instance instance(argc, argv);
        instance.setBinaryMode();
        instance.setSignalPipe(selfpipe[0]);
        instance.addonManager().registerDefaultLoader(&getStaticAddon());

        ret = instance.exec();
        restart = instance.isRestartRequested();
        canRestart = instance.canRestart();
    } catch (const InstanceQuietQuit &) {
    } catch (const std::exception &e) {
        std::cerr << "Received exception: " << e.what() << '\n';
        return 1;
    }

    if (restart && canRestart) {
        std::vector<std::string> args;
        if (isInFlatpak()) {
            args = {"flatpak-spawn",
                    StandardPath::fcitxPath("bindir", "fcitx5"), "-rd"};
        } else {
            args = {StandardPath::fcitxPath("bindir", "fcitx5"), "-r"};
        }
        startProcess(args);
    }
    return ret;
}
