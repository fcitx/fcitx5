//
// Copyright (C) 2019~2019 by CSSlayer
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
#include "fcitx-utils/event.h"
#include "fcitx-utils/eventdispatcher.h"
#include "fcitx-utils/log.h"
#include <atomic>
#include <thread>
#include <unistd.h>

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
    EventLoop loop;
    EventDispatcher dispatcher;
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
