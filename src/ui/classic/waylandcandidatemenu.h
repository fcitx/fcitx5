/*
 * SPDX-FileCopyrightText: 2026 John Xu <JohnXu22786@users.noreply.github.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef _FCITX_UI_CLASSIC_WAYLANDCANDIDATEMENU_H_
#define _FCITX_UI_CLASSIC_WAYLANDCANDIDATEMENU_H_

#include <memory>
#include <optional>
#include <vector>
#include <cairo.h>
#include <pango/pango-context.h>
#include <pango/pango-fontmap.h>
#include <pango/pango-layout.h>
#include "fcitx-utils/rect.h"
#include "fcitx/candidatelist.h"
#include "common.h"
#include "ext_background_effect_manager_v1.h"
#include "ext_background_effect_surface_v1.h"
#include "wl_subsurface.h"

namespace fcitx {
class InputContext;
}

namespace fcitx::classicui {

class WaylandUI;
class WaylandWindow;

class WaylandCandidateMenu {
public:
    WaylandCandidateMenu(WaylandUI *ui, WaylandWindow *parentWindow);

    void show(InputContext *inputContext,
              const std::vector<Rect> &candidateRegions, int x, int y,
              const std::optional<Rect> &textInputRectangle);
    void clear();
    void position(const std::optional<Rect> &textInputRectangle);
    void resetSubsurface();
    void createWindow();
    void destroyWindow();
    void updateScale();
    void setFontDPI(int dpi);
    void setBlurManager(
        std::shared_ptr<wayland::ExtBackgroundEffectManagerV1> blur);

private:
    bool createSubsurface();
    bool hover(int x, int y);
    void click(int x, int y);
    Rect highlightRegion(const Rect &region) const;
    void updateBlur();
    void paint(cairo_t *cr);
    void repaint();

    WaylandUI *ui_;
    WaylandWindow *parentWindow_;
    std::unique_ptr<WaylandWindow> window_;
    std::unique_ptr<wayland::WlSubsurface> subsurface_;
    std::shared_ptr<wayland::ExtBackgroundEffectManagerV1> blurManager_;
    std::unique_ptr<wayland::ExtBackgroundEffectSurfaceV1> blur_;
    GObjectUniquePtr<PangoFontMap> fontMap_;
    double fontMapDefaultDPI_ = 96.0;
    GObjectUniquePtr<PangoContext> context_;
    GObjectUniquePtr<PangoLayout> layout_;

    std::shared_ptr<CandidateList> candidateList_;
    const CandidateWord *candidate_ = nullptr;
    std::vector<CandidateAction> actions_;
    std::vector<Rect> regions_;
    Rect anchor_;
    int width_ = 0;
    int height_ = 0;
    int itemWidth_ = 0;
    int itemHeight_ = 0;
    bool hasCheckable_ = false;
    int hoveredIndex_ = -1;
    bool visible_ = false;
};

} // namespace fcitx::classicui

#endif // _FCITX_UI_CLASSIC_WAYLANDCANDIDATEMENU_H_
