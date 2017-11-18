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
#include "fcitx-utils/fs.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/standardpath.h"
#include "fcitx/misc_p.h"
#include <cassert>
#include <fcntl.h>
#include <fmt/format.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gio/gunixinputstream.h>
#include <pango/pangocairo.h>

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

cairo_surface_t *pixBufToCairoSurface(GdkPixbuf *image) {
    cairo_format_t format;
    cairo_surface_t *surface;

    if (gdk_pixbuf_get_n_channels(image) == 3)
        format = CAIRO_FORMAT_RGB24;
    else
        format = CAIRO_FORMAT_ARGB32;

    surface = cairo_image_surface_create(format, gdk_pixbuf_get_width(image),
                                         gdk_pixbuf_get_height(image));

    gint width, height;
    guchar *gdk_pixels, *cairo_pixels;
    int gdk_rowstride, cairo_stride;
    int n_channels;
    int j;

    if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
        cairo_surface_destroy(surface);
        return nullptr;
    }

    cairo_surface_flush(surface);

    width = gdk_pixbuf_get_width(image);
    height = gdk_pixbuf_get_height(image);
    gdk_pixels = gdk_pixbuf_get_pixels(image);
    gdk_rowstride = gdk_pixbuf_get_rowstride(image);
    n_channels = gdk_pixbuf_get_n_channels(image);
    cairo_stride = cairo_image_surface_get_stride(surface);
    cairo_pixels = cairo_image_surface_get_data(surface);

    for (j = height; j; j--) {
        guchar *p = gdk_pixels;
        guchar *q = cairo_pixels;

        if (n_channels == 3) {
            guchar *end = p + 3 * width;

            while (p < end) {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
                q[0] = p[2];
                q[1] = p[1];
                q[2] = p[0];
                q[3] = 0xFF;
#else
                q[0] = 0xFF;
                q[1] = p[0];
                q[2] = p[1];
                q[3] = p[2];
#endif
                p += 3;
                q += 4;
            }
        } else {
            guchar *end = p + 4 * width;
            guint t1, t2, t3;

#define MULT(d, c, a, t)                                                       \
    G_STMT_START {                                                             \
        t = c * a + 0x80;                                                      \
        d = ((t >> 8) + t) >> 8;                                               \
    }                                                                          \
    G_STMT_END

            while (p < end) {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
                MULT(q[0], p[2], p[3], t1);
                MULT(q[1], p[1], p[3], t2);
                MULT(q[2], p[0], p[3], t3);
                q[3] = p[3];
#else
                q[0] = p[3];
                MULT(q[1], p[0], p[3], t1);
                MULT(q[2], p[1], p[3], t2);
                MULT(q[3], p[2], p[3], t3);
#endif

                p += 4;
                q += 4;
            }

#undef MULT
        }

        gdk_pixels += gdk_rowstride;
        cairo_pixels += cairo_stride;
    }

    cairo_surface_mark_dirty(surface);
    return surface;
}

cairo_surface_t *loadImage(StandardPathFile &file) {
    if (file.fd() < 0) {
        return nullptr;
    }
    if (stringutils::endsWith(file.path(), ".png")) {
        int fd = file.fd();
        auto surface =
            cairo_image_surface_create_from_png_stream(readFromFd, &fd);
        if (!surface) {
            return nullptr;
        }
        if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
            g_clear_pointer(&surface, cairo_surface_destroy);
            return nullptr;
        }
        return surface;
    } else {
        auto *stream = g_unix_input_stream_new(file.fd(), false);
        auto image = gdk_pixbuf_new_from_stream(stream, nullptr, nullptr);
        if (!image) {
            return nullptr;
        }

        auto surface = pixBufToCairoSurface(image);

        g_input_stream_close(stream, nullptr, nullptr);
        g_object_unref(stream);
        g_object_unref(image);

        return surface;
    }
    return nullptr;
}

ThemeImage::ThemeImage(const std::string &icon, const std::string &label,
                       const std::string &font, uint32_t size)
    : size_(size), image_(nullptr, &cairo_surface_destroy) {
    if (!icon.empty()) {
        auto fd = open(icon.c_str(), O_RDONLY);
        StandardPathFile file(fd, icon);
        image_.reset(loadImage(file));
        if (image_ &&
            cairo_surface_status(image_.get()) != CAIRO_STATUS_SUCCESS) {
            image_.reset();
        }
    }
    if (!image_) {
        image_.reset(
            cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size, size));
        auto cr = cairo_create(image_.get());
        cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
        cairoSetSourceColor(cr, Color("#00000000"));
        cairo_paint(cr);

        int pixelSize = size * 0.7;
        // FIXME use a color from config.
        Color color("#ffffffff");
        cairoSetSourceColor(cr, color);
        auto fontMap = pango_cairo_font_map_get_default();
        std::unique_ptr<PangoContext, decltype(&g_object_unref)> context(
            pango_font_map_create_context(fontMap), &g_object_unref);
        std::unique_ptr<PangoLayout, decltype(&g_object_unref)> layout(
            pango_layout_new(context.get()), &g_object_unref);
        pango_layout_set_single_paragraph_mode(layout.get(), true);
        pango_layout_set_text(layout.get(), label.c_str(), label.size());
        pango_layout_set_height(layout.get(), -(2 << 20));
        PangoRectangle rect;
        PangoFontDescription *desc =
            pango_font_description_from_string(font.c_str());
        pango_font_description_set_absolute_size(desc, pixelSize * PANGO_SCALE);
        pango_layout_set_font_description(layout.get(), desc);
        pango_font_description_free(desc);
        pango_layout_get_pixel_extents(layout.get(), &rect, NULL);
        cairo_move_to(cr, (size - rect.width) * 0.5 - rect.x,
                      (size - rect.height) * 0.5 - rect.y);
        pango_cairo_show_layout(cr, layout.get());

        cairo_destroy(cr);
    }
}

ThemeImage::ThemeImage(const std::string &name,
                       const BackgroundImageConfig &cfg)
    : image_(nullptr, &cairo_surface_destroy) {
    if (!cfg.image->empty()) {
        auto imageFile = StandardPath::global().open(
            StandardPath::Type::PkgData,
            fmt::format("themes/{0}/{1}", name, *cfg.image), O_RDONLY);
        image_.reset(loadImage(imageFile));
        if (image_ &&
            cairo_surface_status(image_.get()) != CAIRO_STATUS_SUCCESS) {
            image_.reset();
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

Theme::Theme() : iconTheme_(IconTheme::defaultIconThemeName()) {}

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

const ThemeImage &Theme::loadImage(const std::string &icon,
                                   const std::string &label, uint32_t size,
                                   ImagePurpose purpose) {
    auto &map =
        purpose == ImagePurpose::General ? imageTable_ : trayImageTable_;
    auto name = stringutils::concat("icon:", icon, "label:", label);
    if (auto image = findValue(map, name)) {
        if (image->size() == size) {
            return *image;
        }
        map.erase(name);
    }

    std::string iconPath;
    if (!icon.empty()) {
        iconPath = iconTheme_.findIcon(icon, size, 1);
    }

    auto result = map.emplace(
        std::piecewise_construct, std::forward_as_tuple(name),
        std::forward_as_tuple(iconPath, label, *config->trayFont, size));
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
