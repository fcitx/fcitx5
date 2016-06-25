/*
* Copyright (C) 2015~2015 by CSSlayer
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

#include <cassert>
#include <fcitx-utils/event.h>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>

using namespace fcitx;

int main() {
    EventLoop e;

    int pipefd[2];
    int r = pipe(pipefd);
    assert(r == 0);

    std::unique_ptr<EventSource> source(
        e.addIOEvent(pipefd[0], IOEventFlag::In, [&e, pipefd](EventSource *, int fd, IOEventFlags flags) {
            assert(pipefd[0] == fd);
            if (flags & IOEventFlag::Hup) {
                e.quit();
            }

            if (flags & IOEventFlag::In) {
                char buf[20];
                auto size = read(fd, buf, 20);
                std::cout << "QUIT" << flags << std::endl;
                assert(size == 1);
                assert(buf[0] == 'a');
            }
            return true;
        }));

    std::unique_ptr<EventSource> source2(
        e.addTimeEvent(CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 1000000ul, 0, [&e, pipefd](EventSource *, uint64_t) {
            auto r = write(pipefd[1], "a", 1);
            assert(r == 1);
            return false;
        }));

    std::unique_ptr<EventSource> source3(
        e.addTimeEvent(CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 2000000ul, 0, [&e, pipefd](EventSource *, uint64_t) {
            close(pipefd[1]);
            return false;
        }));

    return e.exec() ? 0 : 1;
}