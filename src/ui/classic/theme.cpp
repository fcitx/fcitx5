/*
 * Copyright (C) 2016~2017 by CSSlayer
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

#include "theme.h"
#include "fcitx-utils/standardpath.h"
#include "fcitx/misc_p.h"
#include <cassert>
#include <fcitx-utils/fs.h>
#include <fcitx-utils/log.h>
#include <fcntl.h>
#include <fmt/format.h>

namespace fcitx {

FCITX_DEFINE_LOG_CATEGORY(classicui_logcategory, "classicui");

cairo_status_t readFromFd(void *closure, unsigned char *data,
                          unsigned int length) {
    int fd = *static_cast<int *>(closure);

    while (length) {
        auto sz = fs::safeRead(fd, data, length);
        if (sz <= 0) {
            return CAIRO_STATUS_READ_ERROR;
        }
        length -= sz;
        data += sz;
    }
    return CAIRO_STATUS_SUCCESS;
}

ThemeImage::ThemeImage(const std::string &name,
                       const BackgroundImageConfig &cfg)
    : image_(nullptr, &cairo_surface_destroy) {
    if (!cfg.image->empty()) {
        auto imageFile = StandardPath::global().open(
            StandardPath::Type::PkgData,
            fmt::format("themes/{0}/{1}", name, *cfg.image), O_RDONLY);
        if (imageFile.fd() >= 0) {
            int fd = imageFile.fd();
            image_.reset(
                cairo_image_surface_create_from_png_stream(readFromFd, &fd));
            if (cairo_surface_status(image_.get()) != CAIRO_STATUS_SUCCESS) {
                image_.reset();
            }
        }
    }

    if (!image_) {
        auto width = *cfg.margin->marginLeft + *cfg.margin->marginRight + 1;
        auto height = *cfg.margin->marginTop + *cfg.margin->marginBottom + 1;

        CLASSICUI_DEBUG() << "height" << height << "width" << width;
        image_.reset(
            cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height));
        auto cr = cairo_create(image_.get());
        cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
        cairoSetSourceColor(cr, *cfg.color);
        cairo_paint(cr);
        cairo_destroy(cr);
    }
}

ThemeImage::ThemeImage(const std::string &name, const std::string &icon,
                       const std::string &label, const std::string &font)
    : image_(nullptr, &cairo_surface_destroy) {}

Theme::Theme() {}

Theme::~Theme() {}

const ThemeImage &Theme::loadBackground(const BackgroundImageConfig &cfg) {
    if (auto image = findValue(backgroundImageTable_, &cfg)) {
        return *image;
    }

    auto result = backgroundImageTable_.emplace(
        std::piecewise_construct, std::forward_as_tuple(&cfg),
        std::forward_as_tuple(name_, cfg));
    assert(result.second);
    return result.first->second;
}

void Theme::paint(cairo_t *c, const BackgroundImageConfig &cfg, int width,
                  int height) {
    const ThemeImage &image = loadBackground(cfg);
    auto marginTop = *cfg.margin->marginTop;
    auto marginBottom = *cfg.margin->marginBottom;
    auto marginLeft = *cfg.margin->marginLeft;
    auto marginRight = *cfg.margin->marginRight;
    int resizeHeight =
        cairo_image_surface_get_height(image) - marginTop - marginBottom;
    int resizeWidth =
        cairo_image_surface_get_width(image) - marginLeft - marginRight;
    CLASSICUI_DEBUG() << "resizeHeight" << resizeHeight << "resizeWidth"
                      << resizeWidth;

    if (resizeHeight <= 0) {
        resizeHeight = 1;
    }

    if (resizeWidth <= 0) {
        resizeWidth = 1;
    }
    cairo_save(c);

    /*
     * 7 8 9
     * 4 5 6
     * 1 2 3
     */

    if (marginLeft && marginBottom) {
        /* part 1 */
        cairo_save(c);
        cairo_translate(c, 0, height - marginBottom);
        cairo_set_source_surface(c, image, 0, -marginTop - resizeHeight);
        cairo_rectangle(c, 0, 0, marginLeft, marginBottom);
        cairo_clip(c);
        cairo_paint(c);
        cairo_restore(c);
    }

    if (marginRight && marginBottom) {
        /* part 3 */
        cairo_save(c);
        cairo_translate(c, width - marginRight, height - marginBottom);
        cairo_set_source_surface(c, image, -marginLeft - resizeWidth,
                                 -marginTop - resizeHeight);
        cairo_rectangle(c, 0, 0, marginRight, marginBottom);
        cairo_clip(c);
        cairo_paint(c);
        cairo_restore(c);
    }

    if (marginLeft && marginTop) {
        /* part 7 */
        cairo_save(c);
        cairo_set_source_surface(c, image, 0, 0);
        cairo_rectangle(c, 0, 0, marginLeft, marginTop);
        cairo_clip(c);
        cairo_paint(c);
        cairo_restore(c);
    }

    if (marginRight && marginTop) {
        /* part 9 */
        cairo_save(c);
        cairo_translate(c, width - marginRight, 0);
        cairo_set_source_surface(c, image, -marginLeft - resizeWidth, 0);
        cairo_rectangle(c, 0, 0, marginRight, marginTop);
        cairo_clip(c);
        cairo_paint(c);
        cairo_restore(c);
    }

    /* part 2 & 8 */
    if (marginTop) {
        cairo_save(c);
        cairo_translate(c, marginLeft, 0);
        cairo_scale(
            c, (double)(width - marginLeft - marginRight) / (double)resizeWidth,
            1);
        cairo_set_source_surface(c, image, -marginLeft, 0);
        cairo_rectangle(c, 0, 0, resizeWidth, marginTop);
        cairo_clip(c);
        cairo_paint(c);
        cairo_restore(c);
    }

    if (marginBottom) {
        cairo_save(c);
        cairo_translate(c, marginLeft, height - marginBottom);
        cairo_scale(
            c, (double)(width - marginLeft - marginRight) / (double)resizeWidth,
            1);
        cairo_set_source_surface(c, image, -marginLeft,
                                 -marginTop - resizeHeight);
        cairo_rectangle(c, 0, 0, resizeWidth, marginBottom);
        cairo_clip(c);
        cairo_paint(c);
        cairo_restore(c);
    }

    /* part 4 & 6 */
    if (marginLeft) {
        cairo_save(c);
        cairo_translate(c, 0, marginTop);
        cairo_scale(c, 1,
                    (double)(height - marginTop - marginBottom) /
                        (double)resizeHeight);
        cairo_set_source_surface(c, image, 0, -marginTop);
        cairo_rectangle(c, 0, 0, marginLeft, resizeHeight);
        cairo_clip(c);
        cairo_paint(c);
        cairo_restore(c);
    }

    if (marginRight) {
        cairo_save(c);
        cairo_translate(c, width - marginRight, marginTop);
        cairo_scale(c, 1,
                    (double)(height - marginTop - marginBottom) /
                        (double)resizeHeight);
        cairo_set_source_surface(c, image, -marginLeft - resizeWidth,
                                 -marginTop);
        cairo_rectangle(c, 0, 0, marginRight, resizeHeight);
        cairo_clip(c);
        cairo_paint(c);
        cairo_restore(c);
    }

    /* part 5 */
    {
        int repaintH = 0, repaintV = 0;
        double scaleX = 1.0, scaleY = 1.0;

        repaintH = 1;
        scaleX =
            (double)(width - marginLeft - marginRight) / (double)resizeWidth;

        repaintV = 1;
        scaleY =
            (double)(height - marginTop - marginBottom) / (double)resizeHeight;

        int i, j;
        for (i = 0; i < repaintH; i++) {
            for (j = 0; j < repaintV; j++) {
                cairo_save(c);
                cairo_translate(c, marginLeft + i * resizeWidth,
                                marginTop + j * resizeHeight);
                cairo_scale(c, scaleX, scaleY);
                cairo_set_source_surface(c, image, -marginLeft, -marginTop);
                cairo_pattern_set_filter(cairo_get_source(c),
                                         CAIRO_FILTER_NEAREST);
                int w = resizeWidth, h = resizeHeight;

                cairo_rectangle(c, 0, 0, w, h);
                cairo_clip(c);
                cairo_paint(c);
                cairo_restore(c);
            }
        }
    }
    cairo_restore(c);
}

void Theme::load(const std::string &name, const RawConfig &rawConfig) {
    imageTable_.clear();
    trayImageTable_.clear();
    backgroundImageTable_.clear();
    Configuration::load(rawConfig);
    name_ = name;
}
}
