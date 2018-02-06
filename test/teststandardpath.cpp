//
// Copyright (C) 2016~2016 by CSSlayer
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

#include "fcitx-utils/fs.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/standardpath.h"
#include "testdir.h"
#include <fcntl.h>
#include <set>
#include <unistd.h>

using namespace fcitx;

#define TEST_ADDON_DIR FCITX5_SOURCE_DIR "/test/addon"

int main() {
    FCITX_ASSERT(setenv("XDG_CONFIG_HOME", "/TEST/PATH", 1) == 0);
    FCITX_ASSERT(setenv("XDG_CONFIG_DIRS", "/TEST/PATH1:/TEST/PATH2", 1) == 0);
    FCITX_ASSERT(setenv("XDG_DATA_DIRS", TEST_ADDON_DIR, 1) == 0);
    StandardPath standardPath(true);

    FCITX_ASSERT(standardPath.userDirectory(StandardPath::Type::Config) ==
                 "/TEST/PATH");
    FCITX_ASSERT(standardPath.directories(StandardPath::Type::Config) ==
                 std::vector<std::string>({"/TEST/PATH1", "/TEST/PATH2"}));

    {
        auto result = standardPath.multiOpen(
            StandardPath::Type::PkgData, "addon", O_RDONLY,
            filter::Not(filter::User()), filter::Suffix(".conf"));
        std::set<std::string> names,
            expect_names = {"testim.conf", "testfrontend.conf"};
        for (auto &p : result) {
            names.insert(p.first);
            FCITX_ASSERT(p.second.fd() >= 0);
        }

        FCITX_ASSERT(names == expect_names);
    }

    {
        auto result = standardPath.multiOpen(
            StandardPath::Type::PkgData, "addon", O_RDONLY,
            filter::Not(filter::User()), filter::Suffix("im.conf"));
        std::set<std::string> names, expect_names = {"testim.conf"};
        for (auto &p : result) {
            names.insert(p.first);
            FCITX_ASSERT(p.second.fd() >= 0);
        }

        FCITX_ASSERT(names == expect_names);
    }

    auto file = standardPath.open(StandardPath::Type::PkgData,
                                  "addon/testim.conf", O_RDONLY);
    FCITX_ASSERT(file.path() ==
                 fs::cleanPath(TEST_ADDON_DIR "/fcitx5/addon/testim.conf"));

    auto file2 = standardPath.open(StandardPath::Type::Data,
                                   "fcitx5/addon/testim2.conf", O_RDONLY);
    FCITX_ASSERT(file2.fd() == -1);

    return 0;
}
