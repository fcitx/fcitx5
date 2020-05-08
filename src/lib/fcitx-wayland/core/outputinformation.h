/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_WAYLAND_CORE_OUTPUTINFORMATION_H_
#define _FCITX_WAYLAND_CORE_OUTPUTINFORMATION_H_

#include "wl_output.h"

#include <string>

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
} // namespace wayland
} // namespace fcitx

#endif // _FCITX_WAYLAND_CORE_OUTPUTINFORMATION_H_
