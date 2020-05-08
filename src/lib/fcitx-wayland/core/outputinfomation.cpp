/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "outputinformation.h"

namespace fcitx {
namespace wayland {

class OutputInfomationData {
public:
    int32_t x_, y_;
    int32_t width_, height_;
    int32_t refreshRate_;
    int32_t physicalWidth_;
    int32_t physicalHeight_;
    wl_output_subpixel subpixel_;
    std::string make_;
    std::string model_;
    wl_output_transform transform_;
    int32_t scale_;
};

class OutputInfomationPrivate {
public:
    OutputInfomationData current_, next_;
    ScopedConnection geometryConnection_, modeConnection_, scaleConnection_,
        doneConnection_;
};

OutputInfomation::OutputInfomation(WlOutput *output)
    : d_ptr(std::make_unique<OutputInfomationPrivate>()) {
    FCITX_D();
    d->geometryConnection_ = output->geometry().connect(
        [this](int32_t x, int32_t y, int32_t physicalWidth,
               int32_t physicalHeight, int32_t subpixel, const char *make,
               const char *model, int32_t transform) {
            FCITX_D();
            d->next_.x_ = x;
            d->next_.y_ = y;
            d->next_.physicalWidth_ = physicalWidth;
            d->next_.physicalHeight_ = physicalHeight;
            d->next_.subpixel_ = static_cast<wl_output_subpixel>(subpixel);
            d->next_.make_ = make;
            d->next_.model_ = model;
            d->next_.transform_ = static_cast<wl_output_transform>(transform);
        });
    d->modeConnection_ = output->mode().connect(
        [this](uint32_t flags, int32_t width, int32_t height, int32_t refresh) {
            if (!(flags & WL_OUTPUT_MODE_CURRENT)) {
                return;
            }

            FCITX_D();
            d->next_.width_ = width;
            d->next_.height_ = height;
            d->next_.refreshRate_ = refresh;
        });
    d->scaleConnection_ = output->scale().connect([this](int32_t scale) {
        FCITX_D();
        d->next_.scale_ = scale;
    });
    d->doneConnection_ = output->done().connect([this]() {
        FCITX_D();
        d->current_ = d->next_;
    });
}

OutputInfomation::~OutputInfomation() {}

int32_t OutputInfomation::x() const {
    FCITX_D();
    return d->current_.x_;
}
int32_t OutputInfomation::y() const {
    FCITX_D();
    return d->current_.y_;
}
int32_t OutputInfomation::width() const {
    FCITX_D();
    return d->current_.width_;
}
int32_t OutputInfomation::height() const {
    FCITX_D();
    return d->current_.height_;
}
int32_t OutputInfomation::refreshRate() const {
    FCITX_D();
    return d->current_.refreshRate_;
}
int32_t OutputInfomation::phyiscalWidth() const {
    FCITX_D();
    return d->current_.physicalWidth_;
}
int32_t OutputInfomation::phyiscalHeight() const {
    FCITX_D();
    return d->current_.physicalHeight_;
}
wl_output_subpixel OutputInfomation::subpixel() const {
    FCITX_D();
    return d->current_.subpixel_;
}
const std::string &OutputInfomation::make() const {
    FCITX_D();
    return d->current_.make_;
}
const std::string &OutputInfomation::model() const {
    FCITX_D();
    return d->current_.model_;
}
wl_output_transform OutputInfomation::transform() const {
    FCITX_D();
    return d->current_.transform_;
}
int32_t OutputInfomation::scale() const {
    FCITX_D();
    return d->current_.scale_;
}
} // namespace wayland
} // namespace fcitx
