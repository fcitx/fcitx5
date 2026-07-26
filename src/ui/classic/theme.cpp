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
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>
#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gio/gio.h>
#include <gio/gunixinputstream.h>
#include <glib.h>
#include <glibconfig.h>
#include <librsvg/rsvg.h>
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

// This is heuristic, but we guarantee that we don't do crazy things with label.
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

ThemeImage::CairoSurface pixBufToCairoSurface(GdkPixbuf *image) {
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
            guchar *end = p + (static_cast<ptrdiff_t>(3) * width);

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
            guchar *end = p + (static_cast<ptrdiff_t>(4) * width);
            guint t1;
            guint t2;
            guint t3;

#define MULT(d, c, a, t)                                                       \
    G_STMT_START {                                                             \
        (t) = ((c) * (a)) + 0x80;                                              \
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
    return ThemeImage::CairoSurface{surface};
}

ThemeImage::CairoSurface loadImage(UnixFD &file,
                                   const std::filesystem::path &path) {
    if (file.fd() < 0) {
        return nullptr;
    }
    if (path.extension() == ".png") {
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
        return ThemeImage::CairoSurface{surface};
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

    return pixBufToCairoSurface(image.get());
}

std::optional<std::pair<int, int>> svgSize(RsvgHandle *svg) {
    double w = 0;
    double h = 0;
    if (rsvg_handle_get_intrinsic_size_in_pixels(svg, &w, &h)) {
        int width = std::max(1, static_cast<int>(std::ceil(w)));
        int height = std::max(1, static_cast<int>(std::ceil(h)));
        return std::make_pair(width, height);
    }
    return std::nullopt;
}

std::optional<ThemeImage::Svg> loadSvg(UnixFD &file) {
    if (!file.isValid()) {
        return std::nullopt;
    }
    GObjectUniquePtr<GInputStream> stream(
        g_unix_input_stream_new(file.fd(), false));
    if (!stream) {
        return std::nullopt;
    }
    GObjectUniquePtr<RsvgHandle> handle(rsvg_handle_new_from_stream_sync(
        stream.get(), nullptr, RSVG_HANDLE_FLAGS_NONE, nullptr, nullptr));
    g_input_stream_close(stream.get(), nullptr, nullptr);
    if (!handle) {
        return std::nullopt;
    }
    auto size = svgSize(handle.get());
    if (!size) {
        return std::nullopt;
    }
    ThemeImage::Svg svg;
    svg.width = size->first;
    svg.height = size->second;
    svg.handle = std::move(handle);
    return svg;
}

bool isSvgPath(const std::filesystem::path &path) {
    return path.extension() == ".svg" || path.extension() == ".svgz";
}

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

        // Only put the common types and we make a preferred order.
        for (std::string ext : {"svg", "svgz", "png", "bmp", "xpm"}) {
            // png is supported by cairo.
            if (ext == "png" || exts.contains(ext)) {
                result.push_back("." + ext);
            }
        }
        CLASSICUI_DEBUG() << "Supported image extensions: " << result;
        return result;
    }();

    return formats;
}

constexpr double RoundEpsilon = 1e-3;

double pixelCeil(double f) { return std::ceil(f - RoundEpsilon); }
double pixelFloor(double f) { return std::floor(f + RoundEpsilon); }

void paintTile(cairo_t *c, int x, int y, int width, int height, double alpha,
               const ThemeImage &image, int marginLeft, int marginTop,
               int marginRight, int marginBottom) {

    int resizeHeight = image.height() - marginTop - marginBottom;
    int resizeWidth = image.width() - marginLeft - marginRight;

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

    double sourceX[] = {0.0, static_cast<double>(marginLeft),
                        static_cast<double>(image.width() - marginRight),
                        static_cast<double>(image.width())};
    double sourceY[] = {0.0, static_cast<double>(marginTop),
                        static_cast<double>(image.height() - marginBottom),
                        static_cast<double>(image.height())};

    double gridX[] = {0.0, static_cast<double>(marginLeft),
                      static_cast<double>(width - marginRight),
                      static_cast<double>(width)};
    double gridY[] = {0.0, static_cast<double>(marginTop),
                      static_cast<double>(height - marginBottom),
                      static_cast<double>(height)};
    for (double &gx : gridX) {
        gx += x;
    }
    for (double &gy : gridY) {
        gy += y;
    }

    auto *surface = cairo_get_target(c);
    double xScale;
    double yScale;
    cairo_surface_get_device_scale(surface, &xScale, &yScale);
    for (int i = 0; i < 4; i++) {
        if (i % 2 == 0) {
            gridX[i] = pixelFloor(gridX[i] * xScale) / xScale;
            gridY[i] = pixelFloor(gridY[i] * yScale) / yScale;
        } else {
            gridX[i] = pixelCeil(gridX[i] * xScale) / xScale;
            gridY[i] = pixelCeil(gridY[i] * yScale) / yScale;
        }
    }

    auto part = [&](int ix, int iy) {
        double sx = sourceX[ix];
        double sy = sourceY[iy];
        double sw = sourceX[ix + 1] - sourceX[ix];
        double sh = sourceY[iy + 1] - sourceY[iy];
        double dx = gridX[ix];
        double dy = gridY[iy];
        double dw = gridX[ix + 1] - gridX[ix];
        double dh = gridY[iy + 1] - gridY[iy];
        if (dw > 0 && dh > 0) {
            image.paintRegion(c, sx, sy, sw, sh, dx, dy, dw, dh, alpha);
        }
    };
    /*
     * 7 8 9
     * 4 5 6
     * 1 2 3
     */

    if (marginLeft && marginBottom) {
        /* part 1 */
        part(0, 2);
    }

    if (marginRight && marginBottom) {
        /* part 3 */
        part(2, 2);
    }

    if (marginLeft && marginTop) {
        /* part 7 */
        part(0, 0);
    }

    if (marginRight && marginTop) {
        /* part 9 */
        part(2, 0);
    }

    /* part 2 & 8 */
    if (marginTop && targetResizeWidth > 0) {
        part(1, 0);
    }

    if (marginBottom && targetResizeWidth > 0) {
        part(1, 2);
    }

    /* part 4 & 6 */
    if (marginLeft && targetResizeHeight > 0) {
        part(0, 1);
    }

    if (marginRight && targetResizeHeight > 0) {
        part(2, 1);
    }

    /* part 5 */
    if (targetResizeHeight > 0 && targetResizeWidth > 0) {
        part(1, 1);
    }
}

} // namespace

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
        std::filesystem::path iconPath =
            iconTheme.findIconPath(icon, size, 1, gdkPixbufSupportedFormats());
        auto fd = StandardPaths::openPath(iconPath);
        if (isSvgPath(iconPath)) {
            if (auto svg = loadSvg(fd)) {
                image_ = std::move(svg.value());
            }
        } else {
            CairoSurface image = loadImage(fd, iconPath);
            if (image) {
                image_ = std::move(image);
            }
        }
    }
    if (std::holds_alternative<std::monostate>(image_)) {
        CairoSurface textImage(
            cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size, size));
        drawTextIcon(textImage.get(), label, size, classicui->config());
        image_ = std::move(textImage);
    }
}

ThemeImage::ThemeImage(const Theme &theme, const BackgroundImageConfig &cfg,
                       const Color &color, const Color &borderColor) {
    if (!cfg.image->empty()) {
        std::filesystem::path imagePath;
        auto imageFile = StandardPaths::global().open(
            StandardPathsType::PkgData,
            std::filesystem::path("themes") / theme.name() / *cfg.image,
            theme.isSystemTheme() ? StandardPathsMode::System
                                  : StandardPathsMode::Default,
            &imagePath);
        if (isSvgPath(imagePath)) {
            if (auto svg = loadSvg(imageFile)) {
                image_ = std::move(svg.value());
            }
        } else {
            CairoSurface image = loadImage(imageFile, imagePath);
            if (image) {
                image_ = std::move(image);
            }
        }
    }

    if (!cfg.overlay->empty()) {
        std::filesystem::path imagePath;
        auto imageFile = StandardPaths::global().open(
            StandardPathsType::PkgData,
            std::filesystem::path("themes") / theme.name() / *cfg.overlay,
            theme.isSystemTheme() ? StandardPathsMode::System
                                  : StandardPathsMode::Default,
            &imagePath);
        if (isSvgPath(imagePath)) {
            if (auto svg = loadSvg(imageFile)) {
                overlay_ = std::move(svg.value());
            }
        } else if (auto overlay = loadImage(imageFile, imagePath)) {
            overlay_ = std::move(overlay);
        }
    }

    if (!valid()) {
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
        ThemeImage::Pattern pattern;
        pattern.width = width;
        pattern.height = height;
        pattern.borderWidth = borderWidth;
        pattern.pattern.reset(cairo_pattern_create_mesh());
        const auto setPatch = [&](double x0, double y0, double x1, double y1,
                                  const Color &color) {
            cairo_mesh_pattern_begin_patch(pattern.pattern.get());
            cairo_mesh_pattern_move_to(pattern.pattern.get(), x0, y0);
            cairo_mesh_pattern_line_to(pattern.pattern.get(), x1, y0);
            cairo_mesh_pattern_line_to(pattern.pattern.get(), x1, y1);
            cairo_mesh_pattern_line_to(pattern.pattern.get(), x0, y1);
            for (int i = 0; i < 4; i++) {
                cairo_mesh_pattern_set_corner_color_rgba(
                    pattern.pattern.get(), i, color.redF(), color.greenF(),
                    color.blueF(), color.alphaF());
            }
            cairo_mesh_pattern_end_patch(pattern.pattern.get());
        };
        if (borderWidth > 0) {
            const double x[] = {0.0, static_cast<double>(borderWidth),
                                static_cast<double>(width - borderWidth),
                                static_cast<double>(width)};
            const double y[] = {0.0, static_cast<double>(borderWidth),
                                static_cast<double>(height - borderWidth),
                                static_cast<double>(height)};
            for (int row = 0; row < 3; row++) {
                for (int column = 0; column < 3; column++) {
                    setPatch(x[column], y[row], x[column + 1], y[row + 1],
                             (row == 1 && column == 1) ? color : borderColor);
                }
            }
        } else {
            setPatch(0, 0, width, height, color);
        }
        image_ = std::move(pattern);
    }
}

ThemeImage::ThemeImage(const Theme &theme, const ActionImageConfig &cfg) {
    if (!cfg.image->empty()) {
        std::filesystem::path imagePath;
        auto imageFile = StandardPaths::global().open(
            StandardPathsType::PkgData,
            std::filesystem::path("themes") / theme.name() / *cfg.image,
            theme.isSystemTheme() ? StandardPathsMode::System
                                  : StandardPathsMode::Default,
            &imagePath);
        if (isSvgPath(imagePath)) {
            if (auto svg = loadSvg(imageFile)) {
                image_ = std::move(svg.value());
            }
        } else {
            auto image = loadImage(imageFile, imagePath);
            if (image) {
                image_ = std::move(image);
            }
        }
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
    cairo_translate(cr, ((size - rect.width) * 0.5) - rect.x,
                    ((size - rect.height) * 0.5) - rect.y);
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

void ThemeImage::paintRegion(cairo_t *c, double sourceX, double sourceY,
                             double sourceWidth, double sourceHeight,
                             double destX, double destY, double destWidth,
                             double destHeight, double alpha,
                             bool overlay) const {
    const auto &source = overlay ? overlay_ : image_;
    if (const auto *image = std::get_if<CairoSurface>(&source)) {
        cairo_save(c);
        cairo_rectangle(c, destX, destY, destWidth, destHeight);
        cairo_clip(c);
        cairo_translate(c, destX - (sourceX * destWidth / sourceWidth),
                        destY - (sourceY * destHeight / sourceHeight));
        cairo_scale(c, destWidth / sourceWidth, destHeight / sourceHeight);
        cairo_set_source_surface(c, image->get(), 0, 0);
        cairo_paint_with_alpha(c, alpha);
        cairo_restore(c);
        return;
    }
    if (const auto *svg = std::get_if<Svg>(&source)) {
        cairo_save(c);
        cairo_rectangle(c, destX, destY, destWidth, destHeight);
        cairo_clip(c);
        cairo_translate(c, destX - (sourceX * destWidth / sourceWidth),
                        destY - (sourceY * destHeight / sourceHeight));
        cairo_scale(c, destWidth / sourceWidth, destHeight / sourceHeight);
        RsvgRectangle viewport{0, 0, static_cast<double>(svg->width),
                               static_cast<double>(svg->height)};
        cairo_push_group(c);
        if (rsvg_handle_render_document(svg->handle.get(), c, &viewport,
                                        nullptr)) {
            cairo_pop_group_to_source(c);
            cairo_paint_with_alpha(c, alpha);
        } else {
            cairo_pop_group(c);
        }
        cairo_restore(c);
        return;
    }
    if (const auto *pattern = std::get_if<Pattern>(&source)) {
        cairo_save(c);
        cairo_rectangle(c, destX, destY, destWidth, destHeight);
        cairo_clip(c);
        cairo_translate(c, destX - (sourceX * destWidth / sourceWidth),
                        destY - (sourceY * destHeight / sourceHeight));
        cairo_scale(c, destWidth / sourceWidth, destHeight / sourceHeight);
        cairo_set_source(c, pattern->pattern.get());
        cairo_paint_with_alpha(c, alpha);
        cairo_restore(c);
    }
}

Theme::Theme() : iconTheme_(IconTheme::defaultIconThemeName()) {}

Theme::~Theme() {}

bool Theme::isSystemThemeName(std::string_view themeName) {
    return themeName == "default" || themeName == "default-dark";
}

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
        std::forward_as_tuple(*this, cfg, color, borderColor));
    assert(result.second);
    return result.first->second;
}

const ThemeImage &Theme::loadAction(const ActionImageConfig &cfg) {
    if (auto *image = findValue(actionImageTable_, &cfg)) {
        return *image;
    }

    auto result = actionImageTable_.emplace(std::piecewise_construct,
                                            std::forward_as_tuple(&cfg),
                                            std::forward_as_tuple(*this, cfg));
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

void Theme::paint(cairo_t *c, const BackgroundImageConfig &cfg, int dx, int dy,
                  int width, int height, double alpha) {
    const ThemeImage &image = loadBackground(cfg);
    auto marginTop = *cfg.margin->marginTop;
    auto marginBottom = *cfg.margin->marginBottom;
    auto marginLeft = *cfg.margin->marginLeft;
    auto marginRight = *cfg.margin->marginRight;

    paintTile(c, dx, dy, width, height, alpha, image, marginLeft, marginTop,
              marginRight, marginBottom);

    if (!image.hasOverlay()) {
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
        x = ((width - image.overlayWidth()) / 2) + *cfg.overlayOffsetX;
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
        y = ((height - image.overlayHeight()) / 2) + *cfg.overlayOffsetY;
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
    cairo_rectangle(c, finalRect.left(), finalRect.top(), finalRect.width(),
                    finalRect.height());
    cairo_clip(c);
    image.paintRegion(c, 0, 0, image.overlayWidth(), image.overlayHeight(), x,
                      y, image.overlayWidth(), image.overlayHeight(), alpha,
                      true);
    cairo_restore(c);
}

void Theme::paint(cairo_t *c, const ActionImageConfig &cfg, double alpha) {
    const ThemeImage &image = loadAction(cfg);
    int height = image.height();
    int width = image.width();

    image.paintRegion(c, 0, 0, image.width(), image.height(), 0, 0, width,
                      height, alpha);
}

void Theme::reset() {
    trayImageTable_.clear();
    backgroundImageTable_.clear();
    actionImageTable_.clear();
}

void Theme::load(std::string_view name) {
    reset();
    isSystemTheme_ = isSystemThemeName(name);
    ThemeConfig config;
    copyHelper(config);
    // Reset the default value to state.
    syncDefaultValueToCurrent();
    if (auto themeConfigFile = StandardPaths::global().open(
            StandardPathsType::PkgData,
            std::filesystem::path("themes") / name / "theme.conf",
            StandardPathsMode::System);
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
    if (!isSystemTheme_) {
        // For system theme, we don't load user file, as user file is only for
        // custom theme.
        if (auto themeConfigFile = StandardPaths::global().open(
                StandardPathsType::PkgData,
                std::filesystem::path("themes") / name / "theme.conf",
                StandardPathsMode::User);
            themeConfigFile.isValid()) {
            // Has user file, load user file data.
            RawConfig themeConfig;
            readFromIni(themeConfig, themeConfigFile.fd());
            Configuration::load(themeConfig, true);
        }
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
    isSystemTheme_ = isSystemThemeName(name);
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
    paint(c, cfg, 0, 0, width, height, 1);
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

    auto inputPanelCandidateLabelText = *inputPanel->candidateLabelColor;
    auto inputPanelHighlightCandidateLabelText =
        *inputPanel->highlightCandidateLabelColor;
    auto inputPanelCandidateCommentText = *inputPanel->candidateCommentColor;
    auto inputPanelHighlightCandidateCommentText =
        *inputPanel->highlightCandidateCommentColor;

    menuBackground_ = *menu->background->color;
    menuBorder_ = *menu->background->borderColor;
    menuSelectedItemBackground_ = *menu->highlight->color;
    menuSelectedItemBorder_ = *menu->highlight->borderColor;
    menuSeparator_ = *menu->separator->color;
    menuText_ = *menu->normalColor;
    menuSelectedItemText_ = *menu->highlightTextColor;

    if (accent) {
        auto [foreground, foregroundDim] = accentForeground(*accent);
        for (auto field : accentColorFields_) {
            switch (field) {
            case ColorField::InputPanel_Background:
                inputPanelBackground_ = *accent;
                inputPanelText_ = foreground;
                if (inputPanelCandidateLabelText) {
                    inputPanelCandidateLabelText = foreground;
                }
                if (inputPanelCandidateCommentText) {
                    inputPanelCandidateCommentText = foregroundDim;
                }
                break;
            case ColorField::InputPanel_Border:
                inputPanelBorder_ = *accent;
                break;
            case ColorField::InputPanel_HighlightCandidateBackground:
                inputPanelHighlightCandidateBackground_ = *accent;
                inputPanelHighlightCandidateText_ = foreground;
                if (inputPanelHighlightCandidateLabelText) {
                    inputPanelHighlightCandidateLabelText = foreground;
                }
                if (inputPanelHighlightCandidateCommentText) {
                    inputPanelHighlightCandidateCommentText = foregroundDim;
                }
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
    if (inputPanelCandidateLabelText) {
        inputPanelCandidateLabelText_ = *inputPanelCandidateLabelText;
    } else {
        inputPanelCandidateLabelText_ = inputPanelText_;
    }
    if (inputPanelHighlightCandidateLabelText) {
        inputPanelHighlightCandidateLabelText_ =
            *inputPanelHighlightCandidateLabelText;
    } else {
        inputPanelHighlightCandidateLabelText_ =
            inputPanelHighlightCandidateText_;
    }
    if (inputPanelCandidateCommentText) {
        inputPanelCandidateCommentText_ = *inputPanelCandidateCommentText;
    } else {
        inputPanelCandidateCommentText_ = inputPanelText_;
        inputPanelCandidateCommentText_.setAlphaF(
            inputPanelCandidateCommentText_.alphaF() * 0.6);
    }
    if (inputPanelHighlightCandidateCommentText) {
        inputPanelHighlightCandidateCommentText_ =
            *inputPanelHighlightCandidateCommentText;
    } else {
        inputPanelHighlightCandidateCommentText_ =
            inputPanelHighlightCandidateText_;
        inputPanelHighlightCandidateCommentText_.setAlphaF(
            inputPanelHighlightCandidateCommentText_.alphaF() * 0.6);
    }
}

} // namespace fcitx::classicui
