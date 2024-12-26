/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "window.h"

namespace fcitx::classicui {

Window::Window() = default;

void Window::resize(unsigned int width, unsigned int height) {
    width_ = width;
    height_ = height;
}
} // namespace fcitx::classicui
