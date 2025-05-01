/*
 * SPDX-FileCopyrightText: 2016-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "theme.h"
#include <fcntl.h>
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <cairo.h>
#include <format>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gio/gio.h>
#include <gio/gunixinputstream.h>
#include <glib.h>
#include <glibconfig.h>
#include <pango/pango-font.h>
#include <pango/pango-fontmap.h>
#include <pango/pango-layout.h>
#include <pango/pango-types.h>
#include <pango/pangocairo.h>
#include "fcitx-config/iniparser.h"
#include "fcitx-config/rawconfig.h"
#include "fcitx-utils/color.h"
#include "fcitx-utils/fs.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/macros.h"
#include "fcitx-utils/misc.h"
#include "fcitx-utils/misc_p.h"
#include "fcitx-utils/rect.h"
#include "fcitx-utils/standardpaths.h"
#include "fcitx-utils/stringutils.h"
#include "fcitx-utils/utf8.h"
#include "fcitx/icontheme.h"
#include "fcitx/misc_p.h"
#include "classicui.h"
#include "colorhelper.h"
#include "common.h"

namespace fcitx::classicui {

namespace {

inline uint32_t charWidth(uint32_t c) {
    if (g_unichar_iszerowidth(c)) {
        return 0;
    }
    return g_unichar_iswide(c) ? 2 : 1;
}

// This is heuristic, but we guranteed that we don't do crazy things with label.
std::pair<std::string, size_t> extractTextForLabel(const std::string &label) {
    std::string extracted;

    // We have non white space here because xkb shortDescription have things
    // like fr-tg, mon-a1.
    auto texts = stringutils::split(label, FCITX_WHITESPACE "-_/|");
    if (texts.empty()) {
        return {"", 0};
    }

    size_t currentWidth = 0;
    for (uint32_t chr : utf8::MakeUTF8CharRange(texts[0])) {
        const auto width = charWidth(chr);
        if (currentWidth + width <= 3) {
            extracted.append(utf8::UCS4ToUTF8(chr));
            currentWidth += width;
        } else {
            break;
        }
    }

    return {extracted, currentWidth};
}

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

    gint width;
    gint height;
    guchar *gdk_pixels;
    guchar *cairo_pixels;
    int gdk_rowstride;
    int cairo_stride;
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
            guchar *end = p + static_cast<ptrdiff_t>(3) * width;

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
            guchar *end = p + static_cast<ptrdiff_t>(4) * width;
            guint t1;
            guint t2;
            guint t3;

#define MULT(d, c, a, t)                                                       \
    G_STMT_START {                                                             \
        (t) = (c) * (a) + 0x80;                                                \
        (d) = (((t) >> 8) + (t)) >> 8;                                         \
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

    GObjectUniquePtr<GInputStream> stream(
        g_unix_input_stream_new(file.fd(), false));
    if (!stream) {
        return nullptr;
    }
    GObjectUniquePtr<GdkPixbuf> image(
        gdk_pixbuf_new_from_stream(stream.get(), nullptr, nullptr));
    g_input_stream_close(stream.get(), nullptr, nullptr);
    if (!image) {
        return nullptr;
    }

    auto *surface = pixBufToCairoSurface(image.get());
    return surface;
}

} // namespace

const std::vector<std::string> &gdkPixbufSupportedFormats() {
    const static std::vector<std::string> formats = []() {
        std::unordered_set<std::string> exts;
        std::vector<std::string> result;
        // PNG is supported by cairo.
        UniqueCPtr<GSList, g_slist_free> list(gdk_pixbuf_get_formats());
        for (GSList *item = list.get(); item; item = g_slist_next(item)) {
            gchar **extension = gdk_pixbuf_format_get_extensions(
                static_cast<GdkPixbufFormat *>(item->data));
            for (auto *iter = extension; iter && *iter; iter++) {
                exts.insert(std::string(*iter));
            }
            g_strfreev(extension);
        }

        // Only put the common types and we make a prefered order.
        for (std::string ext : {"svg", "svgz", "png", "bmp", "xpm"}) {
            // png is supported by cairo.
            if (ext == "png" || exts.count(ext)) {
                result.push_back("." + ext);
            }
        }
        CLASSICUI_DEBUG() << "Supported image extensions: " << result;
        return result;
    }();

    return formats;
}

ThemeImage::ThemeImage(const IconTheme &iconTheme, const std::string &icon,
                       const std::string &label, uint32_t size,
                       const ClassicUI *classicui)
    : size_(size) {
    bool preferTextIcon =
        !label.empty() &&
        ((icon == "input-keyboard" &&
          hasTwoKeyboardInCurrentGroup(classicui->instance())) ||
         *classicui->config().preferTextIcon);
    if (!preferTextIcon && !icon.empty()) {
        std::string iconPath =
            iconTheme.findIcon(icon, size, 1, gdkPixbufSupportedFormats());
        auto fd = open(iconPath.c_str(), O_RDONLY);
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
        drawTextIcon(image_.get(), label, size, classicui->config());
    }
}

ThemeImage::ThemeImage(const std::string &name,
                       const BackgroundImageConfig &cfg, const Color &color,
                       const Color &borderColor) {
    if (!cfg.image->empty()) {
        auto imageFile = StandardPath::global().open(
            StandardPath::Type::PkgData,
            std::format("themes/{0}/{1}", name, *cfg.image), O_RDONLY);
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
            std::format("themes/{0}/{1}", name, *cfg.overlay), O_RDONLY);
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
                          << width << " border=" << borderColor
                          << " border width=" << *cfg.borderWidth
                          << " color=" << color;
        image_.reset(
            cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height));
        auto *cr = cairo_create(image_.get());
        cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
        if (borderWidth) {
            cairoSetSourceColor(cr, borderColor);
            cairo_paint(cr);
        }

        cairo_rectangle(cr, borderWidth, borderWidth, width - borderWidth * 2,
                        height - borderWidth * 2);
        cairo_clip(cr);
        cairoSetSourceColor(cr, color);
        cairo_paint(cr);
        cairo_destroy(cr);
        isImage_ = true;
    }
}

ThemeImage::ThemeImage(const std::string &name, const ActionImageConfig &cfg) {
    if (!cfg.image->empty()) {
        auto imageFile = StandardPath::global().open(
            StandardPath::Type::PkgData,
            std::format("themes/{0}/{1}", name, *cfg.image), O_RDONLY);
        image_.reset(loadImage(imageFile));
        if (image_ &&
            cairo_surface_status(image_.get()) != CAIRO_STATUS_SUCCESS) {
            image_.reset();
        }
        valid_ = image_ != nullptr;
    }
}

void ThemeImage::drawTextIcon(cairo_surface_t *surface,
                              const std::string &rawLabel, uint32_t size,
                              const ClassicUIConfig &config) {
    auto [label, textWidth] = extractTextForLabel(rawLabel);
    auto *cr = cairo_create(surface);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairoSetSourceColor(cr, Color("#00000000"));
    cairo_paint(cr);

    int pixelSize = size * 0.7 * (textWidth >= 3 ? (2.0 / textWidth) : 1.0);
    if (pixelSize < 0) {
        pixelSize = 1;
    }
    auto *fontMap = pango_cairo_font_map_get_default();
    GObjectUniquePtr<PangoContext> context(
        pango_font_map_create_context(fontMap));
    GObjectUniquePtr<PangoLayout> layout(pango_layout_new(context.get()));
    pango_layout_set_single_paragraph_mode(layout.get(), true);
    pango_layout_set_text(layout.get(), label.c_str(), label.size());
    PangoRectangle rect;
    PangoFontDescription *desc =
        pango_font_description_from_string(config.trayFont->c_str());
    pango_font_description_set_absolute_size(desc, pixelSize * PANGO_SCALE);
    pango_layout_set_font_description(layout.get(), desc);
    pango_font_description_free(desc);
    pango_layout_get_pixel_extents(layout.get(), &rect, nullptr);
    cairo_translate(cr, (size - rect.width) * 0.5 - rect.x,
                    (size - rect.height) * 0.5 - rect.y);
    if (config.trayBorderColor->alpha()) {
        cairo_save(cr);
        cairoSetSourceColor(cr, *config.trayBorderColor);
        pango_cairo_layout_path(cr, layout.get());
        cairo_set_line_width(cr, std::min(4, (pixelSize + 4) / 8));
        cairo_stroke(cr);
        cairo_restore(cr);
    }

    cairo_save(cr);
    cairoSetSourceColor(cr, *config.trayTextColor);
    pango_cairo_show_layout(cr, layout.get());
    cairo_restore(cr);

    cairo_destroy(cr);
}

Theme::Theme() : iconTheme_(IconTheme::defaultIconThemeName()) {}

Theme::~Theme() {}

const ThemeImage &Theme::loadBackground(const BackgroundImageConfig &cfg) {
    if (auto *image = findValue(backgroundImageTable_, &cfg)) {
        return *image;
    }

    Color color;
    Color borderColor;
    if (&cfg == &*inputPanel->background) {
        color = inputPanelBackground_;
        borderColor = inputPanelBorder_;
    } else if (&cfg == &*inputPanel->highlight) {
        color = inputPanelHighlightCandidateBackground_;
        borderColor = inputPanelHighlightCandidateBorder_;
    } else if (&cfg == &*menu->background) {
        color = menuBackground_;
        borderColor = menuBorder_;
    } else if (&cfg == &*menu->highlight) {
        color = menuSelectedItemBackground_;
        borderColor = menuSelectedItemBorder_;
    } else if (&cfg == &*menu->separator) {
        color = menuSeparator_;
        borderColor = *cfg.borderColor;
    } else {
        color = *cfg.color;
        borderColor = *cfg.borderColor;
    }

    auto result = backgroundImageTable_.emplace(
        std::piecewise_construct, std::forward_as_tuple(&cfg),
        std::forward_as_tuple(name_, cfg, color, borderColor));
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
                                   const ClassicUI *classicui) {
    auto &map = trayImageTable_;
    auto name = stringutils::concat("icon:", icon, "label:", label);
    if (auto *image = findValue(map, name)) {
        if (image->size() == size) {
            return *image;
        }
        map.erase(name);
    }

    auto result = map.emplace(
        std::piecewise_construct, std::forward_as_tuple(name),
        std::forward_as_tuple(iconTheme_, icon, label, size, classicui));
    assert(result.second);
    return result.first->second;
}

void paintTile(cairo_t *c, int width, int height, double alpha,
               cairo_surface_t *image, int marginLeft, int marginTop,
               int marginRight, int marginBottom) {

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
        int w = resizeWidth;
        int h = resizeHeight;

        cairo_rectangle(c, 0, 0, w, h);
        cairo_clip(c);
        cairo_paint_with_alpha(c, alpha);
        cairo_restore(c);
    }
}

void Theme::paint(cairo_t *c, const BackgroundImageConfig &cfg, int width,
                  int height, double alpha, double scale) {
    const ThemeImage &image = loadBackground(cfg);
    auto marginTop = *cfg.margin->marginTop;
    auto marginBottom = *cfg.margin->marginBottom;
    auto marginLeft = *cfg.margin->marginLeft;
    auto marginRight = *cfg.margin->marginRight;

    if (scale != 1.0) {
        UniqueCPtr<cairo_surface_t, cairo_surface_destroy> background(
            cairo_surface_create_similar_image(
                cairo_get_target(c), CAIRO_FORMAT_ARGB32, width, height));
        {
            UniqueCPtr<cairo_t, cairo_destroy> backgroundC(
                cairo_create(background.get()));
            paintTile(backgroundC.get(), width, height, 1.0, image, marginLeft,
                      marginTop, marginRight, marginBottom);
        }
        cairo_save(c);
        cairo_rectangle(c, 0, 0, width, height);
        cairo_set_source_surface(c, background.get(), 0, 0);
        cairo_clip(c);
        cairo_paint_with_alpha(c, alpha);
        cairo_restore(c);
    } else {
        paintTile(c, width, height, alpha, image, marginLeft, marginTop,
                  marginRight, marginBottom);
    }

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

    int x = 0;
    int y = 0;
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
    trayImageTable_.clear();
    backgroundImageTable_.clear();
    actionImageTable_.clear();
}

void Theme::load(std::string_view name) {
    reset();
    ThemeConfig config;
    copyHelper(config);
    // Reset the default value to state.
    syncDefaultValueToCurrent();
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
    maskConfig_ = *inputPanel->background;
    maskConfig_.overlay.setValue("");
    maskConfig_.image.setValue(*inputPanel->blurMask);
    accentColorFields_ = std::unordered_set<ColorField>(
        accentColor.value().begin(), accentColor.value().end());
}

void Theme::load(std::string_view name, const RawConfig &rawConfig) {
    reset();
    Configuration::load(rawConfig, true);
    name_ = name;
}

bool Theme::setIconTheme(const std::string &name) {
    if (iconTheme_.internalName() != name) {
        CLASSICUI_DEBUG() << "New Icon theme: " << name;
        iconTheme_ = IconTheme(name);
        trayImageTable_.clear();
        return true;
    }
    return false;
}
std::vector<Rect> Theme::mask(const BackgroundImageConfig &cfg, int width,
                              int height) {
    UniqueCPtr<cairo_surface_t, cairo_surface_destroy> mask(
        cairo_image_surface_create(CAIRO_FORMAT_A1, width, height));
    auto *c = cairo_create(mask.get());
    cairo_set_operator(c, CAIRO_OPERATOR_SOURCE);
    paint(c, cfg, width, height, 1, 1);
    cairo_destroy(c);

    UniqueCPtr<cairo_region_t, cairo_region_destroy> region(
        cairo_region_create());
    cairo_rectangle_int_t rect;
#define AddSpan                                                                \
    do {                                                                       \
        rect.x = prev1;                                                        \
        rect.y = y;                                                            \
        rect.width = x - prev1;                                                \
        rect.height = 1;                                                       \
        cairo_region_union_rectangle(region.get(), &rect);                     \
    } while (0)
    const uint8_t zero = 0;
    const auto *data = cairo_image_surface_get_data(mask.get());
    auto cairo_stride = cairo_image_surface_get_stride(mask.get());
    constexpr bool little = G_BYTE_ORDER == G_LITTLE_ENDIAN;
    int x;
    int y;
    for (y = 0; y < height; ++y) {
        uint8_t all = zero;
        int prev1 = -1;
        for (x = 0; x < width;) {
            uint8_t byte = data[x / 8];
            if (x > width - 8 || byte != all) {
                if constexpr (little) {
                    for (int b = 8; b > 0 && x < width; --b) {
                        if (!(byte & 0x01) == !all) {
                            // More of the same
                        } else {
                            // A change.
                            if (all != zero) {
                                AddSpan;
                                all = zero;
                            } else {
                                prev1 = x;
                                all = ~zero;
                            }
                        }
                        byte >>= 1;
                        ++x;
                    }
                } else {
                    for (int b = 8; b > 0 && x < width; --b) {
                        if (!(byte & 0x80) == !all) {
                            // More of the same
                        } else {
                            // A change.
                            if (all != zero) {
                                AddSpan;
                                all = zero;
                            } else {
                                prev1 = x;
                                all = ~zero;
                            }
                        }
                        byte <<= 1;
                        ++x;
                    }
                }
            } else {
                x += 8;
            }
        }
        if (all != zero) {
            AddSpan;
        }
        data += cairo_stride;
    }
#undef AddSpan
    std::vector<Rect> result;
    for (int i = 0, e = cairo_region_num_rectangles(region.get()); i < e; i++) {
        cairo_region_get_rectangle(region.get(), i, &rect);
        result.push_back(Rect()
                             .setPosition(rect.x, rect.y)
                             .setSize(rect.width, rect.height));
    }
    return result;
}

void Theme::populateColor(std::optional<Color> accent) {
    inputPanelBackground_ = *inputPanel->background->color;
    inputPanelBorder_ = *inputPanel->background->borderColor;
    inputPanelHighlightCandidateBackground_ = *inputPanel->highlight->color;
    inputPanelHighlightCandidateBorder_ = *inputPanel->highlight->borderColor;
    inputPanelHighlight_ = *inputPanel->highlightBackgroundColor;
    inputPanelText_ = *inputPanel->normalColor;
    inputPanelHighlightText_ = *inputPanel->highlightColor;
    inputPanelHighlightCandidateText_ = *inputPanel->highlightCandidateColor;

    menuBackground_ = *menu->background->color;
    menuBorder_ = *menu->background->borderColor;
    menuSelectedItemBackground_ = *menu->highlight->color;
    menuSelectedItemBorder_ = *menu->highlight->borderColor;
    menuSeparator_ = *menu->separator->color;
    menuText_ = *menu->normalColor;
    menuSelectedItemText_ = *menu->highlightTextColor;

    if (accent) {
        auto foreground = accentForeground(*accent);
        for (auto field : accentColorFields_) {
            switch (field) {
            case ColorField::InputPanel_Background:
                inputPanelBackground_ = *accent;
                inputPanelText_ = foreground;
                break;
            case ColorField::InputPanel_Border:
                inputPanelBorder_ = *accent;
                break;
            case ColorField::InputPanel_HighlightCandidateBackground:
                inputPanelHighlightCandidateBackground_ = *accent;
                inputPanelHighlightCandidateText_ = foreground;
                break;
            case ColorField::InputPanel_HighlightCandidateBorder:
                inputPanelHighlightCandidateBorder_ = *accent;
                break;
            case ColorField::InputPanel_Highlight:
                inputPanelHighlight_ = *accent;
                inputPanelHighlightText_ = foreground;
                break;
            case ColorField::Menu_Background:
                menuBackground_ = *accent;
                menuText_ = foreground;
                break;
            case ColorField::Menu_Border:
                menuBorder_ = *accent;
                break;
            case ColorField::Menu_SelectedItemBackground:
                menuSelectedItemBackground_ = *accent;
                menuSelectedItemText_ = foreground;
                break;
            case ColorField::Menu_SelectedItemBorder:
                menuSelectedItemBorder_ = *accent;
                break;
            case ColorField::Menu_Separator:
                menuSeparator_ = *accent;
                break;
            }
        }
    }
}

} // namespace fcitx::classicui
