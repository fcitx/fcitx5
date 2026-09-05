/*
 * SPDX-FileCopyrightText: 2026 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include <memory>
#include <utility>
#include "fcitx-utils/coroutine.h"
#include "fcitx-utils/event.h"
#include "fcitx-utils/eventdispatcher.h"
#include "fcitx-utils/log.h"

using namespace fcitx;

namespace {

Coroutine<int> makeValue() {
    FCITX_INFO() << "makeValue";
    co_return 7;
}

Coroutine<unsigned int> makeAwaitedValue() {
    FCITX_INFO() << "makeAwaitedValue";
    int value = co_await makeValue();
    FCITX_INFO() << "makeAwaitedValue After";
    co_return value * 6;
}

Coroutine<void> makeVoid() { co_return; }

Coroutine<void> makeAwaitVoid() {
    FCITX_INFO() << "makeAwaitVoid";
    co_await makeVoid();
    FCITX_INFO() << "makeAwaitVoid After";
}

void testBasic() {
    auto value = makeValue();
    FCITX_ASSERT(!value.resume());
    FCITX_ASSERT(value.result() == 7);

    auto awaited = makeAwaitedValue();
    FCITX_ASSERT(!awaited.resume());
    FCITX_ASSERT(awaited.result() == 42);

    auto voidTask = makeVoid();
    FCITX_ASSERT(!voidTask.resume());
    voidTask.result();

    auto awaitVoidTask = makeAwaitVoid();
    FCITX_ASSERT(!awaitVoidTask.resume());
    awaitVoidTask.result();
}

void testDispatcher() {
    EventLoop eventLoop;
    EventDispatcher dispatcher;
    dispatcher.attach(&eventLoop);
    CoroutineTask task = [](EventLoop &eventLoop) -> Coroutine<void> {
        eventLoop.exit();
        co_return;
    }(eventLoop);
    sendToEventDispatcherDetached(&dispatcher, std::move(task));
    eventLoop.exec();
}

void testDispatcherTrackedTask() {
    EventLoop eventLoop;
    EventDispatcher dispatcher;
    dispatcher.attach(&eventLoop);

    bool finished = false;
    CoroutineTask task = [](EventLoop &eventLoop,
                            bool &finished) -> Coroutine<void> {
        eventLoop.exit();
        finished = true;
        co_return;
    }(eventLoop, finished);

    sendToEventDispatcher(&dispatcher, task);
    eventLoop.exec();

    FCITX_ASSERT(finished);
}

void testDispatcherTrackedTaskDestroyedBeforeRun() {
    EventLoop eventLoop;
    EventDispatcher dispatcher;
    dispatcher.attach(&eventLoop);

    bool finished = false;
    {
        CoroutineTask task = [](EventLoop &eventLoop,
                                bool &finished) -> Coroutine<void> {
            eventLoop.exit();
            finished = true;
            co_return;
        }(eventLoop, finished);

        sendToEventDispatcher(&dispatcher, task);
        // The task object is destroyed before the event loop runs.
    }

    dispatcher.detach();
    FCITX_ASSERT(!finished);
}

void testDispatcherNoLoopNoLeak() {
    std::weak_ptr<int> weak;

    {
        EventLoop eventLoop;
        EventDispatcher dispatcher;
        dispatcher.attach(&eventLoop);

        {
            auto pivot = std::make_shared<int>(0);
            weak = pivot;

            CoroutineTask task =
                [](std::shared_ptr<int> pivot) -> Coroutine<void> {
                FCITX_ASSERT(pivot);
                co_return;
            }(std::move(pivot));

            sendToEventDispatcherDetached(&dispatcher, std::move(task));
        }

        dispatcher.detach();

        // Intentionally do not run the event loop. Detaching the dispatcher
        // should release the queued task and its captures.
    }

    FCITX_ASSERT(!weak.lock());
}

void testDispatcherDetachedMoveOnlyTask() {
    EventLoop eventLoop;
    EventDispatcher dispatcher;
    dispatcher.attach(&eventLoop);

    bool finished = false;
    CoroutineTask task = [](EventLoop &eventLoop,
                            bool &finished) -> Coroutine<void> {
        eventLoop.exit();
        finished = true;
        co_return;
    }(eventLoop, finished);

    sendToEventDispatcherDetached(&dispatcher, std::move(task));
    eventLoop.exec();

    FCITX_ASSERT(finished);
}

} // namespace

int main() {
    testBasic();
    testDispatcher();
    testDispatcherTrackedTask();
    testDispatcherTrackedTaskDestroyedBeforeRun();
    testDispatcherDetachedMoveOnlyTask();
    testDispatcherNoLoopNoLeak();
    return 0;
}
