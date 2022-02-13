/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "classicui.h"
#include <fcntl.h>
#include "fcitx-config/iniparser.h"
#include "fcitx-utils/misc_p.h"
#include "fcitx-utils/standardpath.h"
#include "fcitx-utils/utf8.h"
#include "fcitx/inputcontext.h"
#include "fcitx/inputcontextmanager.h"
#include "fcitx/instance.h"
#include "fcitx/userinterfacemanager.h"
#include "common.h"
#include "notificationitem_public.h"
#ifdef ENABLE_X11
#include "xcbui.h"
#endif
#ifdef WAYLAND_FOUND
#include "waylandui.h"
#endif

namespace fcitx::classicui {

FCITX_DEFINE_LOG_CATEGORY(classicui_logcategory, "classicui");

ClassicUI::ClassicUI(Instance *instance) : instance_(instance) {
    reloadConfig();

#ifdef ENABLE_X11
    if (auto *xcbAddon = xcb()) {
        xcbCreatedCallback_ =
            xcbAddon->call<IXCBModule::addConnectionCreatedCallback>(
                [this](const std::string &name, xcb_connection_t *conn,
                       int screen, FocusGroup *) {
                    uis_["x11:" + name].reset(
                        new XCBUI(this, name, conn, screen));
                });
        xcbClosedCallback_ =
            xcbAddon->call<IXCBModule::addConnectionClosedCallback>(
                [this](const std::string &name, xcb_connection_t *) {
                    uis_.erase("x11:" + name);
                });
    }
#endif

#ifdef WAYLAND_FOUND
    if (auto *waylandAddon = wayland()) {
        waylandCreatedCallback_ =
            waylandAddon->call<IWaylandModule::addConnectionCreatedCallback>(
                [this](const std::string &name, wl_display *display,
                       FocusGroup *) {
                    try {
                        uis_["wayland:" + name].reset(
                            new WaylandUI(this, name, display));
                    } catch (const std::runtime_error &) {
                    }
                });
        waylandClosedCallback_ =
            waylandAddon->call<IWaylandModule::addConnectionClosedCallback>(
                [this](const std::string &name, wl_display *) {
                    uis_.erase("wayland:" + name);
                });
    }
#endif
}

ClassicUI::~ClassicUI() {}

void ClassicUI::reloadConfig() {
    readAsIni(config_, "conf/classicui.conf");
    reloadTheme();
}

void ClassicUI::reloadTheme() { theme_.load(*config_.theme); }

void ClassicUI::suspend() {
    suspended_ = true;
    for (auto &p : uis_) {
        p.second->suspend();
    }

    if (auto *sni = notificationitem()) {
        sni->call<INotificationItem::disable>();
    }
    eventHandlers_.clear();
}

const Configuration *ClassicUI::getConfig() const {
    std::set<std::string> themeDirs;
    StandardPath::global().scanFiles(
        StandardPath::Type::PkgData, "themes",
        [&themeDirs](const std::string &path, const std::string &dir, bool) {
            if (fs::isdir(stringutils::joinPath(dir, path))) {
                themeDirs.insert(path);
            }
            return true;
        });
    std::map<std::string, std::string> themes;
    for (const auto &themeName : themeDirs) {
        auto file = StandardPath::global().open(
            StandardPath::Type::PkgData,
            stringutils::joinPath("themes", themeName, "theme.conf"), O_RDONLY);
        if (file.fd() < 0) {
            continue;
        }
        RawConfig config;
        readFromIni(config, file.fd());

        ThemeConfig themeConfig;
        themeConfig.load(config);
        if (!themeConfig.metadata.value()
                 .name.value()
                 .defaultString()
                 .empty()) {
            themes[themeName] =
                themeConfig.metadata.value().name.value().match();
        }
    }
    config_.theme.annotation().setThemes({themes.begin(), themes.end()});
    return &config_;
}

UIInterface *ClassicUI::uiForEvent(Event &event) {
    if (suspended_) {
        return nullptr;
    }

    if (!event.isInputContextEvent()) {
        return nullptr;
    }

    auto &icEvent = static_cast<InputContextEvent &>(event);
    return uiForInputContext(icEvent.inputContext());
}

UIInterface *ClassicUI::uiForInputContext(InputContext *inputContext) {
    if (suspended_ || !inputContext) {
        return nullptr;
    }
    if (!inputContext->hasFocus()) {
        return nullptr;
    }
    return uiForDisplay(inputContext->display());
}

UIInterface *ClassicUI::uiForDisplay(const std::string &display) {
    auto iter = uis_.find(display);
    if (iter == uis_.end()) {
        return nullptr;
    }
    return iter->second.get();
}

void ClassicUI::resume() {
    suspended_ = false;
    for (auto &p : uis_) {
        p.second->resume();
    }

    if (auto *sni = notificationitem()) {
        if (!sniHandler_) {
            sniHandler_ =
                sni->call<INotificationItem::watch>([this](bool enable) {
                    for (auto &p : uis_) {
                        p.second->setEnableTray(!enable);
                    }
                });
        }
        sni->call<INotificationItem::enable>();
        auto registered = sni->call<INotificationItem::registered>();
        for (auto &p : uis_) {
            p.second->setEnableTray(!registered);
        }
    } else {
        for (auto &p : uis_) {
            p.second->setEnableTray(true);
        }
    }

    eventHandlers_.emplace_back(instance_->watchEvent(
        EventType::InputContextCursorRectChanged, EventWatcherPhase::Default,
        [this](Event &event) {
            if (auto *ui = uiForEvent(event)) {
                auto &icEvent = static_cast<InputContextEvent &>(event);
                ui->updateCursor(icEvent.inputContext());
            }
        }));
    eventHandlers_.emplace_back(instance_->watchEvent(
        EventType::InputContextFocusIn, EventWatcherPhase::Default,
        [this](Event &event) {
            if (auto *ui = uiForEvent(event)) {
                auto &icEvent = static_cast<InputContextEvent &>(event);
                ui->updateCursor(icEvent.inputContext());
                ui->updateCurrentInputMethod(icEvent.inputContext());
            }
        }));
    eventHandlers_.emplace_back(instance_->watchEvent(
        EventType::InputContextSwitchInputMethod, EventWatcherPhase::Default,
        [this](Event &event) {
            if (auto *ui = uiForEvent(event)) {
                auto &icEvent = static_cast<InputContextEvent &>(event);
                ui->updateCurrentInputMethod(icEvent.inputContext());
            }
        }));
    eventHandlers_.emplace_back(instance_->watchEvent(
        EventType::InputMethodGroupChanged, EventWatcherPhase::Default,
        [this](Event &) {
            instance_->inputContextManager().foreachFocused(
                [this](InputContext *ic) {
                    if (auto *ui = uiForInputContext(ic)) {
                        ui->updateCurrentInputMethod(ic);
                    }
                    return true;
                });
        }));
    eventHandlers_.emplace_back(instance_->watchEvent(
        EventType::FocusGroupFocusChanged, EventWatcherPhase::Default,
        [this](Event &event) {
            auto &focusEvent =
                static_cast<FocusGroupFocusChangedEvent &>(event);
            if (!focusEvent.newFocus()) {
                if (auto ui = uiForDisplay(focusEvent.group()->display())) {
                    ui->update(UserInterfaceComponent::InputPanel, nullptr);
                }
            }
        }));
}

void ClassicUI::update(UserInterfaceComponent component,
                       InputContext *inputContext) {
    UIInterface *ui = nullptr;
    if (stringutils::startsWith(inputContext->display(), "wayland:") &&
        !stringutils::startsWith(inputContext->frontend(), "wayland")) {
        // If display is wayland, but frontend is not, then we can only do X11
        // for now, though position is wrong. We don't know which is xwayland
        // unfortunately, hopefully main display is X wayland.
        // The position will be wrong anyway.
#ifdef ENABLE_X11
        auto mainX11Display = xcb()->call<IXCBModule::mainDisplay>();
        if (!mainX11Display.empty()) {
            if (auto *uiPtr = findValue(uis_, "x11:" + mainX11Display)) {
                ui = uiPtr->get();
            }
        }
#endif
    } else {
        if (auto *uiPtr = findValue(uis_, inputContext->display())) {
            ui = uiPtr->get();
        }
    }

    if (ui) {
        ui->update(component, inputContext);
        if (component == UserInterfaceComponent::StatusArea) {
            ui->updateCurrentInputMethod(inputContext);
        }
    }
}

class ClassicUIFactory : public AddonFactory {
public:
    AddonInstance *create(AddonManager *manager) override {
        return new ClassicUI(manager->instance());
    }
};

const fcitx::Configuration *
ClassicUI::getSubConfig(const std::string &path) const {
    if (!stringutils::startsWith(path, "theme/")) {
        return nullptr;
    }

    auto name = path.substr(6);
    if (name.empty()) {
        return nullptr;
    }

    if (name == *config_.theme) {
        return &theme_;
    }

    subconfigTheme_.load(name);
    return &subconfigTheme_;
}

void ClassicUI::setSubConfig(const std::string &path,
                             const fcitx::RawConfig &config) {
    if (!stringutils::startsWith(path, "theme/")) {
        return;
    }
    auto name = path.substr(6);
    if (name.empty()) {
        return;
    }

    auto &theme = name == *config_.theme ? theme_ : subconfigTheme_;
    if (&theme == &subconfigTheme_) {
        // Fill the system value.
        getSubConfig(path);
    }
    theme.load(name, config);
    safeSaveAsIni(theme, StandardPath::Type::PkgData,
                  stringutils::joinPath("themes", name, "theme.conf"));
}

std::vector<unsigned char> ClassicUI::labelIcon(const std::string &label,
                                                unsigned int size) {
    std::vector<unsigned char> data;
    auto stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, size);
    data.resize(stride * size);
    UniqueCPtr<cairo_surface_t, cairo_surface_destroy> image;
    image.reset(cairo_image_surface_create_for_data(
        data.data(), CAIRO_FORMAT_ARGB32, size, size, stride));
    ThemeImage::drawTextIcon(image.get(), label, size, config_);
    image.reset();
    return data;
}

bool ClassicUI::preferTextIcon() const { return *config_.preferTextIcon; }

bool ClassicUI::showLayoutNameInIcon() const {
    return *config_.showLayoutNameInIcon;
}

} // namespace fcitx::classicui

FCITX_ADDON_FACTORY(fcitx::classicui::ClassicUIFactory);
