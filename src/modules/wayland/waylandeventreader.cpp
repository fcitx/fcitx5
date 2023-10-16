/*
 * SPDX-FileCopyrightText: 2023-2023 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "waylandeventreader.h"
#include <mutex>
#include <wayland-client-core.h>
#include "fcitx-utils/event.h"
#include "waylandmodule.h"

namespace fcitx {

WaylandEventReader::WaylandEventReader(WaylandConnection *conn)
    : module_(conn->parent()), conn_(conn), display_(*conn_->display()) {
    dispatcherToMain_.attach(&conn->parent()->instance()->eventLoop());
    // Actively trigger an initial dispatch so we can make sure:
    // 1. depending even before this WaylandEventReader are handled.
    // 2. prepare_read is called.
    dispatcherToMain_.schedule([this]() { dispatch(); });
    thread_ =
        std::make_unique<std::thread>(&WaylandEventReader::runThread, this);
}
WaylandEventReader::~WaylandEventReader() {
    // Prevent that dispatcherToMain_ to deliver removeConnection.
    dispatcherToMain_.detach();
    if (thread_->joinable()) {
        quit();
        thread_->join();
    }
}

void WaylandEventReader::run() {
    EventLoop event;
    dispatcherToWorker_.attach(&event);
    int fd = wl_display_get_fd(display_);
    std::unique_ptr<EventSourceIO> ioEvent;
    ioEvent = event.addIOEvent(
        fd, IOEventFlag::In,
        [this, &ioEvent](EventSource *, int, IOEventFlags flags) {
            if (!onIOEvent(flags)) {
                ioEvent.reset();
            }
            return true;
        });

    event.exec();
    ioEvent.reset();
    dispatcherToWorker_.detach();
    {
        std::lock_guard lock(mutex_);
        if (isReading_) {
            wl_display_cancel_read(display_);
        }
    }
}

bool WaylandEventReader::onIOEvent(IOEventFlags flags) {
    {
        // Make sure previous dispatch ended.
        std::unique_lock lock(mutex_);
        condition_.wait(lock, [this] { return quitting_ || isReading_; });

        if (quitting_) {
            return false;
        }

        isReading_ = false;
    }

    // isReading_ is false at this point, we need to make sure either one of
    // wl_display_cancel_read and wl_display_read_events are called.
    if ((flags & IOEventFlag::Err) || (flags & IOEventFlag::Hup)) {
        wl_display_cancel_read(display_);
        quit();
        return false;
    }

    wl_display_read_events(display_);
    dispatcherToMain_.schedule([this]() { dispatch(); });
    return true;
}

void WaylandEventReader::quit() {
    {
        std::lock_guard lock(mutex_);
        quitting_ = true;
        condition_.notify_one();
    }
    // Allow quit to be called from both main thread and reader thread.
    dispatcherToWorker_.schedule([dispatcher = &dispatcherToWorker_]() {
        dispatcher->eventLoop()->exit();
    });

    // Make sure the connection will be removed.
    // The destructor will join the reader thread so it's ok.
    dispatcherToMain_.schedule([module = module_, name = conn_->name()]() {
        module->removeConnection(name);
    });
}

void WaylandEventReader::dispatch() {
    {
        std::lock_guard lk(mutex_);
        if (quitting_ || isReading_) {
            return;
        }
    }

    // Always try to dispatch pending and flush the connection.
    do {
        if (wl_display_dispatch_pending(display_) < 0) {
            auto err = wl_display_get_error(display_);
            FCITX_LOGC_IF(wayland_log, Error, err != 0)
                << "Wayland connection got error: " << err;
            quit();
            return;
        }
        wl_display_flush(display_);
    } while (wl_display_prepare_read(display_) != 0);

    // Make sure wl_display_prepare_read succeed at this point.
    {
        std::lock_guard lk(mutex_);
        isReading_ = true;
        condition_.notify_one();
    }
}

} // namespace fcitx