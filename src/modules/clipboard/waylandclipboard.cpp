/*
 * SPDX-FileCopyrightText: 2021~2021 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "waylandclipboard.h"
#include <unordered_set>
#include "clipboard.h"
#include "wl_seat.h"
#include "zwlr_data_control_manager_v1.h"
#include "zwlr_data_control_offer_v1.h"

namespace fcitx {

uint64_t DataReaderThread::addTask(std::shared_ptr<UnixFD> fd,
                                   DataOfferCallback callback) {
    auto id = nextId_++;
    if (id == 0) {
        id = nextId_++;
    }
    FCITX_CLIPBOARD_DEBUG() << "Add task: " << id << " " << fd;
    dispatcherToWorker_.schedule([this, id, fd = std::move(fd),
                                  dispatcher = &dispatcherToWorker_,
                                  callback = std::move(callback)]() {
        auto &task = ((*tasks_)[id] = std::make_unique<DataOfferTask>());
        task->fd_ = fd;
        task->callback_ = std::move(callback);
        try {
            task->ioEvent_ = dispatcher->eventLoop()->addIOEvent(
                fd->fd(), {IOEventFlag::In, IOEventFlag::Err},
                [this, id, task = task.get()](EventSource *, int fd,
                                              IOEventFlags flags) {
                    if (flags.test(IOEventFlag::Err)) {
                        tasks_->erase(id);
                        return true;
                    }
                    char buf[4096];
                    auto n = fs::safeRead(fd, buf, sizeof(buf));
                    if (n == 0) {
                        dispatcherToMain_.schedule(
                            [data = std::move(task->data_),
                             callback = std::move(task->callback_)]() {
                                callback(data);
                            });
                        tasks_->erase(id);
                    } else if (n < 0) {
                        tasks_->erase(id);
                    } else {
                        if (task->data_.size() + n > MAX_CLIPBOARD_SIZE) {
                            tasks_->erase(id);
                            return true;
                        }
                        task->data_.insert(task->data_.end(), buf, buf + n);
                    }
                    return true;
                });
            FCITX_CLIPBOARD_DEBUG() << "Add watch to fd: " << fd->fd();
            // 1 sec timeout in case it takes forever.
            task->timeEvent_ = dispatcher->eventLoop()->addTimeEvent(
                CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 1000000, 0,
                [this, id](EventSource *, uint64_t) {
                    FCITX_CLIPBOARD_DEBUG() << "Reading data timeout.";
                    tasks_->erase(id);
                    return true;
                });
        } catch (const EventLoopException &) {
            // This may happen if fd is already closed.
            tasks_->erase(id);
        }
    });
    return id;
}

void DataReaderThread::removeTask(uint64_t token) {
    FCITX_CLIPBOARD_DEBUG() << "Remove task: " << token;
    dispatcherToWorker_.schedule([this, token]() { tasks_->erase(token); });
}

void DataReaderThread::realRun() {
    EventLoop loop;
    std::unordered_map<uint64_t, std::unique_ptr<DataOfferTask>> tasks;
    tasks_ = &tasks;
    dispatcherToWorker_.attach(&loop);
    loop.exec();
    FCITX_DEBUG() << "Ending DataReaderThread";
    tasks.clear();
    tasks_ = nullptr;
}

DataOffer::DataOffer(wayland::ZwlrDataControlOfferV1 *offer) : offer_(offer) {
    offer_->setUserData(this);
    conns_.emplace_back(offer_->offer().connect(
        [this](const char *offer) { mimeTypes_.insert(offer); }));
}

DataOffer::~DataOffer() {
    if (thread_) {
        thread_->removeTask(taskId_);
    }
}

void DataOffer::receiveData(DataReaderThread &thread,
                            DataOfferCallback callback) {
    if (thread_) {
        return;
    }
    std::string mime;
    static const std::string utf8Mime = "text/plain;charset=utf-8";
    static const std::string textMime = "text/plain";
    if (mimeTypes_.count(utf8Mime)) {
        mime = utf8Mime;
    } else if (mimeTypes_.count(textMime)) {
        mime = textMime;
    } else {
        return;
    }

    // Create a pipe for sending data.
    int pipeFds[2];
    if (safePipe(pipeFds) != 0) {
        return;
    }

    offer_->receive(mime.data(), pipeFds[1]);
    close(pipeFds[1]);

    thread_ = &thread;
    taskId_ = thread_->addTask(
        std::make_shared<UnixFD>(UnixFD::own(pipeFds[0])), std::move(callback));
}

DataDevice::DataDevice(WaylandClipboard *clipboard,
                       wayland::ZwlrDataControlDeviceV1 *device)
    : clipboard_(clipboard), device_(device), thread_(clipboard_->eventLoop()) {
    conns_.emplace_back(device_->dataOffer().connect(
        [](wayland::ZwlrDataControlOfferV1 *offer) { new DataOffer(offer); }));
    conns_.emplace_back(device_->selection().connect(
        [this](wayland::ZwlrDataControlOfferV1 *offer) {
            clipboardOffer_.reset(
                offer ? static_cast<DataOffer *>(offer->userData()) : nullptr);
            if (!clipboardOffer_) {
                return;
            }
            clipboardOffer_->receiveData(
                thread_, [this](std::vector<char> data) {
                    data.push_back('\0');
                    clipboard_->setClipboard(data.data());
                    clipboardOffer_.reset();
                });
        }));
    conns_.emplace_back(device_->primarySelection().connect(
        [this](wayland::ZwlrDataControlOfferV1 *offer) {
            primaryOffer_.reset(
                offer ? static_cast<DataOffer *>(offer->userData()) : nullptr);
            if (!primaryOffer_) {
                clipboard_->setPrimary("");
                return;
            }
            primaryOffer_->receiveData(thread_, [this](std::vector<char> data) {
                data.push_back('\0');
                clipboard_->setPrimary(data.data());
                primaryOffer_.reset();
            });
        }));
    conns_.emplace_back(device_->finished().connect([this]() {
        conns_.clear();
        primaryOffer_.reset();
        clipboardOffer_.reset();
        device_.reset();
    }));
    thread_.start();
}

WaylandClipboard::WaylandClipboard(Clipboard *clipboard,
                                   const std::string &name, wl_display *display)
    : parent_(clipboard), name_(name), display_(static_cast<wayland::Display *>(
                                           wl_display_get_user_data(display))) {
    display_->requestGlobals<wayland::ZwlrDataControlManagerV1>();
    globalConn_ = display_->globalCreated().connect(
        [this](const std::string &interface, std::shared_ptr<void> ptr) {
            if (interface == wayland::ZwlrDataControlManagerV1::interface) {
                if (ptr != manager_) {
                    deviceMap_.clear();
                    manager_ =
                        display_
                            ->getGlobal<wayland::ZwlrDataControlManagerV1>();
                }
                refreshSeat();
            } else if (interface == wayland::WlSeat::interface) {
                refreshSeat();
            }
        });
    globalRemoveConn_ = display_->globalRemoved().connect(
        [this](const std::string &interface, std::shared_ptr<void> ptr) {
            if (interface == wayland::ZwlrDataControlManagerV1::interface) {
                deviceMap_.clear();
                if (manager_ == ptr) {
                    manager_.reset();
                }
            } else if (interface == wayland::WlSeat::interface) {
                deviceMap_.erase(static_cast<wayland::WlSeat *>(ptr.get()));
            }
        });

    if (auto manager =
            display_->getGlobal<wayland::ZwlrDataControlManagerV1>()) {
        manager_ = std::move(manager);
    }
    refreshSeat();
}

void WaylandClipboard::refreshSeat() {
    if (!manager_) {
        return;
    }

    auto seats = display_->getGlobals<wayland::WlSeat>();
    for (const auto &seat : seats) {
        if (deviceMap_.count(seat.get())) {
            continue;
        }

        auto device = manager_->getDataDevice(seat.get());
        deviceMap_.emplace(seat.get(),
                           std::make_unique<DataDevice>(this, device));
    }
}

EventLoop *WaylandClipboard::eventLoop() {
    return &parent_->instance()->eventLoop();
}

void WaylandClipboard::setClipboard(const std::string &str) {
    parent_->setClipboard(name_, str);
}

void WaylandClipboard::setPrimary(const std::string &str) {
    parent_->setPrimary(name_, str);
}

} // namespace fcitx
