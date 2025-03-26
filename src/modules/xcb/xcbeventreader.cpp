/*
 * SPDX-FileCopyrightText: 2020-2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "xcbeventreader.h"
#include <xcb/xcb.h>
#include "fcitx-utils/event.h"
#include "xcbconnection.h"
#include "xcbmodule.h"

namespace fcitx {
XCBEventReader::XCBEventReader(XCBConnection *conn)
    : conn_(conn), dispatcherToMain_(conn->instance()->eventDispatcher()) {
    postEvent_ =
        conn->instance()->eventLoop().addPostEvent([this](EventSource *source) {
            if (xcb_connection_has_error(conn_->connection())) {
                source->setEnabled(false);
                return true;
            }
            FCITX_XCB_DEBUG() << "xcb_flush";
            xcb_flush(conn_->connection());
            wakeUp();
            return true;
        });
    thread_ = std::make_unique<std::thread>(&XCBEventReader::runThread, this);
}

XCBEventReader::~XCBEventReader() {
    if (thread_->joinable()) {
        dispatcherToWorker_.schedule([dispatcher = &dispatcherToWorker_]() {
            dispatcher->eventLoop()->exit();
        });
        thread_->join();
    }
}

auto nextXCBEvent(xcb_connection_t *conn, IOEventFlags flags) {
    if (flags.test(IOEventFlag::In)) {
        return makeUniqueCPtr(xcb_poll_for_event(conn));
    }
    return makeUniqueCPtr(xcb_poll_for_queued_event(conn));
}

bool XCBEventReader::onIOEvent(IOEventFlags flags) {
    if (hadError_) {
        return false;
    }
    if (int err = xcb_connection_has_error(conn_->connection())) {
        hadError_ = true;
        FCITX_WARN() << "XCB connection \"" << conn_->name()
                     << "\" got error: " << err;
        dispatcherToMain_.scheduleWithContext(watch(), [this]() {
            deferEvent_ =
                conn_->parent()->instance()->eventLoop().addDeferEvent(
                    [this](EventSource *) {
                        conn_->parent()->removeConnection(conn_->name());
                        return true;
                    });
        });
        return false;
    }

    bool hasEvent = false;
    std::list<UniqueCPtr<xcb_generic_event_t>> events;
    while (auto event = nextXCBEvent(conn_->connection(), flags)) {
        events.emplace_back(std::move(event));
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        events_.splice(events_.end(), events);
        hasEvent = !events_.empty();
    }
    if (hasEvent) {
        dispatcherToMain_.scheduleWithContext(
            watch(), [this]() { conn_->processEvent(); });
    }
    return true;
}

void XCBEventReader::wakeUp() {
    dispatcherToWorker_.schedule([this]() { onIOEvent(IOEventFlags{}); });
}

void XCBEventReader::run() {
    EventLoop event;
    dispatcherToWorker_.attach(&event);

    FCITX_XCB_DEBUG() << "Start XCBEventReader thread";

    int fd = xcb_get_file_descriptor(conn_->connection());
    auto ioEvent = event.addIOEvent(
        fd, IOEventFlag::In,
        [this, &event](EventSource *, int, IOEventFlags flags) {
            if (!onIOEvent(flags)) {
                event.exit();
            }
            return true;
        });
    event.exec();
    ioEvent.reset();
    dispatcherToWorker_.detach();

    FCITX_XCB_DEBUG() << "End XCBEventReader thread";
}

} // namespace fcitx
