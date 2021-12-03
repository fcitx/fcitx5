/*
 * SPDX-FileCopyrightText: 2021~2021 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "layout.h"
#include <any>
#include <map>
#include <unordered_map>
#include <fcitx-utils/keysym.h>
#include <fcitx-utils/misc_p.h>

namespace fcitx::virtualkeyboard {

class LayoutPrivate {
public:
    LayoutOrientation orientation_ = LayoutOrientation::Horizontal;
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
    std::any actionData_;
};

class KeymapPrivate {
public:
    std::map<uint32_t, ButtonMetadata> keys_;
};

class VirtualKeyboardPrivate {
public:
    std::vector<std::pair<std::string, std::unique_ptr<Keymap>>> keymaps_;
};

Layout::Layout() : d_ptr(std::make_unique<LayoutPrivate>()) {}

Layout::~Layout() = default;

FCITX_DEFINE_PROPERTY_PRIVATE(Layout, LayoutOrientation, orientation,
                              setOrientation);
FCITX_DEFINE_PROPERTY_PRIVATE(Layout, LayoutAlignment, alignment, setAlignment);

const std::vector<std::unique_ptr<LayoutItem>> &Layout::items() const {
    FCITX_D();
    return d->subItems_;
}

Layout *Layout::addLayout(float size) {
    if (size > 1) {
        throw std::invalid_argument("Invalid size");
    }
    FCITX_D();
    d->subItems_.push_back(std::make_unique<Layout>());
    auto newItem = d->subItems_.back().get();
    d->sizeMap_[newItem] = size;
    return static_cast<Layout *>(newItem);
}

Button *Layout::addButton(float size, uint32_t id) {
    if (size > 1) {
        throw std::invalid_argument("Invalid size");
    }
    FCITX_D();
    d->subItems_.push_back(std::make_unique<Button>(id));
    auto newItem = d->subItems_.back().get();
    d->sizeMap_[newItem] = size;
    return static_cast<Button *>(newItem);
}

void Layout::writeToRawConfig(RawConfig &config) const {
    FCITX_D();
    auto sub = config.get("Layout", true);
    sub->setValueByPath("Alignment", LayoutAlignmentToString(d->alignment_));
    sub->setValueByPath("Orientation",
                        LayoutOrientationToString(d->orientation_));
    for (size_t i = 0; i < d->subItems_.size(); i++) {
        auto item = sub->get(std::to_string(i), true);
        auto weight = findValue(d->sizeMap_, d->subItems_[i].get());
        assert(weight);
        item->setValueByPath("Weight", std::to_string(*weight));
        d->subItems_[i]->writeToRawConfig(*item);
    }
}

Button::Button(uint32_t id) : d_ptr(std::make_unique<ButtonPrivate>(id)) {}

uint32_t Button::id() const {
    FCITX_D();
    return d->id_;
}

bool Button::isSpacer() const { return id() == SPACER_ID; }

void Button::writeToRawConfig(RawConfig &config) const {
    config.setValueByPath("Button", std::to_string(id()));
}

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

fcitx::virtualkeyboard::SpecialAction
fcitx::virtualkeyboard::ButtonMetadata::specialAction() const {
    FCITX_D();
    return d->specialAction_;
}

void ButtonMetadata::setEnterKey() {
    FCITX_D();
    d->specialAction_ = SpecialAction::EnterKey;
    d->actionData_.reset();
}

void ButtonMetadata::setLanguageSwitch() {
    FCITX_D();
    d->specialAction_ = SpecialAction::LanguageSwitch;
    d->actionData_.reset();
}

void ButtonMetadata::setLayoutSwitch(uint32_t layout) {
    FCITX_D();
    d->specialAction_ = SpecialAction::LayoutSwitch;
    d->actionData_ = layout;
}

void ButtonMetadata::writeToRawConfig(RawConfig &config) const {
    FCITX_D();
    config.setValueByPath("Text", d->text_);
    config.setValueByPath("Code", std::to_string(d->code_));
    if (!d->longPress_.empty()) {
        auto longpress = config.get("LongPress", true);
        size_t i = 0;
        for (const auto &[text, code] : d->longPress_) {
            auto item = longpress->get(std::to_string(i), true);
            item->setValueByPath("Text", text);
            item->setValueByPath("Code", std::to_string(code));
        }
    }

    if (d->specialAction_ != SpecialAction::None) {
        config.setValueByPath("Action",
                              SpecialActionToString(d->specialAction_));
    }

    switch (d->specialAction_) {
    case SpecialAction::LayoutSwitch:
        config.setValueByPath(
            "Layout", std::to_string(std::any_cast<uint32_t>(d->actionData_)));
    default:
        break;
    }
}

Keymap::Keymap() : d_ptr(std::make_unique<KeymapPrivate>()) {}

FCITX_DEFINE_DEFAULT_DTOR_AND_MOVE(Keymap);

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
    layout->setOrientation(LayoutOrientation::Vertical);
    auto topLayout = layout->addLayout(1.0f / 4);
    topLayout->setOrientation(LayoutOrientation::Horizontal);
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
    middleLayout->setOrientation(LayoutOrientation::Horizontal);
    for (auto key : middleKey) {
        middleLayout->addButton(1.0f / topKey.size(), key);
    }

    std::array bottomKey = {FcitxKey_Z, FcitxKey_X, FcitxKey_C, FcitxKey_V,
                            FcitxKey_B, FcitxKey_N, FcitxKey_M};
    auto bottomLayout = layout->addLayout(1.0f / 4);
    bottomLayout->setOrientation(LayoutOrientation::Horizontal);
    bottomLayout->addButton(0.15f, FcitxKey_Caps_Lock);
    for (auto key : bottomKey) {
        bottomLayout->addButton(1.0f / topKey.size(), key);
    }
    bottomLayout->addButton(0.15f, FcitxKey_BackSpace);

    auto functionLayout = layout->addLayout(1.0f / 4);
    functionLayout->setOrientation(LayoutOrientation::Horizontal);
    functionLayout->addButton(0.125f, FcitxKey_Execute);
    functionLayout->addButton(0.125f, FcitxKey_Mode_switch);
    functionLayout->addButton(0.1f, FcitxKey_comma);
    functionLayout->addButton(-1, FcitxKey_space);
    functionLayout->addButton(0.1f, FcitxKey_period);
    functionLayout->addButton(0.25f, FcitxKey_Return);

    return layout;
}

void Keymap::writeToRawConfig(RawConfig &config) const {
    FCITX_D();
    for (const auto &[id, button] : d->keys_) {
        auto buttonConfig = config.get(std::to_string(id), true);
        button.writeToRawConfig(*buttonConfig);
    }
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
    keymap.setKey(FcitxKey_Mode_switch, {"🌐", FcitxKey_Mode_switch})
        .setLanguageSwitch();
    keymap.setKey(FcitxKey_comma, {",", FcitxKey_comma});
    keymap.setKey(FcitxKey_space, {"", FcitxKey_space});
    keymap.setKey(FcitxKey_period, {".", FcitxKey_period});
    keymap.setKey(FcitxKey_Return, {"⏎", FcitxKey_Return}).setEnterKey();
    return keymap;
}

fcitx::virtualkeyboard::VirtualKeyboard::VirtualKeyboard()
    : d_ptr(std::make_unique<VirtualKeyboardPrivate>()) {}

fcitx::virtualkeyboard::VirtualKeyboard::~VirtualKeyboard() = default;

fcitx::virtualkeyboard::Keymap *
fcitx::virtualkeyboard::VirtualKeyboard::addKeymap(std::string layout) {
    FCITX_D();
    d->keymaps_.emplace_back(std::move(layout), std::make_unique<Keymap>());
    return d->keymaps_.back().second.get();
}

void fcitx::virtualkeyboard::VirtualKeyboard::writeToRawConfig(
    fcitx::RawConfig &config) const {
    FCITX_D();
    for (size_t i = 0; i < d->keymaps_.size(); i++) {
        auto keymapConfig = config.get(std::to_string(i), true);
        keymapConfig->setValueByPath("LayoutName", d->keymaps_[i].first);
        d->keymaps_[i].second->writeToRawConfig(*keymapConfig);
    }
}

} // namespace fcitx::virtualkeyboard
