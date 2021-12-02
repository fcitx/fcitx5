/*
 * SPDX-FileCopyrightText: 2021~2021 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "layout.h"
#include <unordered_map>
#include <fcitx-utils/keysym.h>

namespace fcitx::virtualkeyboard {

class LayoutPrivate {
public:
    LayoutDirection direction_ = LayoutDirection::Horizontal;
    LayoutAlignment alignment_ = LayoutAlignment::Begin;
    std::vector<std::unique_ptr<LayoutItem>> subItems_;
    std::unordered_map<LayoutItem *, float> sizeMap_;
};

class ButtonPrivate {
public:
    ButtonPrivate(uint32_t id) : id_(id) {}
    uint32_t id_;
};

class ButtonMetadataPrivate {
public:
    uint32_t code_ = 0;
    std::string text_;
    std::vector<ButtonMetadata::LongPressMetadata> longPress_;
    bool visible_ = true;
    SpecialAction specialAction_ = SpecialAction::None;
};

class KeymapPrivate {
public:
    std::unordered_map<uint32_t, ButtonMetadata> keys_;
};

Layout::Layout() : d_ptr(std::make_unique<LayoutPrivate>()) {}

Layout::~Layout() = default;

FCITX_DEFINE_PROPERTY_PRIVATE(Layout, LayoutDirection, direction, setDirection);
FCITX_DEFINE_PROPERTY_PRIVATE(Layout, LayoutAlignment, alignment, setAlignment);

std::vector<LayoutItem *> Layout::items() const {
    std::vector<LayoutItem *> result;
    for (auto *ele : childs()) {
        result.push_back(static_cast<LayoutItem *>(ele));
    }
    return result;
}

Layout *Layout::addLayout(float size) {
    if (size <= 0 || size > 1) {
        throw std::invalid_argument("Invalid size");
    }
    FCITX_D();
    d->subItems_.push_back(std::make_unique<Layout>());
    return static_cast<Layout *>(d->subItems_.back().get());
}

Button *Layout::addButton(float size, uint32_t id) {
    if (size > 1) {
        throw std::invalid_argument("Invalid size");
    }
    FCITX_D();
    d->subItems_.push_back(std::make_unique<Button>(id));
    return static_cast<Button *>(d->subItems_.back().get());
}

Button::Button(uint32_t id) : d_ptr(std::make_unique<ButtonPrivate>(id)) {}

uint32_t Button::id() const {
    FCITX_D();
    return d->id_;
}

bool Button::isSpacer() const { return id() == SPACER_ID; }

Button::~Button() = default;

ButtonMetadata::ButtonMetadata()
    : d_ptr(std::make_unique<ButtonMetadataPrivate>()) {}

ButtonMetadata::ButtonMetadata(std::string text, uint32_t code,
                               std::vector<LongPressMetadata> longpress)
    : ButtonMetadata() {
    setText(std::move(text));
    setCode(code);
    setLongPress(std::move(longpress));
}

FCITX_DEFINE_PROPERTY_PRIVATE(ButtonMetadata, uint32_t, code, setCode);
FCITX_DEFINE_PROPERTY_PRIVATE(ButtonMetadata, std::string, text, setText);
FCITX_DEFINE_PROPERTY_PRIVATE(ButtonMetadata,
                              std::vector<ButtonMetadata::LongPressMetadata>,
                              longPress, setLongPress);
FCITX_DEFINE_PROPERTY_PRIVATE(ButtonMetadata, bool, visible, setVisible);
FCITX_DEFINE_PROPERTY_PRIVATE(ButtonMetadata, SpecialAction, specialAction,
                              setSpecialAction);

Keymap::Keymap() : d_ptr(std::make_unique<KeymapPrivate>()) {}

ButtonMetadata &Keymap::setKey(uint32_t id, ButtonMetadata metadata) {
    FCITX_D();
    return (d->keys_[id] = std::move(metadata));
}

void Keymap::removeKey(uint32_t id) {
    FCITX_D();
    d->keys_.erase(id);
}

void Keymap::clear() {
    FCITX_D();
    d->keys_.clear();
}

std::unique_ptr<Layout> Layout::standard26Key() {
    auto layout = std::make_unique<Layout>();
    layout->setDirection(LayoutDirection::Vertical);
    auto topLayout = layout->addLayout(1.0f / 4);
    topLayout->setDirection(LayoutDirection::Horizontal);
    std::array topKey = {FcitxKey_Q, FcitxKey_W, FcitxKey_E, FcitxKey_R,
                         FcitxKey_T, FcitxKey_Y, FcitxKey_U, FcitxKey_I,
                         FcitxKey_O, FcitxKey_P};
    for (auto key : topKey) {
        topLayout->addButton(1.0f / topKey.size(), key);
    }
    std::array middleKey = {FcitxKey_A, FcitxKey_S, FcitxKey_D,
                            FcitxKey_F, FcitxKey_G, FcitxKey_H,
                            FcitxKey_J, FcitxKey_K, FcitxKey_L};
    auto middleLayout = layout->addLayout(1.0f / 4);
    middleLayout->setDirection(LayoutDirection::Horizontal);
    for (auto key : middleKey) {
        middleLayout->addButton(1.0f / topKey.size(), key);
    }

    std::array bottomKey = {FcitxKey_Z, FcitxKey_X, FcitxKey_C, FcitxKey_V,
                            FcitxKey_B, FcitxKey_N, FcitxKey_M};
    auto bottomLayout = layout->addLayout(1.0f / 4);
    bottomLayout->setDirection(LayoutDirection::Horizontal);
    bottomLayout->addButton(0.15f, FcitxKey_Caps_Lock);
    for (auto key : bottomKey) {
        bottomLayout->addButton(1.0f / topKey.size(), key);
    }
    bottomLayout->addButton(0.15f, FcitxKey_BackSpace);

    auto functionLayout = layout->addLayout(1.0f / 4);
    functionLayout->setDirection(LayoutDirection::Horizontal);
    functionLayout->addButton(0.125f, FcitxKey_Execute);
    functionLayout->addButton(0.125f, FcitxKey_Mode_switch);
    functionLayout->addButton(0.1f, FcitxKey_comma);
    functionLayout->addButton(-1, FcitxKey_space);
    functionLayout->addButton(0.1f, FcitxKey_period);
    functionLayout->addButton(0.25f, FcitxKey_Return);

    return layout;
}

Keymap Keymap::qwerty() {
    Keymap keymap;
    keymap.setKey(FcitxKey_Q, {"q", FcitxKey_q, {{"1", FcitxKey_1}}});
    keymap.setKey(FcitxKey_W, {"w", FcitxKey_w, {{"2", FcitxKey_2}}});
    keymap.setKey(FcitxKey_E, {"e", FcitxKey_e, {{"3", FcitxKey_3}}});
    keymap.setKey(FcitxKey_R, {"r", FcitxKey_r, {{"4", FcitxKey_4}}});
    keymap.setKey(FcitxKey_T, {"t", FcitxKey_t, {{"5", FcitxKey_5}}});
    keymap.setKey(FcitxKey_Y, {"y", FcitxKey_y, {{"6", FcitxKey_6}}});
    keymap.setKey(FcitxKey_U, {"u", FcitxKey_u, {{"7", FcitxKey_7}}});
    keymap.setKey(FcitxKey_I, {"i", FcitxKey_i, {{"8", FcitxKey_8}}});
    keymap.setKey(FcitxKey_O, {"o", FcitxKey_o, {{"9", FcitxKey_9}}});
    keymap.setKey(FcitxKey_P, {"p", FcitxKey_p, {{"0", FcitxKey_0}}});

    keymap.setKey(FcitxKey_A, {"a", FcitxKey_a, {{"@", FcitxKey_at}}});
    keymap.setKey(FcitxKey_S, {"s", FcitxKey_s, {{"*", FcitxKey_asterisk}}});
    keymap.setKey(FcitxKey_D, {"d", FcitxKey_d, {{"+", FcitxKey_plus}}});
    keymap.setKey(FcitxKey_F, {"f", FcitxKey_f, {{"-", FcitxKey_minus}}});
    keymap.setKey(FcitxKey_G, {"g", FcitxKey_g, {{"=", FcitxKey_equal}}});
    keymap.setKey(FcitxKey_H, {"h", FcitxKey_h, {{"/", FcitxKey_slash}}});
    keymap.setKey(FcitxKey_J, {"j", FcitxKey_j, {{"#", FcitxKey_numbersign}}});
    keymap.setKey(FcitxKey_K, {"k", FcitxKey_k, {{"(", FcitxKey_parenleft}}});
    keymap.setKey(FcitxKey_L, {"l", FcitxKey_l, {{")", FcitxKey_parenright}}});

    keymap.setKey(FcitxKey_Caps_Lock, {"⇑", FcitxKey_Caps_Lock});
    keymap.setKey(FcitxKey_Z, {"q", FcitxKey_q, {{"1", FcitxKey_1}}});
    keymap.setKey(FcitxKey_X, {"w", FcitxKey_w, {{"2", FcitxKey_2}}});
    keymap.setKey(FcitxKey_C, {"e", FcitxKey_e, {{"3", FcitxKey_3}}});
    keymap.setKey(FcitxKey_V, {"r", FcitxKey_r, {{"4", FcitxKey_4}}});
    keymap.setKey(FcitxKey_B, {"t", FcitxKey_t, {{"5", FcitxKey_5}}});
    keymap.setKey(FcitxKey_N, {"y", FcitxKey_y, {{"6", FcitxKey_6}}});
    keymap.setKey(FcitxKey_M, {"u", FcitxKey_u, {{"7", FcitxKey_7}}});
    keymap.setKey(FcitxKey_BackSpace, {"⌫", FcitxKey_BackSpace});

    keymap.setKey(FcitxKey_Execute, {"“”", FcitxKey_Execute});
    keymap.setKey(FcitxKey_Mode_switch, {"🌐", FcitxKey_Mode_switch});
    keymap.setKey(FcitxKey_comma, {",", FcitxKey_comma});
    keymap.setKey(FcitxKey_space, {"", FcitxKey_space});
    keymap.setKey(FcitxKey_period, {".", FcitxKey_period});
    keymap.setKey(FcitxKey_Return, {"⏎", FcitxKey_Return})
        .setSpecialAction(SpecialAction::Commit);
    return keymap;
}

} // namespace fcitx::virtualkeyboard
