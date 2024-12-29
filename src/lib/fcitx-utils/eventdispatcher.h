/*
 * SPDX-FileCopyrightText: 2015-2019 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#ifndef _FCITX_UTILS_EVENTDISPATCHER_H_
#define _FCITX_UTILS_EVENTDISPATCHER_H_

#include <functional>
#include <memory>
#include <utility>
#include <fcitx-utils/fcitxutils_export.h>
#include <fcitx-utils/macros.h>
#include <fcitx-utils/trackableobject.h>

namespace fcitx {

class EventLoop;
class EventDispatcherPrivate;
/**
 * A thread safe class to post event to a certain EventLoop.
 *
 */
class FCITXUTILS_EXPORT EventDispatcher {
public:
    /**
     * Construct a new event dispatcher. May throw exception if it fails to
     * create underlying file descriptor.
     */
    EventDispatcher();
    virtual ~EventDispatcher();

    /**
     * Attach EventDispatcher to an EventLoop. Must be called in the same thread
     * of EventLoop.
     *
     * @param event event loop to attach to.
     */
    void attach(EventLoop *event);

    /**
     * Detach event dispatcher from event loop, must be called from the same
     * thread from event loop.
     */
    void detach();

    /**
     * A thread-safe function to schedule a functor to be call from event loop.
     *
     * If functor is null, it will simply wake up event loop. Passing null can
     * be useful if you want to implement your own event loop and wake up event
     * loop.
     *
     * @param functor functor to be called.
     */
    void schedule(std::function<void()> functor);

    /**
     * A helper function that allows to only invoke certain function if the
     * reference is still valid.
     *
     * If context object is not valid when calling scheduleWithContext, it won't
     * be scheduled at all.
     *
     * @param context the context object.
     * @param functor function to be scheduled
     *
     * @since 5.1.8
     */
    template <typename T>
    void scheduleWithContext(TrackableObjectReference<T> context,
                             std::function<void()> functor) {
        if (!context.isValid()) {
            return;
        }

        schedule(
            [context = std::move(context), functor = std::move(functor)]() {
                if (context.isValid()) {
                    functor();
                }
            });
    }

    /**
     * Return the currently attached event loop
     *
     * @since 5.0.11
     */
    EventLoop *eventLoop() const;

private:
    const std::unique_ptr<EventDispatcherPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(EventDispatcher);
};

} // namespace fcitx
#endif // _FCITX_UTILS_EVENTDISPATCHER_H_
