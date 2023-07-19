/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include <fcntl.h>
#include <locale.h>
#include <unistd.h>
#include <exception>
#include <iostream>
#include <libintl.h>
#include "fcitx-utils/fs.h"
#include "fcitx-utils/misc.h"
#include "fcitx-utils/misc_p.h"
#include "fcitx-utils/standardpath.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addonmanager.h"
#include "fcitx/instance.h"
#include "errorhandler.h"

#ifdef ENABLE_KEYBOARD
#include "keyboard.h"
#endif

using namespace fcitx;
int selfpipe[2];
std::string crashlog;

#ifdef ENABLE_KEYBOARD
static KeyboardEngineFactory keyboardFactory;
#endif

StaticAddonRegistry staticAddon = {
#ifdef ENABLE_KEYBOARD
    std::make_pair<std::string, AddonFactory *>("keyboard", &keyboardFactory)
#endif
};

int main(int argc, char *argv[]) {
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
    try {
        FCITX_LOG_IF(Info, isInFlatpak()) << "Running inside flatpak.";
        Instance instance(argc, argv);
        instance.setSignalPipe(selfpipe[0]);
        instance.addonManager().registerDefaultLoader(&staticAddon);

        ret = instance.exec();
        restart = instance.isRestartRequested();
    } catch (const InstanceQuietQuit &) {
    } catch (const std::exception &e) {
        std::cerr << "Received exception: " << e.what() << std::endl;
        return 1;
    }

    if (restart) {
        auto fcitxBinary = StandardPath::fcitxPath("bindir", "fcitx5");
        if (isInFlatpak()) {
            startProcess({"flatpak-spawn", fcitxBinary, "-rd"});
        } else {
            startProcess({fcitxBinary, "-r"});
        }
    }
    return ret;
}
