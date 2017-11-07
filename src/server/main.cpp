/*
 * Copyright (C) 2016~2016 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; see the file COPYING. If not,
 * see <http://www.gnu.org/licenses/>.
 */

#include "errorhandler.h"
#include "fcitx-utils/fs.h"
#include "fcitx-utils/standardpath.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addonmanager.h"
#include "fcitx/instance.h"
#include "keyboard.h"
#include <exception>
#include <fcntl.h>
#include <iostream>
#include <libintl.h>
#include <locale.h>
#include <unistd.h>

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

    auto home = getenv("HOME");
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
