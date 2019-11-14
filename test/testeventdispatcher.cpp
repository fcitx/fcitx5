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
#include <thread>
#include <unistd.h>

using namespace fcitx;

volatile int a = 0;

void scheduleEvent(EventDispatcher *dispatcher, EventLoop *loop) {
    for (int i = 0; i < 100; i++) {
        dispatcher->schedule([]() { a += 1; });
    }
    while (a != 100) {
        usleep(1000);
    }
    dispatcher->schedule([loop, dispatcher]() {
        loop->quit();
        dispatcher->detach();
    });
}

int main() {
    EventLoop loop;
    EventDispatcher dispatcher;
    dispatcher.attach(&loop);
    std::thread thread(scheduleEvent, &dispatcher, &loop);

    loop.exec();
    thread.join();

    return 0;
}
