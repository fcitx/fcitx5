#include "waylandimserverbase.h"
#include <wayland-client-core.h>
#include "fcitx-utils/event.h"
#include "waylandim.h"

namespace fcitx {

WaylandIMServerBase::WaylandIMServerBase(wl_display *display, FocusGroup *group,
                                         const std::string &name,
                                         WaylandIMModule *waylandim)
    : group_(group), name_(name), parent_(waylandim),
      display_(
          static_cast<wayland::Display *>(wl_display_get_user_data(display))) {}

void WaylandIMServerBase::deferredFlush() {
    if (deferEvent_) {
        return;
    }

    deferEvent_ =
        parent_->instance()->eventLoop().addDeferEvent([this](EventSource *) {
            display_->flush();
            deferEvent_.reset();
            return true;
        });
}

} // namespace fcitx