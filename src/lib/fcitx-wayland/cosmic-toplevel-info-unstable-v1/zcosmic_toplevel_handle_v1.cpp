#include "zcosmic_toplevel_handle_v1.h"
#include <cassert>
#include <wayland-client-core.h>
#include "wayland-cosmic-toplevel-info-unstable-v1-client-protocol.h"

namespace fcitx::wayland {
const struct zcosmic_toplevel_handle_v1_listener
    ZcosmicToplevelHandleV1::listener = {
#if defined(ZCOSMIC_TOPLEVEL_HANDLE_V1_CLOSED_SINCE_VERSION)
        .closed =
            [](void *data, zcosmic_toplevel_handle_v1 *wldata) {
                auto *obj = static_cast<ZcosmicToplevelHandleV1 *>(data);
                assert(*obj == wldata);
                {
                    obj->closed()();
                }
            },
#endif
#if defined(ZCOSMIC_TOPLEVEL_HANDLE_V1_DONE_SINCE_VERSION)
        .done =
            [](void *data, zcosmic_toplevel_handle_v1 *wldata) {
                auto *obj = static_cast<ZcosmicToplevelHandleV1 *>(data);
                assert(*obj == wldata);
                {
                    obj->done()();
                }
            },
#endif
#if defined(ZCOSMIC_TOPLEVEL_HANDLE_V1_TITLE_SINCE_VERSION)
        .title =
            [](void *data, zcosmic_toplevel_handle_v1 *wldata,
               const char *title) {
                auto *obj = static_cast<ZcosmicToplevelHandleV1 *>(data);
                assert(*obj == wldata);
                {
                    obj->title()(title);
                }
            },
#endif
#if defined(ZCOSMIC_TOPLEVEL_HANDLE_V1_APP_ID_SINCE_VERSION)
        .app_id =
            [](void *data, zcosmic_toplevel_handle_v1 *wldata,
               const char *appId) {
                auto *obj = static_cast<ZcosmicToplevelHandleV1 *>(data);
                assert(*obj == wldata);
                {
                    obj->appId()(appId);
                }
            },
#endif
#if defined(ZCOSMIC_TOPLEVEL_HANDLE_V1_OUTPUT_ENTER_SINCE_VERSION)
        .output_enter =
            [](void *data, zcosmic_toplevel_handle_v1 *wldata,
               wl_output *output) {
                auto *obj = static_cast<ZcosmicToplevelHandleV1 *>(data);
                assert(*obj == wldata);
                {
                    if (!output) {
                        return;
                    }
                    auto *output_ =
                        static_cast<WlOutput *>(wl_proxy_get_user_data(
                            reinterpret_cast<wl_proxy *>(output)));
                    obj->outputEnter()(output_);
                }
            },
#endif
#if defined(ZCOSMIC_TOPLEVEL_HANDLE_V1_OUTPUT_LEAVE_SINCE_VERSION)
        .output_leave =
            [](void *data, zcosmic_toplevel_handle_v1 *wldata,
               wl_output *output) {
                auto *obj = static_cast<ZcosmicToplevelHandleV1 *>(data);
                assert(*obj == wldata);
                {
                    if (!output) {
                        return;
                    }
                    auto *output_ =
                        static_cast<WlOutput *>(wl_proxy_get_user_data(
                            reinterpret_cast<wl_proxy *>(output)));
                    obj->outputLeave()(output_);
                }
            },
#endif
#if defined(ZCOSMIC_TOPLEVEL_HANDLE_V1_WORKSPACE_ENTER_SINCE_VERSION)
        .workspace_enter =
            [](void *data, zcosmic_toplevel_handle_v1 *wldata,
               zcosmic_workspace_handle_v1 *workspace) {
                auto *obj = static_cast<ZcosmicToplevelHandleV1 *>(data);
                assert(*obj == wldata);
                {
                    if (!workspace) {
                        return;
                    }
                    auto *workspace_ = static_cast<ZcosmicWorkspaceHandleV1 *>(
                        wl_proxy_get_user_data(
                            reinterpret_cast<wl_proxy *>(workspace)));
                    obj->workspaceEnter()(workspace_);
                }
            },
#endif
#if defined(ZCOSMIC_TOPLEVEL_HANDLE_V1_WORKSPACE_LEAVE_SINCE_VERSION)
        .workspace_leave =
            [](void *data, zcosmic_toplevel_handle_v1 *wldata,
               zcosmic_workspace_handle_v1 *workspace) {
                auto *obj = static_cast<ZcosmicToplevelHandleV1 *>(data);
                assert(*obj == wldata);
                {
                    if (!workspace) {
                        return;
                    }
                    auto *workspace_ = static_cast<ZcosmicWorkspaceHandleV1 *>(
                        wl_proxy_get_user_data(
                            reinterpret_cast<wl_proxy *>(workspace)));
                    obj->workspaceLeave()(workspace_);
                }
            },
#endif
#if defined(ZCOSMIC_TOPLEVEL_HANDLE_V1_STATE_SINCE_VERSION)
        .state =
            [](void *data, zcosmic_toplevel_handle_v1 *wldata,
               wl_array *state) {
                auto *obj = static_cast<ZcosmicToplevelHandleV1 *>(data);
                assert(*obj == wldata);
                {
                    obj->state()(state);
                }
            },
#endif
#if defined(ZCOSMIC_TOPLEVEL_HANDLE_V1_GEOMETRY_SINCE_VERSION)
        .geometry =
            [](void *data, zcosmic_toplevel_handle_v1 *wldata,
               wl_output *output, int32_t x, int32_t y, int32_t width,
               int32_t height) {
                auto *obj = static_cast<ZcosmicToplevelHandleV1 *>(data);
                assert(*obj == wldata);
                {
                    if (!output) {
                        return;
                    }
                    auto *output_ =
                        static_cast<WlOutput *>(wl_proxy_get_user_data(
                            reinterpret_cast<wl_proxy *>(output)));
                    obj->geometry()(output_, x, y, width, height);
                }
            },
#endif
#if defined(ZCOSMIC_TOPLEVEL_HANDLE_V1_EXT_WORKSPACE_ENTER_SINCE_VERSION)
        .ext_workspace_enter =
            [](void *data, zcosmic_toplevel_handle_v1 *wldata,
               ext_workspace_handle_v1 *workspace) {
                auto *obj = static_cast<ZcosmicToplevelHandleV1 *>(data);
                assert(*obj == wldata);
                {
                    if (!workspace) {
                        return;
                    }
                    auto *workspace_ = static_cast<ExtWorkspaceHandleV1 *>(
                        wl_proxy_get_user_data(
                            reinterpret_cast<wl_proxy *>(workspace)));
                    obj->extWorkspaceEnter()(workspace_);
                }
            },
#endif
#if defined(ZCOSMIC_TOPLEVEL_HANDLE_V1_EXT_WORKSPACE_LEAVE_SINCE_VERSION)
        .ext_workspace_leave =
            [](void *data, zcosmic_toplevel_handle_v1 *wldata,
               ext_workspace_handle_v1 *workspace) {
                auto *obj = static_cast<ZcosmicToplevelHandleV1 *>(data);
                assert(*obj == wldata);
                {
                    if (!workspace) {
                        return;
                    }
                    auto *workspace_ = static_cast<ExtWorkspaceHandleV1 *>(
                        wl_proxy_get_user_data(
                            reinterpret_cast<wl_proxy *>(workspace)));
                    obj->extWorkspaceLeave()(workspace_);
                }
            },
#endif
};

ZcosmicToplevelHandleV1::ZcosmicToplevelHandleV1(
    zcosmic_toplevel_handle_v1 *data)
    : version_(zcosmic_toplevel_handle_v1_get_version(data)), data_(data) {
    zcosmic_toplevel_handle_v1_set_user_data(*this, this);
    zcosmic_toplevel_handle_v1_add_listener(
        *this, &ZcosmicToplevelHandleV1::listener, this);
}

void ZcosmicToplevelHandleV1::destructor(zcosmic_toplevel_handle_v1 *data) {
    zcosmic_toplevel_handle_v1_destroy(data);
}

} // namespace fcitx::wayland
