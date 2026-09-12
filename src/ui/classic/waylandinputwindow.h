/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UI_CLASSIC_WAYLANDINPUTWINDOW_H_
#define _FCITX_UI_CLASSIC_WAYLANDINPUTWINDOW_H_

#include <cstddef>
#include <memory>
#include <vector>
#include <wayland-util.h>
#include "fcitx-utils/trackableobject.h"
#include "fcitx/inputcontext.h"
#include "ext_background_effect_manager_v1.h"
#include "ext_background_effect_surface_v1.h"
#include "inputwindow.h"
#include "zwp_input_panel_surface_v1.h"
#include "zwp_input_popup_surface_v2.h"

namespace fcitx::classicui {

class WaylandUI;
class WaylandWindow;

class WaylandInputWindow : public InputWindow {
public:
    WaylandInputWindow(WaylandUI *ui);

    void initPanel();
    void resetPanel();
    void update(InputContext *ic);
    void repaint();
    void
    setBlurManager(std::shared_ptr<wayland::ExtBackgroundEffectManagerV1> blur);
    void updateScale();

private:
    /// Build and show the candidate action menu at the pointer position.
    void showCandidateMenu(int x, int y);
    /// Hide the candidate action menu and discard its temporary state.
    void clearCandidateMenu();
    /// Restore the input panel surface after a candidate menu closes.
    void restorePanelSize();
    /// Update the menu item under the pointer.
    bool hoverCandidateMenu(int x, int y);
    /// Activate or dismiss the menu in response to a left click.
    void clickCandidateMenu(int x, int y);
    /// Paint the candidate action menu over the input panel.
    void paintCandidateMenu(cairo_t *cr);
    void updateBlur();

    WaylandUI *ui_;
    wl_fixed_t scroll_ = 0;
    GObjectUniquePtr<PangoContext> menuContext_;
    GObjectUniquePtr<PangoLayout> menuLayout_;
    std::unique_ptr<wayland::ZwpInputPanelSurfaceV1> panelSurface_;
    TrackableObjectReference<InputContext> v2IC_;
    std::unique_ptr<wayland::ZwpInputPopupSurfaceV2> panelSurfaceV2_;
    std::unique_ptr<WaylandWindow> window_;
    TrackableObjectReference<InputContext> repaintIC_;
    std::shared_ptr<wayland::ExtBackgroundEffectManagerV1> blurManager_;
    std::unique_ptr<wayland::ExtBackgroundEffectSurfaceV1> blur_;

    std::shared_ptr<CandidateList> candidateMenuList_;
    const CandidateWord *candidateMenuCandidate_ = nullptr;
    std::vector<CandidateAction> candidateMenuActions_;
    std::vector<Rect> candidateMenuRegions_;
    Rect candidateMenuRegion_;
    int candidateMenuX_ = 0;
    int candidateMenuY_ = 0;
    int candidateMenuWidth_ = 0;
    int candidateMenuHeight_ = 0;
    int candidateMenuItemWidth_ = 0;
    int candidateMenuItemHeight_ = 0;
    bool candidateMenuHasCheckable_ = false;
    int candidateMenuHoveredIndex_ = -1;
    int panelWidth_ = 0;
    int panelHeight_ = 0;
    bool candidateMenuVisible_ = false;
};

} // namespace fcitx::classicui

#endif // _FCITX_UI_CLASSIC_WAYLANDINPUTWINDOW_H_
