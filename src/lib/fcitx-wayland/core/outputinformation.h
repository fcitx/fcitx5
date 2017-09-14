/*
 * Copyright (C) 2017~2017 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; see the file COPYING. If not,
 * see <http://www.gnu.org/licenses/>.
 */
#ifndef _FCITX_WAYLAND_CORE_OUTPUTINFORMATION_H_
#define _FCITX_WAYLAND_CORE_OUTPUTINFORMATION_H_

#include "wl_output.h"

namespace fcitx {
namespace wayland {

class OutputInfomationPrivate;

class OutputInfomation {
    friend class Display;

public:
    OutputInfomation(wayland::WlOutput *output);
    ~OutputInfomation();

    int32_t x() const;
    int32_t y() const;
    int32_t width() const;
    int32_t height() const;
    int32_t refreshRate() const;
    int32_t phyiscalWidth() const;
    int32_t phyiscalHeight() const;
    wl_output_subpixel subpixel() const;
    const std::string &make() const;
    const std::string &model() const;
    wl_output_transform transform() const;
    int32_t scale() const;

private:
    std::unique_ptr<OutputInfomationPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(OutputInfomation);
};
}
}

#endif // _FCITX_WAYLAND_CORE_OUTPUTINFORMATION_H_
