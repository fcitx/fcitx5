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
#include "fcitx-utils/standardpath.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addonmanager.h"
#include "fcitx/instance.h"
#include "errorhandler.h"
#include "keyboard.h"

using namespace fcitx;
int selfpipe[2];
std::string crashlog;

static KeyboardEngineFactory keyboardFactory;
StaticAddonRegistry staticAddon = {
    std::make_pair<std::string, AddonFactory *>("keyboard", &keyboardFactory)};

int main(int argc, char *argv[]) {
    if (pipe2(selfpipe, O_CLOEXEC | O_NONBLOCK) < 0) {
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

    try {
        Instance instance(argc, argv);
        instance.setSignalPipe(selfpipe[0]);
        instance.addonManager().registerDefaultLoader(&staticAddon);

        return instance.exec();
    } catch (const InstanceQuietQuit &) {
    } catch (const std::exception &e) {
        std::cerr << "Received exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
