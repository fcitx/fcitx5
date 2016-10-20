/*
 * Copyright (C) 2016~2016 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
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

#include "fcitx-utils/fs.h"
#include "fcitx-utils/standardpath.h"
#include <cassert>
#include <fcntl.h>
#include <set>
#include <unistd.h>

using namespace fcitx;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        return 1;
    }

    assert(setenv("XDG_CONFIG_HOME", "/TEST/PATH", 1) == 0);
    assert(setenv("XDG_CONFIG_DIRS", "/TEST/PATH1:/TEST/PATH2", 1) == 0);
    assert(setenv("XDG_DATA_DIRS", argv[1], 1) == 0);
    StandardPath standardPath(true);

    assert(standardPath.userDirectory(StandardPath::Type::Config) == "/TEST/PATH");
    assert(standardPath.directories(StandardPath::Type::Config) ==
           std::vector<std::string>({"/TEST/PATH1", "/TEST/PATH2"}));

    {
        auto result = standardPath.multiOpen(StandardPath::Type::Data, "fcitx5/addon", O_RDONLY,
                                             filter::Not(filter::User()), filter::Suffix(".conf"));
        std::set<std::string> names, expect_names = {"testim.conf", "testfrontend.conf"};
        for (auto &p : result) {
            names.insert(p.first);
            assert(p.second.fd() >= 0);
        }

        assert(names == expect_names);
    }

    {
        auto result = standardPath.multiOpen(StandardPath::Type::Data, "fcitx5/addon", O_RDONLY,
                                             filter::Not(filter::User()), filter::Suffix("im.conf"));
        std::set<std::string> names, expect_names = {"testim.conf"};
        for (auto &p : result) {
            names.insert(p.first);
            assert(p.second.fd() >= 0);
        }

        assert(names == expect_names);
    }

    auto file = standardPath.open(StandardPath::Type::Data, "fcitx5/addon/testim.conf", O_RDONLY);
    assert(file.path() == fs::cleanPath(std::string(argv[1]) + "/" + "fcitx5/addon/testim.conf"));

    auto file2 = standardPath.open(StandardPath::Type::Data, "fcitx5/addon/testim2.conf", O_RDONLY);
    assert(file2.fd() == -1);

    return 0;
}
