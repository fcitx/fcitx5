/*
 * SPDX-FileCopyrightText: 2019-2019 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include <unistd.h>
#include <atomic>
#include <thread>
#include "fcitx-utils/event.h"
#include "fcitx-utils/eventdispatcher.h"
#include "fcitx-utils/log.h"

using namespace fcitx;

std::atomic<int> a = 0;

void scheduleEvent(EventDispatcher *dispatcher, EventLoop *loop) {
    for (int i = 0; i < 100; i++) {
        dispatcher->schedule([]() { a.fetch_add(1); });
    }
    while (a != 100) {
        usleep(1000);
    }
    dispatcher->schedule([loop, dispatcher]() {
        loop->exit();
        dispatcher->detach();
    });
}

void basicTest() {
    EventLoop loop;
    EventDispatcher dispatcher;
    dispatcher.attach(&loop);
    std::thread thread(scheduleEvent, &dispatcher, &loop);

    loop.exec();
    thread.join();
}

void recursiveSchedule() {
    EventDispatcher dispatcher;
    EventLoop loop;
    dispatcher.attach(&loop);
    int counter = 0;
    std::function<void()> callback = [&dispatcher, &counter, &loop,
                                      &callback]() {
        if (counter == 100) {
            loop.exit();
            return;
        }
        ++counter;
        dispatcher.schedule(callback);
    };

    dispatcher.schedule(callback);

    loop.exec();
    FCITX_ASSERT(counter == 100);
}

int main() {
    basicTest();
    recursiveSchedule();
    return 0;
}
