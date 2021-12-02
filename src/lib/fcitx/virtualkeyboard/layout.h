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
#include <fcitx-utils/element.h>
#include <fcitx-utils/macros.h>

namespace fcitx::virtualkeyboard {

enum class LayoutDirection { Vertical, Horizontal };

enum class LayoutAlignment {
    Begin,
    Center,
    End,
};

class LayoutPrivate;
class ButtonPrivate;
class Button;

class LayoutItem : public Element {};

class Layout : public LayoutItem {
public:
    Layout();
    ~Layout();
    FCITX_DECLARE_PROPERTY(LayoutDirection, direction, setDirection);
    FCITX_DECLARE_PROPERTY(LayoutAlignment, alignment, setAlignment);

    Layout *addLayout(float size);
    Button *addButton(float size, uint32_t id);
    std::vector<LayoutItem *> items() const;

    static std::unique_ptr<Layout> standard26Key();

private:
    FCITX_DECLARE_PRIVATE(Layout);
    std::unique_ptr<LayoutPrivate> d_ptr;
};

class Button : public LayoutItem {
public:
    static constexpr uint32_t SPACER_ID = 0;

    Button(uint32_t id = 0);
    ~Button();

    uint32_t id() const;
    bool isSpacer() const;

private:
    FCITX_DECLARE_PRIVATE(Button);
    std::unique_ptr<ButtonPrivate> d_ptr;
};

enum class SpecialAction { None, LanguageSwitch, Commit };

class ButtonMetadataPrivate;
class ButtonMetadata {
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
    FCITX_DECLARE_PROPERTY(SpecialAction, specialAction, setSpecialAction);

private:
    FCITX_DECLARE_PRIVATE(ButtonMetadata);
    std::unique_ptr<ButtonMetadataPrivate> d_ptr;
};

class KeymapPrivate;
class Keymap {
public:
    Keymap();
    FCITX_INLINE_DEFINE_DEFAULT_DTOR_AND_MOVE(Keymap);

    ButtonMetadata &setKey(uint32_t id, ButtonMetadata metadata);
    void clear();
    void removeKey(uint32_t id);

    static Keymap qwerty();

private:
    FCITX_DECLARE_PRIVATE(Keymap);
    std::unique_ptr<KeymapPrivate> d_ptr;
};

}; // namespace fcitx::virtualkeyboard

#endif // _FCITX_VIRTUALKEYBOARD_LAYOUT_H_
