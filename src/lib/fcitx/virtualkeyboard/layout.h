/*
 * SPDX-FileCopyrightText: 2021~2021 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_VIRTUALKEYBOARD_LAYOUT_H_
#define _FCITX_VIRTUALKEYBOARD_LAYOUT_H_

#include <memory>
#include <utility>
#include <vector>
#include <fcitx-config/enum.h>
#include <fcitx-config/rawconfig.h>
#include <fcitx-utils/element.h>
#include <fcitx-utils/macros.h>
#include "fcitxcore_export.h"

namespace fcitx::virtualkeyboard {

FCITX_CONFIG_ENUM(LayoutOrientation, Vertical, Horizontal);

FCITX_CONFIG_ENUM(LayoutAlignment, Begin, Center, End, );

class LayoutPrivate;
class ButtonPrivate;
class Button;

class FCITXCORE_EXPORT LayoutItem {
public:
    virtual ~LayoutItem() = default;
    virtual void writeToRawConfig(RawConfig &config) const = 0;
};

class FCITXCORE_EXPORT Layout : public LayoutItem {
public:
    Layout();
    ~Layout();
    FCITX_DECLARE_PROPERTY(LayoutOrientation, orientation, setOrientation);
    FCITX_DECLARE_PROPERTY(LayoutAlignment, alignment, setAlignment);

    Layout *addLayout(float size);
    Button *addButton(float size, uint32_t id);
    const std::vector<std::unique_ptr<LayoutItem>> &items() const;

    static std::unique_ptr<Layout> standard26Key();

    void writeToRawConfig(RawConfig &config) const override;

private:
    FCITX_DECLARE_PRIVATE(Layout);
    std::unique_ptr<LayoutPrivate> d_ptr;
};

class FCITXCORE_EXPORT Button : public LayoutItem {
public:
    static constexpr uint32_t SPACER_ID = 0;

    Button(uint32_t id = 0);
    ~Button();

    uint32_t id() const;
    bool isSpacer() const;

    void writeToRawConfig(RawConfig &config) const override;

private:
    FCITX_DECLARE_PRIVATE(Button);
    std::unique_ptr<ButtonPrivate> d_ptr;
};

FCITX_CONFIG_ENUM(SpecialAction, None, LanguageSwitch, EnterKey, LayoutSwitch);

class ButtonMetadataPrivate;
class FCITXCORE_EXPORT ButtonMetadata {
public:
    using LongPressMetadata = std::pair<std::string, uint32_t>;

    ButtonMetadata();
    ButtonMetadata(std::string text, uint32_t code,
                   std::vector<LongPressMetadata> longpress = {});

    FCITX_INLINE_DEFINE_DEFAULT_DTOR_AND_MOVE(ButtonMetadata);
    FCITX_DECLARE_PROPERTY(std::string, text, setText);
    FCITX_DECLARE_PROPERTY(uint32_t, code, setCode);
    FCITX_DECLARE_PROPERTY(std::vector<LongPressMetadata>, longPress,
                           setLongPress);
    FCITX_DECLARE_PROPERTY(bool, visible, setVisible);
    SpecialAction specialAction() const;
    void setLanguageSwitch();
    void setEnterKey();
    void setLayoutSwitch(uint32_t layout);

    void writeToRawConfig(RawConfig &config) const;

private:
    FCITX_DECLARE_PRIVATE(ButtonMetadata);
    std::unique_ptr<ButtonMetadataPrivate> d_ptr;
};

class KeymapPrivate;
class FCITXCORE_EXPORT Keymap {
public:
    Keymap();
    FCITX_DECLARE_VIRTUAL_DTOR_MOVE(Keymap);

    ButtonMetadata &setKey(uint32_t id, ButtonMetadata metadata);
    void clear();
    void removeKey(uint32_t id);

    static Keymap qwerty();

    void writeToRawConfig(RawConfig &config) const;

private:
    FCITX_DECLARE_PRIVATE(Keymap);
    std::unique_ptr<KeymapPrivate> d_ptr;
};

class VirtualKeyboardPrivate;

class FCITXCORE_EXPORT VirtualKeyboard {
public:
    VirtualKeyboard();
    ~VirtualKeyboard();
    Keymap *addKeymap(std::string layout);

    void writeToRawConfig(RawConfig &config) const;

private:
    FCITX_DECLARE_PRIVATE(VirtualKeyboard);
    std::unique_ptr<VirtualKeyboardPrivate> d_ptr;
};

}; // namespace fcitx::virtualkeyboard

#endif // _FCITX_VIRTUALKEYBOARD_LAYOUT_H_
