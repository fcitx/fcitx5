/*
 * SPDX-FileCopyrightText: 2019-2019 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include <unistd.h>
#include <atomic>
#include <thread>
#include <vector>
#include "fcitx-utils/event.h"
#include "fcitx-utils/eventdispatcher.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/trackableobject.h"

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

void testOrder() {
    EventLoop loop;
    EventDispatcher dispatcher;
    dispatcher.attach(&loop);
    std::vector<int> value;
    for (int i = 0; i < 100; i++) {
        dispatcher.schedule([i, &value]() { value.push_back(i); });
    }
    dispatcher.schedule([&loop]() { loop.exit(); });
    loop.exec();
    FCITX_ASSERT(value.size() == 100);
    for (int i = 0; i < 100; i++) {
        FCITX_ASSERT(i == value[i]) << i << " " << value[i];
    }
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

class TestObject : public TrackableObject<TestObject> {};

void withContext() {
    EventDispatcher dispatcher;
    EventLoop loop;
    dispatcher.attach(&loop);
    bool called = false;
    bool invalidCalled = false;
    TestObject validObject;
    {
        TestObject invalidObject;
        dispatcher.scheduleWithContext(validObject.watch(),
                                       [&called]() { called = true; });
        dispatcher.scheduleWithContext(
            invalidObject.watch(),
            [&invalidCalled]() { invalidCalled = true; });
    }

    dispatcher.schedule([&loop]() { loop.exit(); });
    loop.exec();

    FCITX_ASSERT(called);
    FCITX_ASSERT(!invalidCalled);
}

int main() {
    fcitx::Log::setLogRule("*=5");
    basicTest();
    testOrder();
    recursiveSchedule();
    withContext();
    return 0;
}
