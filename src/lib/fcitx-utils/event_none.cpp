
/*
 * SPDX-FileCopyrightText: 2015-2015 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include <memory>
#include "event_p.h"
#include "eventloopinterface.h"

namespace fcitx {

std::unique_ptr<EventLoopInterface> createDefaultEventLoop() { return nullptr; }

const char *defaultEventLoopImplementation() { return "none"; }

} // namespace fcitx