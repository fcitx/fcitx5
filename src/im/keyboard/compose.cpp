/*
 * SPDX-FileCopyrightText: 2022-2022 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "compose.h"
#include <fcitx-utils/utf8.h>

namespace fcitx {

bool isDeadKey(KeySym sym) {
    return (sym >= FcitxKey_dead_grave && sym <= FcitxKey_dead_greek);
}

void appendDeadKey(std::string &string, KeySym keysym) {
    /* Sadly, not all the dead keysyms have spacing mark equivalents
     * in Unicode. For those that don't, we use NBSP + the non-spacing
     * mark as an approximation.
     */
    switch (keysym) {
#define CASE(keysym, unicode, sp)                                              \
    case FcitxKey_dead_##keysym:                                               \
        if (sp) {                                                              \
            string.push_back('\xa0');                                          \
        }                                                                      \
        string.append(utf8::UCS4ToUTF8(unicode));                              \
        break;

        CASE(grave, 0x60, 0);
        CASE(acute, 0xb4, 0);
        CASE(circumflex, 0x5e, 0);
        CASE(tilde, 0x7e, 0);
        CASE(macron, 0xaf, 0);
        CASE(breve, 0x2d8, 0);
        CASE(abovedot, 0x307, 1);
        CASE(diaeresis, 0xa8, 0);
        CASE(abovering, 0x2da, 0);
        CASE(hook, 0x2c0, 0);
        CASE(doubleacute, 0x2dd, 0);
        CASE(caron, 0x2c7, 0);
        CASE(cedilla, 0xb8, 0);
        CASE(ogonek, 0x2db, 0);
        CASE(iota, 0x37a, 0);
        CASE(voiced_sound, 0x3099, 1);
        CASE(semivoiced_sound, 0x309a, 1);
        CASE(belowdot, 0x323, 1);
        CASE(horn, 0x31b, 1);
        CASE(stroke, 0x335, 1);
        CASE(abovecomma, 0x2bc, 0);
        CASE(abovereversedcomma, 0x2bd, 1);
        CASE(doublegrave, 0x30f, 1);
        CASE(belowring, 0x2f3, 0);
        CASE(belowmacron, 0x2cd, 0);
        CASE(belowcircumflex, 0x32d, 1);
        CASE(belowtilde, 0x330, 1);
        CASE(belowbreve, 0x32e, 1);
        CASE(belowdiaeresis, 0x324, 1);
        CASE(invertedbreve, 0x32f, 1);
        CASE(belowcomma, 0x326, 1);
        CASE(lowline, 0x5f, 0);
        CASE(aboveverticalline, 0x2c8, 0);
        CASE(belowverticalline, 0x2cc, 0);
        CASE(longsolidusoverlay, 0x338, 1);
        CASE(a, 0x363, 1);
        CASE(A, 0x363, 1);
        CASE(e, 0x364, 1);
        CASE(E, 0x364, 1);
        CASE(i, 0x365, 1);
        CASE(I, 0x365, 1);
        CASE(o, 0x366, 1);
        CASE(O, 0x366, 1);
        CASE(u, 0x367, 1);
        CASE(U, 0x367, 1);
        CASE(small_schwa, 0x1dea, 1);
        CASE(capital_schwa, 0x1dea, 1);
#undef CASE
    default:
        string.append(Key::keySymToUTF8(keysym));
    }
}

ComposeState::ComposeState(Instance *instance, InputContext *inputContext)
    : instance_(instance), inputContext_(inputContext) {}

std::tuple<std::string, bool> ComposeState::type(KeySym sym) {
    std::string result;
    bool consumeKey = typeImpl(sym, result);
    return {result, consumeKey};
}

bool ComposeState::typeImpl(KeySym sym, std::string &result) {
    bool wasComposing = instance_->isComposing(inputContext_);
    if (wasComposing != !composeBuffer_.empty()) {
        FCITX_DEBUG() << "Inconsistent compose state";
        reset();
        wasComposing = false;
    }

    composeBuffer_.push_back(sym);
    // check compose first.
    auto composeResult = instance_->processComposeString(inputContext_, sym);
    if (!composeResult) {
        if (instance_->isComposing(inputContext_)) {
            return true;
        }
        // If compose is invalid, and we are not in composing state before,
        // ignore this.
        if (!wasComposing) {
            composeBuffer_.clear();
            return false;
        }

        // Check for dead key prefix.
        size_t numDeadPrefix = 0;
        for (auto sym : composeBuffer_) {
            if (isDeadKey(sym)) {
                numDeadPrefix += 1;
            } else {
                break;
            }
        }
        if (composeBuffer_.size() > 1 &&
            numDeadPrefix >= composeBuffer_.size() - 1) {
            // If compose sequence ends with a non dead key.
            if (numDeadPrefix == composeBuffer_.size() - 1) {
                /* dead keys are never *really* dead */
                for (size_t i = 0; i < numDeadPrefix; i++) {
                    appendDeadKey(result, composeBuffer_[i]);
                }

                reset();
                return false;
            }

            // If everything is dead key, commit the first dead key.
            appendDeadKey(result, composeBuffer_[0]);
            // Then backtrack.
            composeBuffer_.pop_front();
            std::deque<KeySym> buffer;
            using std::swap;
            swap(buffer, composeBuffer_);
            reset();
            bool consumed = false;
            for (auto iter = buffer.begin(), end = buffer.end(); iter != end;
                 ++iter) {
                consumed = typeImpl(sym, result);
                if (std::next(iter) != end) {
                    result.append(Key::keySymToUTF8(sym));
                }
            }
            return consumed;
        }
        if (composeBuffer_.size() > 1) {
            reset();
            return true;
        }
    }
    // Empty string but composing, means ignored
    if (instance_->isComposing(inputContext_)) {
        // Do not push feed ignore key into compose buffer.
        composeBuffer_.pop_back();
        return false;
    }

    reset();
    // Compose success or key should be ignored.
    if (composeResult->empty()) {
        return false;
    }

    result.append(*composeResult);
    return true;
}

std::string ComposeState::preedit() const {
    std::string result;
    for (size_t i = 0, e = composeBuffer_.size(); i < e; i++) {
        if (composeBuffer_[i] == FcitxKey_Multi_key) {
            if (e == 1 || i > 0 ||
                (i + 1 < e && composeBuffer_[i + 1] == FcitxKey_Multi_key)) {
                result.append("·");
            }
        } else {
            if (isDeadKey(composeBuffer_[i])) {
                appendDeadKey(result, composeBuffer_[i]);
            } else {
                result.append(Key::keySymToUTF8(composeBuffer_[i]));
            }
        }
    }
    return result;
}

void ComposeState::backspace() {
    if (composeBuffer_.empty()) {
        return;
    }
    std::deque<KeySym> buffer;
    using std::swap;
    swap(buffer, composeBuffer_);
    reset();
    buffer.pop_back();
    for (auto sym : buffer) {
        type(sym);
    }
}

void ComposeState::reset() {
    instance_->resetCompose(inputContext_);
    composeBuffer_.clear();
}

} // namespace fcitx
