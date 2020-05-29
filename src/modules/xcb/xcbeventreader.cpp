/*
 * SPDX-FileCopyrightText: 2020-2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "xcbeventreader.h"
#include "xcbconnection.h"
#include "xcbmodule.h"

namespace fcitx {
XCBEventReader::XCBEventReader(XCBConnection *conn) : conn_(conn) {
    dispatcherToMain_.attach(&conn->instance()->eventLoop());
    thread_ = std::make_unique<std::thread>(&XCBEventReader::runThread, this);
}

XCBEventReader::~XCBEventReader() {
    dispatcherToWorker_.schedule([this]() { event_->exit(); });
    thread_->join();
}

auto nextXCBEvent(xcb_connection_t *conn, IOEventFlags flags) {
    if (flags.test(IOEventFlag::In)) {
        return makeXCBReply(xcb_poll_for_event(conn));
    }
    return makeXCBReply(xcb_poll_for_queued_event(conn));
}

bool XCBEventReader::onIOEvent(IOEventFlags flags) {
    if (hadError_) {
        return false;
    }
    if (int err = xcb_connection_has_error(conn_->connection())) {
        FCITX_WARN() << "XCB connection \"" << conn_->name()
                     << "\" got error: " << err;
        dispatcherToMain_.schedule([this]() {
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
    std::list<XCBReply<xcb_generic_event_t>> events;
    while (auto event = nextXCBEvent(conn_->connection(), flags)) {
        events.emplace_back(std::move(event));
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        events_.splice(events_.end(), events);
        hasEvent = !events_.empty();
    }
    if (hasEvent) {
        dispatcherToMain_.schedule([this]() { conn_->processEvent(); });
    }
    return true;
}

void XCBEventReader::wakeUp() {
    dispatcherToWorker_.schedule([this]() { onIOEvent(IOEventFlags{}); });
}

void XCBEventReader::run() {
    event_ = std::make_unique<EventLoop>();
    dispatcherToWorker_.attach(event_.get());

    FCITX_XCB_DEBUG() << "Start XCBEventReader thread";

    int fd = xcb_get_file_descriptor(conn_->connection());
    auto ioEvent = event_->addIOEvent(
        fd, IOEventFlag::In, [this](EventSource *src, int, IOEventFlags flags) {
            if (!onIOEvent(flags)) {
                src->setEnabled(false);
            }
            return true;
        });
    event_->exec();
    ioEvent.reset();
    dispatcherToWorker_.detach();

    FCITX_XCB_DEBUG() << "End XCBEventReader thread";
    event_.reset();
}

} // namespace fcitx
