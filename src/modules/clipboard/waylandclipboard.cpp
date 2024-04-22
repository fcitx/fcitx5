/*
 * SPDX-FileCopyrightText: 2021~2021 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "waylandclipboard.h"
#include <unordered_set>
#include "fcitx-utils/event.h"
#include "fcitx-utils/trackableobject.h"
#include "clipboard.h"
#include "wl_seat.h"
#include "zwlr_data_control_manager_v1.h"
#include "zwlr_data_control_offer_v1.h"

namespace fcitx {

uint64_t DataReaderThread::addTask(DataOffer *offer, std::shared_ptr<UnixFD> fd,
                                   DataOfferDataCallback callback) {
    auto id = nextId_++;
    if (id == 0) {
        id = nextId_++;
    }
    FCITX_CLIPBOARD_DEBUG() << "Add task: " << id << " " << fd;
    dispatcherToWorker_.scheduleWithContext(
        offer->watch(),
        [this, id, fd = std::move(fd), offerRef = offer->watch(),
         callback = std::move(callback)]() mutable {
            addTaskOnWorker(id, std::move(offerRef), std::move(fd),
                            std::move(callback));
        });
    return id;
}

void DataReaderThread::removeTask(uint64_t token) {
    FCITX_CLIPBOARD_DEBUG() << "Remove task: " << token;
    dispatcherToWorker_.schedule([this, token]() { tasks_.erase(token); });
}

void DataReaderThread::realRun() {
    EventLoop loop;
    dispatcherToWorker_.attach(&loop);
    loop.exec();
    dispatcherToWorker_.detach();
    FCITX_DEBUG() << "Ending DataReaderThread";
    tasks_.clear();
}

void DataReaderThread::addTaskOnWorker(
    uint64_t id, TrackableObjectReference<DataOffer> offer,
    std::shared_ptr<UnixFD> fd, DataOfferDataCallback callback) {
    // std::unordered_map's ref/pointer to element is stable.
    auto &task = tasks_[id];
    task.id_ = id;
    task.offer_ = std::move(offer);
    task.fd_ = std::move(fd);
    task.callback_ = std::move(callback);
    try {
        task.ioEvent_ = dispatcherToWorker_.eventLoop()->addIOEvent(
            task.fd_->fd(), {IOEventFlag::In, IOEventFlag::Err},
            [this, taskPtr = &task](EventSource *, int, IOEventFlags flags) {
                handleTaskIO(taskPtr, flags);
                return true;
            });
        FCITX_CLIPBOARD_DEBUG() << "Add watch to fd: " << task.fd_->fd();
        // 1 sec timeout in case it takes forever.
        task.timeEvent_ = dispatcherToWorker_.eventLoop()->addTimeEvent(
            CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 1000000, 0,
            [this, taskPtr = &task](EventSource *, uint64_t) {
                handleTaskTimeout(taskPtr);
                return true;
            });
    } catch (const EventLoopException &) {
        // This may happen if fd is already closed.
        tasks_.erase(id);
    }
}

void DataReaderThread::handleTaskIO(DataOfferTask *task, IOEventFlags flags) {
    if (flags.test(IOEventFlag::Err) || !task->offer_.isValid()) {
        tasks_.erase(task->id_);
        return;
    }
    char buf[4096];
    auto n = fs::safeRead(task->fd_->fd(), buf, sizeof(buf));
    if (n == 0) {
        dispatcherToMain_.scheduleWithContext(
            task->offer_,
            [data = std::move(task->data_),
             callback = std::move(task->callback_)]() { callback(data); });
        tasks_.erase(task->id_);
    } else if (n < 0) {
        tasks_.erase(task->id_);
    } else {
        if (task->data_.size() + n > MAX_CLIPBOARD_SIZE) {
            tasks_.erase(task->id_);
            return;
        }
        task->data_.insert(task->data_.end(), buf, buf + n);
    }
}

void DataReaderThread::handleTaskTimeout(DataOfferTask *task) {
    FCITX_CLIPBOARD_DEBUG() << "Reading data timeout.";
    tasks_.erase(task->id_);
}

DataOffer::DataOffer(wayland::ZwlrDataControlOfferV1 *offer,
                     bool ignorePassword)
    : offer_(offer), ignorePassword_(ignorePassword) {
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

    auto callbackWrapper =
        [this, callback = std::move(callback)](const std::vector<char> &data) {
            return callback(data, isPassword_);
        };

    thread_ = &thread;
    static const std::string passwordHint = PASSWORD_MIME_TYPE;
    if (mimeTypes_.count(passwordHint)) {
        receiveDataForMime(passwordHint, [this, callbackWrapper](
                                             const std::vector<char> &data) {
            if (std::string_view(data.data(), data.size()) == "secret" &&
                ignorePassword_) {
                FCITX_CLIPBOARD_DEBUG()
                    << "Wayland clipboard contains password, ignore.";
                return;
            }
            isPassword_ = true;
            receiveRealData(callbackWrapper);
        });
    } else {
        receiveRealData(callbackWrapper);
    }
}

void DataOffer::receiveRealData(DataOfferDataCallback callback) {
    if (!thread_) {
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

    receiveDataForMime(mime, std::move(callback));
}

void DataOffer::receiveDataForMime(const std::string &mime,
                                   DataOfferDataCallback callback) {
    if (!thread_) {
        return;
    }
    // Create a pipe for sending data.
    int pipeFds[2];
    if (safePipe(pipeFds) != 0) {
        return;
    }

    offer_->receive(mime.data(), pipeFds[1]);
    close(pipeFds[1]);

    taskId_ = thread_->addTask(
        this, std::make_shared<UnixFD>(UnixFD::own(pipeFds[0])),
        std::move(callback));
}

DataDevice::DataDevice(WaylandClipboard *clipboard,
                       wayland::ZwlrDataControlDeviceV1 *device)
    : clipboard_(clipboard), device_(device),
      thread_(clipboard_->parent()->instance()->eventDispatcher()) {
    conns_.emplace_back(device_->dataOffer().connect(
        [this](wayland::ZwlrDataControlOfferV1 *offer) {
            new DataOffer(offer, *clipboard_->parent()
                                      ->config()
                                      .ignorePasswordFromPasswordManager);
        }));
    conns_.emplace_back(device_->selection().connect(
        [this](wayland::ZwlrDataControlOfferV1 *offer) {
            clipboardOffer_.reset(
                offer ? static_cast<DataOffer *>(offer->userData()) : nullptr);
            if (!clipboardOffer_) {
                return;
            }
            clipboardOffer_->receiveData(
                thread_, [this](std::vector<char> data, bool password) {
                    data.push_back('\0');
                    clipboard_->setClipboard(data.data(), password);
                    clipboardOffer_.reset();
                });
        }));
    conns_.emplace_back(device_->primarySelection().connect(
        [this](wayland::ZwlrDataControlOfferV1 *offer) {
            primaryOffer_.reset(
                offer ? static_cast<DataOffer *>(offer->userData()) : nullptr);
            if (!primaryOffer_) {
                clipboard_->setPrimary("", false);
                return;
            }
            primaryOffer_->receiveData(
                thread_, [this](std::vector<char> data, bool password) {
                    data.push_back('\0');
                    clipboard_->setPrimary(data.data(), password);
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

WaylandClipboard::WaylandClipboard(Clipboard *clipboard, std::string name,
                                   wl_display *display)
    : parent_(clipboard), name_(std::move(name)),
      display_(
          static_cast<wayland::Display *>(wl_display_get_user_data(display))) {
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

        auto *device = manager_->getDataDevice(seat.get());
        deviceMap_.emplace(seat.get(),
                           std::make_unique<DataDevice>(this, device));
    }
}

void WaylandClipboard::setClipboard(const std::string &str, bool password) {
    parent_->setClipboardV2(name_, str, password);
}

void WaylandClipboard::setPrimary(const std::string &str, bool password) {
    parent_->setPrimaryV2(name_, str, password);
}

} // namespace fcitx
