/*
 * SPDX-FileCopyrightText: 2026 John Xu <JohnXu22786@users.noreply.github.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef _FCITX_UI_CLASSIC_CANDIDATEMENU_H_
#define _FCITX_UI_CLASSIC_CANDIDATEMENU_H_

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>
#include <cairo.h>
#include <pango/pango-context.h>
#include <pango/pango-fontmap.h>
#include <pango/pango-layout.h>
#include "fcitx-utils/rect.h"
#include "fcitx-utils/trackableobject.h"
#include "fcitx/candidatelist.h"
#include "candidatemenulayout.h"
#include "common.h"

namespace fcitx {
class InputContext;
}

namespace fcitx::classicui {

class ClassicUI;

// The candidate actions, Yoga layout, hit testing and painting are shared by
// the XCB and Wayland popup windows. The windows only own native resources.
class CandidateMenu {
public:
    struct Selection {
        TrackableObjectReference<InputContext> inputContext;
        std::shared_ptr<CandidateList> candidateList;
        const CandidateWord *candidate;
        size_t candidateIndex;
        int id;

        void activate() const;
    };

    explicit CandidateMenu(ClassicUI *parent);
    void setFontDPI(int dpi);
    PangoContext *fontContext() const { return context_.get(); }
    bool show(InputContext *inputContext,
              const std::vector<Rect> &candidateRegions, int x, int y);
    void clear();
    bool hover(int x, int y);
    std::optional<Selection> selectionAt(int x, int y) const;
    void paint(cairo_t *cr);

    bool visible() const { return visible_; }
    int width() const { return layoutGeometry_.width(); }
    int height() const { return layoutGeometry_.height(); }
    const Rect &anchor() const { return anchor_; }

private:
    Rect highlightRegion(const Rect &region) const;

    ClassicUI *parent_;
    GObjectUniquePtr<PangoFontMap> fontMap_;
    double fontMapDefaultDPI_ = 96.0;
    GObjectUniquePtr<PangoContext> context_;
    GObjectUniquePtr<PangoLayout> layout_;
    CandidateMenuLayout layoutGeometry_;
    TrackableObjectReference<InputContext> inputContext_;
    std::shared_ptr<CandidateList> candidateList_;
    const CandidateWord *candidate_ = nullptr;
    size_t candidateIndex_ = 0;
    std::vector<CandidateAction> actions_;
    Rect anchor_;
    int hoveredIndex_ = -1;
    bool visible_ = false;
};

} // namespace fcitx::classicui

#endif // _FCITX_UI_CLASSIC_CANDIDATEMENU_H_
