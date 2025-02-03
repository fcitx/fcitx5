/*
 * SPDX-FileCopyrightText: 2024-2024 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "eventlooptests.h"
#include <unistd.h>
#include <cstdint>
#include <ctime>
#include <memory>
#include <thread>
#include "fcitx-utils/event.h"
#include "fcitx-utils/eventdispatcher.h"
#include "fcitx-utils/eventloopinterface.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/misc_p.h"

using namespace fcitx;

void test_basic() {
    FCITX_INFO() << __func__;
    EventLoop e;

    int pipefd[2];
    int r = safePipe(pipefd);
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
        return true;
    }));

    std::unique_ptr<EventSource> source5(e.addExitEvent([](EventSource *) {
        FCITX_INFO() << "EXIT";
        return true;
    }));

    int times = 10;
    std::unique_ptr<EventSource> sourceX(
        e.addTimeEvent(CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 1000000UL, 1,
                       [&times](EventSource *source, uint64_t) {
                           FCITX_INFO() << "Recur:" << times;
                           times--;
                           if (times < 0) {
                               source->setEnabled(false);
                           }
                           return true;
                       }));
    sourceX->setEnabled(true);

    int times2 = 10;
    std::unique_ptr<EventSource> sourceX2(
        e.addTimeEvent(CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 1000000UL, 1,
                       [&times2](EventSourceTime *source, uint64_t t) {
                           FCITX_INFO() << "Recur 2:" << times2 << " " << t;
                           times2--;
                           if (times2 > 0) {
                               source->setNextInterval(100000);
                               source->setOneShot();
                           }
                           return true;
                       }));

    std::unique_ptr<EventSource> source2(
        e.addTimeEvent(CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 1000000UL, 0,
                       [pipefd](EventSource *, uint64_t) {
                           FCITX_INFO() << "WRITE";
                           auto r = write(pipefd[1], "a", 1);
                           FCITX_ASSERT(r == 1);
                           return true;
                       }));

    std::unique_ptr<EventSource> source3(
        e.addTimeEvent(CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 2000000UL, 0,
                       [pipefd](EventSource *, uint64_t) {
                           FCITX_INFO() << "CLOSE";
                           close(pipefd[1]);
                           return true;
                       }));

    e.exec();
}

void test_source_deleted() {
    FCITX_INFO() << __func__;
    EventLoop e;

    std::unique_ptr<EventSourceAsync> source(
        e.addAsyncEvent([&source](EventSource *) {
            FCITX_INFO() << "RESET";
            source.reset();
            return true;
        }));

    std::unique_ptr<EventSource> source2(
        e.addDeferEvent([&source](EventSource *) {
            FCITX_INFO() << "WRITE";
            source->send();
            return true;
        }));

    std::unique_ptr<EventSource> source3(
        e.addTimeEvent(CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 2000000UL, 0,
                       [&e](EventSource *, uint64_t) {
                           FCITX_INFO() << "EXIT";
                           e.exit();
                           return true;
                       }));

    e.exec();
}

void test_post_time() {
    FCITX_INFO() << __func__;
    EventLoop e;
    bool ready = false;
    auto post = e.addPostEvent([&ready, &e](EventSource *) {
        FCITX_INFO() << "POST TIME";
        if (ready) {
            e.exit();
        }
        return true;
    });
    auto time = e.addTimeEvent(CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 1000000,
                               0, [&ready](EventSource *, uint64_t) {
                                   FCITX_INFO() << "TIME";
                                   ready = true;
                                   return true;
                               });
    e.exec();
}

void test_post_io() {
    FCITX_INFO() << __func__;
    EventLoop e;
    EventDispatcher dispatcher;
    bool ready = false;
    auto post = e.addPostEvent([&ready, &e](EventSource *) {
        FCITX_INFO() << "POST IO";
        if (ready) {
            e.exit();
        }
        return true;
    });
    dispatcher.attach(&e);
    std::thread thread([&dispatcher, &ready]() {
        sleep(2);
        dispatcher.schedule([&ready]() {
            FCITX_INFO() << "DISPATCHER";
            ready = true;
        });
    });
    e.exec();
    thread.join();
}

void runAllEventLoopTests() {
    test_basic();
    test_source_deleted();
    test_post_time();
    test_post_io();
}
