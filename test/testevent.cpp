/*
 * SPDX-FileCopyrightText: 2015-2015 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include <fcntl.h>
#include <unistd.h>
#include <fcitx-utils/event.h>
#include "fcitx-utils/log.h"

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
                             e.exit();
                         }

                         if (flags & IOEventFlag::In) {
                             char buf[20];
                             auto size = read(fd, buf, 20);
                             if (size == 0) {
                                 e.exit();
                             } else {
                                 FCITX_INFO() << "QUIT" << flags;
                                 FCITX_ASSERT(size == 1);
                                 FCITX_ASSERT(buf[0] == 'a');
                             }
                         }
                         return true;
                     }));

    std::unique_ptr<EventSource> source4(e.addDeferEvent([](EventSource *) {
        FCITX_INFO() << "DEFER";
        return false;
    }));

    std::unique_ptr<EventSource> source5(e.addExitEvent([](EventSource *) {
        FCITX_INFO() << "EXIT";
        return false;
    }));

    int times = 10;
    std::unique_ptr<EventSource> sourceX(
        e.addTimeEvent(CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 1000000ul, 0,
                       [&times](EventSource *source, uint64_t) {
                           FCITX_INFO() << "Recur:" << times;
                           times--;
                           if (times < 0) {
                               source->setEnabled(false);
                           }
                           return false;
                       }));
    sourceX->setEnabled(true);

    int times2 = 10;
    std::unique_ptr<EventSource> sourceX2(
        e.addTimeEvent(CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 1000000ul, 0,
                       [&times2](EventSourceTime *source, uint64_t t) {
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
                       [pipefd](EventSource *, uint64_t) {
                           FCITX_INFO() << "WRITE";
                           auto r = write(pipefd[1], "a", 1);
                           FCITX_ASSERT(r == 1);
                           return false;
                       }));

    std::unique_ptr<EventSource> source3(
        e.addTimeEvent(CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 2000000ul, 0,
                       [pipefd](EventSource *, uint64_t) {
                           FCITX_INFO() << "CLOSE";
                           close(pipefd[1]);
                           return false;
                       }));

    return e.exec() ? 0 : 1;
}
