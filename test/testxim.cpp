
/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include <condition_variable>
#include <cstdarg>
#include <future>
#include <mutex>
#include <xcb-imdkit/encoding.h>
#include <xcb-imdkit/imclient.h>
#include <xcb/xcb_aux.h>
#include "fcitx-utils/event.h"
#include "fcitx-utils/eventdispatcher.h"
#include "fcitx-utils/testing.h"
#include "fcitx/addonmanager.h"
#include "fcitx/instance.h"
#include "testdir.h"

using namespace fcitx;
constexpr char xmodifiers[] = "@im=testxim";
constexpr char commitText[] = "hello world你好世界켐ㅇㄹ貴方元気？☺";

class XIMTest {

public:
    XIMTest(EventDispatcher *dispatcher, Instance *instance)
        : dispatcher_(dispatcher), instance_(instance) {}

    static void run(XIMTest *self) { self->scheduleEvent(); }

    static void open_callback(xcb_xim_t *, void *user_data) {
        static_cast<XIMTest *>(user_data)->openCallback();
    }

    static void create_ic_callback(xcb_xim_t *, xcb_xic_t ic, void *user_data) {
        static_cast<XIMTest *>(user_data)->createICCallback(ic);
    }

    static void commit_string_callback(xcb_xim_t *, xcb_xic_t ic, uint32_t,
                                       char *str, uint32_t length, uint32_t *,
                                       size_t, void *user_data) {
        static_cast<XIMTest *>(user_data)->commitString(ic, str, length);
    }

    void openCallback() {
        w_ = xcb_generate_id(connection.get());
        xcb_create_window(
            connection.get(), XCB_COPY_FROM_PARENT, w_, screen_->root, 0, 0, 1,
            1, 1, XCB_WINDOW_CLASS_INPUT_OUTPUT, screen_->root_visual, 0, NULL);
        uint32_t input_style = XCB_IM_PreeditPosition | XCB_IM_StatusArea;
        xcb_point_t spot;
        spot.x = 5;
        spot.y = 10;
        xcb_rectangle_t area;
        area.x = 0;
        area.y = 0;
        area.width = 5;
        area.height = 10;
        xcb_xim_nested_list nested =
            xcb_xim_create_nested_list(im.get(), XCB_XIM_XNSpotLocation, &spot,
                                       XCB_XIM_XNArea, &area, NULL);
        xcb_xim_create_ic(im.get(), create_ic_callback, this,
                          XCB_XIM_XNInputStyle, &input_style,
                          XCB_XIM_XNClientWindow, &w_, XCB_XIM_XNFocusWindow,
                          &w_, XCB_XIM_XNPreeditAttributes, &nested, NULL);
        free(nested.data);
    }

    void createICCallback(xcb_xic_t ic) {
        ic_ = ic;
        dispatcher_->schedule([this] {
            bool found = false;
            instance_->inputContextManager().foreach(
                [&found](InputContext *ic) {
                    if (ic->frontendName() == "xim") {
                        ic->commitString(commitText);
                        found = true;
                    }
                    return true;
                });
            FCITX_ASSERT(found);
        });
    }

    void commitString(xcb_xic_t ic, const char *text, size_t length) {
        FCITX_ASSERT(ic == ic_);
        UniqueCPtr<char> result{
            xcb_compound_text_to_utf8(text, length, nullptr)};
        FCITX_ASSERT(result.get() == std::string_view(commitText))
            << "commit string: " << result.get() << " " << commitText;
        end = true;
    }

    static void logger(const char *fmt, ...) {
        va_list argp;
        va_start(argp, fmt);
        vprintf(fmt, argp);
        va_end(argp);
    }

    void scheduleEvent() {
        dispatcher_->schedule([this]() {
            auto *xim = instance_->addonManager().addon("xim", true);
            FCITX_ASSERT(xim);
            std::lock_guard<std::mutex> lck(mtx);
            started = true;
            cv.notify_all();
        });

        std::unique_lock<std::mutex> lck(mtx);
        cv.wait_for(lck, std::chrono::seconds(10), [this] { return started; });
        /* Open the connection to the X server */
        int screen_default_nbr;
        connection.reset(xcb_connect(NULL, &screen_default_nbr));
        screen_ = xcb_aux_get_screen(connection.get(), screen_default_nbr);

        if (!screen_) {
            return;
        }
        im.reset(
            xcb_xim_create(connection.get(), screen_default_nbr, xmodifiers));

        xcb_xim_im_callback callback{};
        callback.commit_string = commit_string_callback;
        xcb_xim_set_im_callback(im.get(), &callback, this);
        xcb_xim_set_log_handler(im.get(), logger);
        assert(xcb_xim_open(im.get(), open_callback, true, this));

        xcb_generic_event_t *event;
        while ((event = xcb_wait_for_event(connection.get()))) {
            xcb_xim_filter_event(im.get(), event);
            free(event);
            if (end) {
                break;
            }
        }

        xcb_xim_close(im.get());
        dispatcher_->schedule([this]() { instance_->exit(); });
    }

private:
    EventDispatcher *dispatcher_;
    Instance *instance_;
    UniqueCPtr<xcb_connection_t, xcb_disconnect> connection;
    UniqueCPtr<xcb_xim_t, xcb_xim_destroy> im;
    xcb_screen_t *screen_ = nullptr;
    xcb_window_t w_ = XCB_NONE;
    xcb_xic_t ic_ = XCB_NONE;
    std::condition_variable cv;
    std::mutex mtx;
    bool end = false;
    bool started = false;
};

int main() {
    setenv("XMODIFIERS", xmodifiers, 1);
    setupTestingEnvironment(
        FCITX5_BINARY_DIR,
        {"src/modules/quickphrase", "src/frontend/xim", "src/modules/xcb",
         "testing/testui", "testing/testim"},
        {"test", "src/modules", FCITX5_SOURCE_DIR "/test/addon/fcitx5"});

    char arg0[] = "testquickphrase";
    char arg1[] = "--disable=all";
    char arg2[] = "--enable=testim,testfrontend,xim,xcb,testui";
    char *argv[] = {arg0, arg1, arg2};
    Instance instance(FCITX_ARRAY_SIZE(argv), argv);
    instance.addonManager().registerDefaultLoader(nullptr);
    EventDispatcher dispatcher;
    dispatcher.attach(&instance.eventLoop());
    XIMTest test(&dispatcher, &instance);
    std::thread thread(XIMTest::run, &test);
    auto watchDog = instance.eventLoop().addTimeEvent(
        CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 20 * 1000 * 1000, 0,
        [](EventSourceTime *, uint64_t) {
            std::abort();
            return 0;
        });
    instance.exec();
    watchDog.reset();
    thread.join();
    return 0;
}
