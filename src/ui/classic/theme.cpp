/*
 * Copyright (C) 2016~2016 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
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

namespace fcitx {

void paint(cairo_t *c, cairo_surface_t *image, BackgroundImageConfig *cfg,
           int width, int height) {
    auto marginTop = cfg->margin.value().marginTop.value();
    auto marginBottom = cfg->margin.value().marginBottom.value();
    auto marginLeft = cfg->margin.value().marginLeft.value();
    auto marginRight = cfg->margin.value().marginRight.value();
    auto fillHorizontal = cfg->fillHorizontal.value();
    auto fillVertical = cfg->fillVertical.value();
    int resizeHeight =
        cairo_image_surface_get_height(image) - marginTop - marginBottom;
    int resizeWidth =
        cairo_image_surface_get_width(image) - marginLeft - marginRight;

    if (resizeHeight <= 0) {
        resizeHeight = 1;
    }

    if (resizeWidth <= 0) {
        resizeWidth = 1;
    }
    cairo_save(c);

    cairo_set_operator(c, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(c, image, 0, 0);

    /*
     * 7 8 9
     * 4 5 6
     * 1 2 3
     */

    /* part 1 */
    cairo_save(c);
    cairo_translate(c, 0, height - marginBottom);
    cairo_set_source_surface(c, image, 0, -marginTop - resizeHeight);
    cairo_rectangle(c, 0, 0, marginLeft, marginBottom);
    cairo_clip(c);
    cairo_paint(c);
    cairo_restore(c);

    /* part 3 */
    cairo_save(c);
    cairo_translate(c, width - marginRight, height - marginBottom);
    cairo_set_source_surface(c, image, -marginLeft - resizeWidth,
                             -marginTop - resizeHeight);
    cairo_rectangle(c, 0, 0, marginRight, marginBottom);
    cairo_clip(c);
    cairo_paint(c);
    cairo_restore(c);

    /* part 7 */
    cairo_save(c);
    cairo_rectangle(c, 0, 0, marginLeft, marginTop);
    cairo_clip(c);
    cairo_paint(c);
    cairo_restore(c);

    /* part 9 */
    cairo_save(c);
    cairo_translate(c, width - marginRight, 0);
    cairo_set_source_surface(c, image, -marginLeft - resizeWidth, 0);
    cairo_rectangle(c, 0, 0, marginRight, marginTop);
    cairo_clip(c);
    cairo_paint(c);
    cairo_restore(c);

    /* part 2 & 8 */
    {
        if (fillHorizontal == FillRule::Copy) {
            int repaint_times =
                (width - marginLeft - marginRight) / resizeWidth;
            int remain_width = (width - marginLeft - marginRight) % resizeWidth;
            int i = 0;

            for (i = 0; i < repaint_times; i++) {
                /* part 8 */
                cairo_save(c);
                cairo_translate(c, marginLeft + i * resizeWidth, 0);
                cairo_set_source_surface(c, image, -marginLeft, 0);
                cairo_rectangle(c, 0, 0, resizeWidth, marginTop);
                cairo_clip(c);
                cairo_paint(c);
                cairo_restore(c);

                /* part 2 */
                cairo_save(c);
                cairo_translate(c, marginLeft + i * resizeWidth,
                                height - marginBottom);
                cairo_set_source_surface(c, image, -marginLeft,
                                         -marginTop - resizeHeight);
                cairo_rectangle(c, 0, 0, resizeWidth, marginBottom);
                cairo_clip(c);
                cairo_paint(c);
                cairo_restore(c);
            }

            if (remain_width != 0) {
                /* part 8 */
                cairo_save(c);
                cairo_translate(c, marginLeft + repaint_times * resizeWidth, 0);
                cairo_set_source_surface(c, image, -marginLeft, 0);
                cairo_rectangle(c, 0, 0, remain_width, marginTop);
                cairo_clip(c);
                cairo_paint(c);
                cairo_restore(c);

                /* part 2 */
                cairo_save(c);
                cairo_translate(c, marginLeft + repaint_times * resizeWidth,
                                height - marginBottom);
                cairo_set_source_surface(c, image, -marginLeft,
                                         -marginTop - resizeHeight);
                cairo_rectangle(c, 0, 0, remain_width, marginBottom);
                cairo_clip(c);
                cairo_paint(c);
                cairo_restore(c);
            }
        } else {
            cairo_save(c);
            cairo_translate(c, marginLeft, 0);
            cairo_scale(c, (double)(width - marginLeft - marginRight) /
                               (double)resizeWidth,
                        1);
            cairo_set_source_surface(c, image, -marginLeft, 0);
            cairo_rectangle(c, 0, 0, resizeWidth, marginTop);
            cairo_clip(c);
            cairo_paint(c);
            cairo_restore(c);

            cairo_save(c);
            cairo_translate(c, marginLeft, height - marginBottom);
            cairo_scale(c, (double)(width - marginLeft - marginRight) /
                               (double)resizeWidth,
                        1);
            cairo_set_source_surface(c, image, -marginLeft,
                                     -marginTop - resizeHeight);
            cairo_rectangle(c, 0, 0, resizeWidth, marginBottom);
            cairo_clip(c);
            cairo_paint(c);
            cairo_restore(c);
        }
    }

    /* part 4 & 6 */
    {
        if (fillVertical == FillRule::Copy) {
            int repaint_times =
                (height - marginTop - marginBottom) / resizeHeight;
            int remain_height =
                (height - marginTop - marginBottom) % resizeHeight;
            int i = 0;

            for (i = 0; i < repaint_times; i++) {
                /* part 4 */
                cairo_save(c);
                cairo_translate(c, 0, marginTop + i * resizeHeight);
                cairo_set_source_surface(c, image, 0, -marginTop);
                cairo_rectangle(c, 0, 0, marginLeft, resizeHeight);
                cairo_clip(c);
                cairo_paint(c);
                cairo_restore(c);

                /* part 6 */
                cairo_save(c);
                cairo_translate(c, width - marginRight,
                                marginTop + i * resizeHeight);
                cairo_set_source_surface(c, image, -marginLeft - resizeWidth,
                                         -marginTop);
                cairo_rectangle(c, 0, 0, marginRight, resizeHeight);
                cairo_clip(c);
                cairo_paint(c);
                cairo_restore(c);
            }

            if (remain_height != 0) {
                /* part 8 */
                cairo_save(c);
                cairo_translate(c, 0, marginTop + repaint_times * resizeHeight);
                cairo_set_source_surface(c, image, 0, -marginTop);
                cairo_rectangle(c, 0, 0, marginLeft, remain_height);
                cairo_clip(c);
                cairo_paint(c);
                cairo_restore(c);

                /* part 2 */
                cairo_save(c);
                cairo_translate(c, width - marginRight,
                                marginTop + repaint_times * resizeHeight);
                cairo_set_source_surface(c, image, -marginLeft - resizeWidth,
                                         -marginTop);
                cairo_rectangle(c, 0, 0, marginRight, remain_height);
                cairo_clip(c);
                cairo_paint(c);
                cairo_restore(c);
            }
        } else {
            cairo_save(c);
            cairo_translate(c, 0, marginTop);
            cairo_scale(c, 1, (double)(height - marginTop - marginBottom) /
                                  (double)resizeHeight);
            cairo_set_source_surface(c, image, 0, -marginTop);
            cairo_rectangle(c, 0, 0, marginLeft, resizeHeight);
            cairo_clip(c);
            cairo_paint(c);
            cairo_restore(c);

            cairo_save(c);
            cairo_translate(c, width - marginRight, marginTop);
            cairo_scale(c, 1, (double)(height - marginTop - marginBottom) /
                                  (double)resizeHeight);
            cairo_set_source_surface(c, image, -marginLeft - resizeWidth,
                                     -marginTop);
            cairo_rectangle(c, 0, 0, marginRight, resizeHeight);
            cairo_clip(c);
            cairo_paint(c);
            cairo_restore(c);
        }
    }

    /* part 5 */
    {
        int repaintH = 0, repaintV = 0;
        int remainW = 0, remainH = 0;
        double scaleX = 1.0, scaleY = 1.0;

        if (fillHorizontal == FillRule::Copy) {
            repaintH = (width - marginLeft - marginRight) / resizeWidth + 1;
            remainW = (width - marginLeft - marginRight) % resizeWidth;
        } else {
            repaintH = 1;
            scaleX = (double)(width - marginLeft - marginRight) /
                     (double)resizeWidth;
        }

        if (fillVertical == FillRule::Copy) {
            repaintV =
                (height - marginTop - marginBottom) / (double)resizeHeight + 1;
            remainH = (height - marginTop - marginBottom) % resizeHeight;
        } else {
            repaintV = 1;
            scaleY = (double)(height - marginTop - marginBottom) /
                     (double)resizeHeight;
        }

        int i, j;
        for (i = 0; i < repaintH; i++) {
            for (j = 0; j < repaintV; j++) {
                cairo_save(c);
                cairo_translate(c, marginLeft + i * resizeWidth,
                                marginTop + j * resizeHeight);
                cairo_scale(c, scaleX, scaleY);
                cairo_set_source_surface(c, image, -marginLeft, -marginTop);
                int w = resizeWidth, h = resizeHeight;

                if (fillVertical == FillRule::Copy && j == repaintV - 1)
                    h = remainH;

                if (fillHorizontal == FillRule::Copy && i == repaintH - 1)
                    w = remainW;

                cairo_rectangle(c, 0, 0, w, h);
                cairo_clip(c);
                cairo_paint(c);
                cairo_restore(c);
            }
        }
    }
    cairo_restore(c);
}

ThemeImage::ThemeImage() : image_(nullptr, &cairo_surface_destroy) {}

ThemeImage::~ThemeImage() {}

cairo_surface_t *ThemeImage::loadSurface(const std::string &name,
                                         const std::string &fallbackText) {

    return image_.get();
}

Theme::Theme() {}

Theme::~Theme() {}

void Theme::load(RawConfig &rawConfig) {
    imageTable.clear();
    trayImageTable.clear();
    Configuration::load(rawConfig);
}
}
