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

#include "fcitx-utils/log.h"
#include <fcitx-utils/event.h>
#include <fcntl.h>
#include <unistd.h>

using namespace fcitx;

int main() {
    EventLoop e;

    int pipefd[2];
    int r = pipe(pipefd);
    FCITX_ASSERT(r == 0);

    std::unique_ptr<EventSource> source(
        e.addIOEvent(pipefd[0], IOEventFlag::In,
                     [&e, pipefd](EventSource *, int fd, IOEventFlags flags) {
                         FCITX_ASSERT(pipefd[0] == fd);
                         if (flags & IOEventFlag::Hup) {
                             e.quit();
                         }

                         if (flags & IOEventFlag::In) {
                             char buf[20];
                             auto size = read(fd, buf, 20);
                             if (size == 0) {
                                 e.quit();
                             } else {
                                 FCITX_INFO() << "QUIT" << flags;
                                 FCITX_ASSERT(size == 1);
                                 FCITX_ASSERT(buf[0] == 'a');
                             }
                         }
                         return true;
                     }));

    std::unique_ptr<EventSource> source4(
        e.addDeferEvent([&e, pipefd](EventSource *) {
            FCITX_INFO() << "DEFER";
            return false;
        }));

    std::unique_ptr<EventSource> source5(
        e.addExitEvent([&e, pipefd](EventSource *) {
            FCITX_INFO() << "EXIT";
            return false;
        }));

    int times = 10;
    std::unique_ptr<EventSource> sourceX(
        e.addTimeEvent(CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 1000000ul, 0,
                       [&e, &times, pipefd](EventSource *source, uint64_t) {
                           FCITX_INFO() << "Recur:" << times;
                           times--;
                           if (times < 0) {
                               source->setEnabled(false);
                           }
                           return false;
                       }));
    sourceX->setEnabled(true);

    int times2 = 10;
    std::unique_ptr<EventSource> sourceX2(e.addTimeEvent(
        CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 1000000ul, 0,
        [&e, &sourceX2, &times2, pipefd](EventSourceTime *source, uint64_t t) {
            FCITX_INFO() << "Recur 2:" << times2 << " " << t;
            times2--;
            if (times2 > 0) {
                source->setNextInterval(100000);
                source->setOneShot();
            }
            return false;
        }));

    std::unique_ptr<EventSource> source2(
        e.addTimeEvent(CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 1000000ul, 0,
                       [&e, pipefd](EventSource *, uint64_t) {
                           FCITX_INFO() << "WRITE";
                           auto r = write(pipefd[1], "a", 1);
                           FCITX_ASSERT(r == 1);
                           return false;
                       }));

    std::unique_ptr<EventSource> source3(
        e.addTimeEvent(CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 2000000ul, 0,
                       [&e, pipefd](EventSource *, uint64_t) {
                           FCITX_INFO() << "CLOSE";
                           close(pipefd[1]);
                           return false;
                       }));

    return e.exec() ? 0 : 1;
}
