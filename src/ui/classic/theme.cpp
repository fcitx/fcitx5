/*
 * SPDX-FileCopyrightText: 2016-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "theme.h"
#include <fcntl.h>
#include <cassert>
#include <fmt/format.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gio/gunixinputstream.h>
#include <pango/pangocairo.h>
#include "fcitx-config/iniparser.h"
#include "fcitx-utils/fs.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/rect.h"
#include "fcitx-utils/standardpath.h"
#include "fcitx/misc_p.h"
#include "common.h"

namespace fcitx::classicui {

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

    if (gdk_pixbuf_get_n_channels(image) == 3) {
        format = CAIRO_FORMAT_RGB24;
    } else {
        format = CAIRO_FORMAT_ARGB32;
    }

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
        auto *surface =
            cairo_image_surface_create_from_png_stream(readFromFd, &fd);
        if (!surface) {
            return nullptr;
        }
        if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
            g_clear_pointer(&surface, cairo_surface_destroy);
            return nullptr;
        }
        return surface;
    }

    auto *stream = g_unix_input_stream_new(file.fd(), false);
    auto *image = gdk_pixbuf_new_from_stream(stream, nullptr, nullptr);
    if (!image) {
        return nullptr;
    }

    auto *surface = pixBufToCairoSurface(image);

    g_input_stream_close(stream, nullptr, nullptr);
    g_object_unref(stream);
    g_object_unref(image);

    return surface;
}

ThemeImage::ThemeImage(const std::string &icon, const std::string &label,
                       const std::string &font, uint32_t size)
    : size_(size) {
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
        auto *cr = cairo_create(image_.get());
        cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
        cairoSetSourceColor(cr, Color("#00000000"));
        cairo_paint(cr);

        int pixelSize = size * 0.7;
        // FIXME use a color from config.
        Color color("#ffffffff");
        cairoSetSourceColor(cr, color);
        auto *fontMap = pango_cairo_font_map_get_default();
        GObjectUniquePtr<PangoContext> context(
            pango_font_map_create_context(fontMap));
        GObjectUniquePtr<PangoLayout> layout(pango_layout_new(context.get()));
        pango_layout_set_single_paragraph_mode(layout.get(), true);
        pango_layout_set_text(layout.get(), label.c_str(), label.size());
        PangoRectangle rect;
        PangoFontDescription *desc =
            pango_font_description_from_string(font.c_str());
        pango_font_description_set_absolute_size(desc, pixelSize * PANGO_SCALE);
        pango_layout_set_font_description(layout.get(), desc);
        pango_font_description_free(desc);
        pango_layout_get_pixel_extents(layout.get(), &rect, nullptr);
        cairo_move_to(cr, (size - rect.width) * 0.5 - rect.x,
                      (size - rect.height) * 0.5 - rect.y);
        pango_cairo_show_layout(cr, layout.get());

        cairo_destroy(cr);
    }
}

ThemeImage::ThemeImage(const std::string &name,
                       const BackgroundImageConfig &cfg) {
    if (!cfg.image->empty()) {
        auto imageFile = StandardPath::global().open(
            StandardPath::Type::PkgData,
            fmt::format("themes/{0}/{1}", name, *cfg.image), O_RDONLY);
        image_.reset(loadImage(imageFile));
        if (image_ &&
            cairo_surface_status(image_.get()) != CAIRO_STATUS_SUCCESS) {
            image_.reset();
        }
        valid_ = image_ != nullptr;
    }

    if (!cfg.overlay->empty()) {
        auto imageFile = StandardPath::global().open(
            StandardPath::Type::PkgData,
            fmt::format("themes/{0}/{1}", name, *cfg.overlay), O_RDONLY);
        overlay_.reset(loadImage(imageFile));
        if (overlay_ &&
            cairo_surface_status(overlay_.get()) != CAIRO_STATUS_SUCCESS) {
            overlay_.reset();
        }
    }

    if (!image_) {
        constexpr auto minimumSize = 20;
        auto width =
            *cfg.margin->marginLeft + *cfg.margin->marginRight +
            std::max(*cfg.margin->marginLeft + *cfg.margin->marginRight,
                     minimumSize);
        auto height =
            *cfg.margin->marginTop + *cfg.margin->marginBottom +
            std::max(*cfg.margin->marginTop + *cfg.margin->marginBottom,
                     minimumSize);

        auto borderWidth =
            std::min({*cfg.borderWidth, *cfg.margin->marginLeft,
                      *cfg.margin->marginRight, *cfg.margin->marginTop,
                      *cfg.margin->marginBottom});

        CLASSICUI_DEBUG() << "Paint background: height " << height << " width "
                          << width;
        image_.reset(
            cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height));
        auto *cr = cairo_create(image_.get());
        cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
        if (borderWidth) {
            cairoSetSourceColor(cr, *cfg.borderColor);
            cairo_paint(cr);
        }

        cairo_rectangle(cr, borderWidth, borderWidth, width - borderWidth * 2,
                        height - borderWidth * 2);
        cairo_clip(cr);
        cairoSetSourceColor(cr, *cfg.color);
        cairo_paint(cr);
        cairo_destroy(cr);
    }
}

ThemeImage::ThemeImage(const std::string &name, const ActionImageConfig &cfg) {
    if (!cfg.image->empty()) {
        auto imageFile = StandardPath::global().open(
            StandardPath::Type::PkgData,
            fmt::format("themes/{0}/{1}", name, *cfg.image), O_RDONLY);
        image_.reset(loadImage(imageFile));
        if (image_ &&
            cairo_surface_status(image_.get()) != CAIRO_STATUS_SUCCESS) {
            image_.reset();
        }
        valid_ = image_ != nullptr;
    }
}

Theme::Theme() : iconTheme_(IconTheme::defaultIconThemeName()) {}

Theme::~Theme() {}

const ThemeImage &Theme::loadBackground(const BackgroundImageConfig &cfg) {
    if (auto *image = findValue(backgroundImageTable_, &cfg)) {
        return *image;
    }

    auto result = backgroundImageTable_.emplace(
        std::piecewise_construct, std::forward_as_tuple(&cfg),
        std::forward_as_tuple(name_, cfg));
    assert(result.second);
    return result.first->second;
}

const ThemeImage &Theme::loadAction(const ActionImageConfig &cfg) {
    if (auto *image = findValue(actionImageTable_, &cfg)) {
        return *image;
    }

    auto result = actionImageTable_.emplace(std::piecewise_construct,
                                            std::forward_as_tuple(&cfg),
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
    if (auto *image = findValue(map, name)) {
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
                  int height, double alpha) {
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

    if (height < 0) {
        height = resizeHeight;
    }

    if (width < 0) {
        width = resizeWidth;
    }

    const auto targetResizeWidth = width - marginLeft - marginRight;
    const auto targetResizeHeight = height - marginTop - marginBottom;
    const double scaleX = static_cast<double>(targetResizeWidth) / resizeWidth;
    const double scaleY =
        static_cast<double>(targetResizeHeight) / resizeHeight;

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
        cairo_paint_with_alpha(c, alpha);
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
        cairo_paint_with_alpha(c, alpha);
        cairo_restore(c);
    }

    if (marginLeft && marginTop) {
        /* part 7 */
        cairo_save(c);
        cairo_set_source_surface(c, image, 0, 0);
        cairo_rectangle(c, 0, 0, marginLeft, marginTop);
        cairo_clip(c);
        cairo_paint_with_alpha(c, alpha);
        cairo_restore(c);
    }

    if (marginRight && marginTop) {
        /* part 9 */
        cairo_save(c);
        cairo_translate(c, width - marginRight, 0);
        cairo_set_source_surface(c, image, -marginLeft - resizeWidth, 0);
        cairo_rectangle(c, 0, 0, marginRight, marginTop);
        cairo_clip(c);
        cairo_paint_with_alpha(c, alpha);
        cairo_restore(c);
    }

    /* part 2 & 8 */
    if (marginTop && targetResizeWidth > 0) {
        cairo_save(c);
        cairo_translate(c, marginLeft, 0);
        cairo_scale(c, scaleX, 1);
        cairo_set_source_surface(c, image, -marginLeft, 0);
        cairo_rectangle(c, 0, 0, resizeWidth, marginTop);
        cairo_clip(c);
        cairo_paint_with_alpha(c, alpha);
        cairo_restore(c);
    }

    if (marginBottom && targetResizeWidth > 0) {
        cairo_save(c);
        cairo_translate(c, marginLeft, height - marginBottom);
        cairo_scale(c, scaleX, 1);
        cairo_set_source_surface(c, image, -marginLeft,
                                 -marginTop - resizeHeight);
        cairo_rectangle(c, 0, 0, resizeWidth, marginBottom);
        cairo_clip(c);
        cairo_paint_with_alpha(c, alpha);
        cairo_restore(c);
    }

    /* part 4 & 6 */
    if (marginLeft && targetResizeHeight > 0) {
        cairo_save(c);
        cairo_translate(c, 0, marginTop);
        cairo_scale(c, 1, scaleY);
        cairo_set_source_surface(c, image, 0, -marginTop);
        cairo_rectangle(c, 0, 0, marginLeft, resizeHeight);
        cairo_clip(c);
        cairo_paint_with_alpha(c, alpha);
        cairo_restore(c);
    }

    if (marginRight && targetResizeHeight > 0) {
        cairo_save(c);
        cairo_translate(c, width - marginRight, marginTop);
        cairo_scale(c, 1, scaleY);
        cairo_set_source_surface(c, image, -marginLeft - resizeWidth,
                                 -marginTop);
        cairo_rectangle(c, 0, 0, marginRight, resizeHeight);
        cairo_clip(c);
        cairo_paint_with_alpha(c, alpha);
        cairo_restore(c);
    }

    /* part 5 */
    if (targetResizeHeight > 0 && targetResizeWidth > 0) {
        cairo_save(c);
        cairo_translate(c, marginLeft, marginTop);
        cairo_scale(c, scaleX, scaleY);
        cairo_set_source_surface(c, image, -marginLeft, -marginTop);
        cairo_pattern_set_filter(cairo_get_source(c), CAIRO_FILTER_NEAREST);
        int w = resizeWidth, h = resizeHeight;

        cairo_rectangle(c, 0, 0, w, h);
        cairo_clip(c);
        cairo_paint_with_alpha(c, alpha);
        cairo_restore(c);
    }
    cairo_restore(c);

    if (!image.overlay()) {
        return;
    }

    Rect clipRect;
    auto clipWidth = width - *cfg.overlayClipMargin->marginLeft -
                     *cfg.overlayClipMargin->marginRight;
    auto clipHeight = height - *cfg.overlayClipMargin->marginTop -
                      *cfg.overlayClipMargin->marginBottom;
    if (clipWidth <= 0 || clipHeight <= 0) {
        return;
    }
    clipRect
        .setPosition(*cfg.overlayClipMargin->marginLeft,
                     *cfg.overlayClipMargin->marginTop)
        .setSize(clipWidth, clipHeight);

    int x = 0, y = 0;
    switch (*cfg.gravity) {
    case Gravity::TopLeft:
    case Gravity::CenterLeft:
    case Gravity::BottomLeft:
        x = *cfg.overlayOffsetX;
        break;
    case Gravity::TopCenter:
    case Gravity::Center:
    case Gravity::BottomCenter:
        x = (width - image.overlayWidth()) / 2 + *cfg.overlayOffsetX;
        break;
    case Gravity::TopRight:
    case Gravity::CenterRight:
    case Gravity::BottomRight:
        x = width - image.overlayWidth() - *cfg.overlayOffsetX;
        break;
    }
    switch (*cfg.gravity) {
    case Gravity::TopLeft:
    case Gravity::TopCenter:
    case Gravity::TopRight:
        y = *cfg.overlayOffsetY;
        break;
    case Gravity::CenterLeft:
    case Gravity::Center:
    case Gravity::CenterRight:
        y = (height - image.overlayHeight()) / 2 + *cfg.overlayOffsetY;
        break;
    case Gravity::BottomLeft:
    case Gravity::BottomCenter:
    case Gravity::BottomRight:
        y = height - image.overlayHeight() - *cfg.overlayOffsetY;
        break;
    }
    Rect rect;
    rect.setPosition(x, y).setSize(image.overlayWidth(), image.overlayHeight());
    Rect finalRect = rect.intersected(clipRect);
    if (finalRect.isEmpty()) {
        return;
    }

    if (*cfg.hideOverlayIfOversize && !clipRect.contains(rect)) {
        return;
    }

    cairo_save(c);
    cairo_set_operator(c, CAIRO_OPERATOR_OVER);
    cairo_translate(c, finalRect.left(), finalRect.top());
    cairo_set_source_surface(c, image.overlay(), x - finalRect.left(),
                             y - finalRect.top());
    cairo_rectangle(c, 0, 0, finalRect.width(), finalRect.height());
    cairo_clip(c);
    cairo_paint_with_alpha(c, alpha);
    cairo_restore(c);
}

void Theme::paint(cairo_t *c, const ActionImageConfig &cfg, double alpha) {
    const ThemeImage &image = loadAction(cfg);
    int height = cairo_image_surface_get_height(image);
    int width = cairo_image_surface_get_width(image);

    cairo_save(c);
    cairo_set_source_surface(c, image, 0, 0);
    cairo_rectangle(c, 0, 0, width, height);
    cairo_clip(c);
    cairo_paint_with_alpha(c, alpha);
    cairo_restore(c);
}

void Theme::reset() {
    imageTable_.clear();
    trayImageTable_.clear();
    backgroundImageTable_.clear();
    actionImageTable_.clear();
}

void Theme::load(const std::string &name) {
    reset();
    if (auto themeConfigFile = StandardPath::global().openSystem(
            StandardPath::Type::PkgData,
            stringutils::joinPath("themes", name, "theme.conf"), O_RDONLY);
        themeConfigFile.isValid()) {
        RawConfig themeConfig;
        readFromIni(themeConfig, themeConfigFile.fd());
        Configuration::load(themeConfig, true);
    } else {
        // No sys file, reset default value.
        ThemeConfig config;
        copyHelper(config);
    }
    syncDefaultValueToCurrent();
    if (auto themeConfigFile = StandardPath::global().openUser(
            StandardPath::Type::PkgData,
            stringutils::joinPath("themes", name, "theme.conf"), O_RDONLY);
        themeConfigFile.isValid()) {
        // Has user file, load user file data.
        RawConfig themeConfig;
        readFromIni(themeConfig, themeConfigFile.fd());
        Configuration::load(themeConfig, true);
    }
    name_ = name;
}

void Theme::load(const std::string &name, const RawConfig &rawConfig) {
    reset();
    Configuration::load(rawConfig, true);
    name_ = name;
}

bool Theme::setIconTheme(const std::string &name) {
    if (iconTheme_.internalName() != name) {
        FCITX_INFO() << "New Icon theme: " << name;
        iconTheme_ = IconTheme(name);
        trayImageTable_.clear();
        return true;
    }
    return false;
}
} // namespace fcitx::classicui
