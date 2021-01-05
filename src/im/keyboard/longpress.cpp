/*
 * SPDX-FileCopyrightText: 2021~2021 CSSlayer <wengxt@gmail.com>
 * SPDX-FileCopyrightText: 2020~2020 Carson Black <uhhadd@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "longpress.h"

namespace fcitx {

// The data is too complex to use default construction, so we use
// syncDefaultValueToCurrent instead.
void setupDefaultLongPressConfig(LongPressConfig &config) {
    static const std::list<std::pair<std::string, std::vector<std::string>>>
        data = {
            //
            // Latin
            //
            {"a", {"à", "á", "â", "ä", "æ", "ã", "å", "ā"}},
            {"c", {"ç", "ć", "č"}},
            {"d", {"ð"}},
            {"e", {"è", "é", "ê", "ë", "ē", "ė", "ę", "ȩ", "ḝ", "ə"}},
            {"g", {"ğ"}},
            {"i", {"î", "ï", "í", "ī", "į", "ì", "ı"}},
            {"l", {"ł"}},
            {"n", {"ñ", "ń"}},
            {"o", {"ô", "ö", "ò", "ó", "œ", "ø", "ō", "õ"}},
            {"s", {"ß", "ś", "š", "ş"}},
            {"u", {"û", "ü", "ù", "ú", "ū"}},
            {"x", {"×"}},
            {"y", {"ÿ", "ұ", "ү", "ӯ", "ў"}},
            {"z", {"ž", "ź", "ż"}},
            //
            // Upper case Latin
            //
            {"A", {"À", "Á", "Â", "Ä", "Æ", "Ã", "Å", "Ā"}},
            {"C", {"Ç", "Ć", "Č"}},
            {"D", {"Ð"}},
            {"E", {"È", "É", "Ê", "Ë", "Ē", "Ė", "Ę", "Ȩ", "Ḝ", "Ə"}},
            {"G", {"Ğ"}},
            {"I", {"Î", "Ï", "Í", "Ī", "Į", "Ì"}},
            {"L", {"Ł"}},
            {"N", {"Ñ", "Ń"}},
            {"O", {"Ô", "Ö", "Ò", "Ó", "Œ", "Ø", "Ō", "Õ"}},
            {"S", {"ẞ", "Ś", "Š", "Ş"}},
            {"U", {"Û", "Ü", "Ù", "Ú", "Ū"}},
            {"Y", {"Ÿ", "Ұ", "Ү", "Ӯ", "Ў"}},
            {"Z", {"Ž", "Ź", "Ż"}},
            //
            // Upper case Cyrilic
            //
            {"г", {"ғ"}},
            {"е", {"ё"}},      // this in fact NOT the same E as before
            {"и", {"ӣ", "і"}}, // і is not i
            {"й", {"ј"}},      // ј is not j
            {"к", {"қ", "ҝ"}},
            {"н", {"ң", "һ"}}, // һ is not h
            {"о", {"ә", "ө"}},
            {"ч", {"ҷ", "ҹ"}},
            {"ь", {"ъ"}},
            //
            // Cyrilic
            //
            {"Г", {"Ғ"}},
            {"Е", {"Ё"}},      // This In Fact Not The Same E As Before
            {"И", {"Ӣ", "І"}}, // І Is Not I
            {"Й", {"Ј"}},      // Ј Is Not J
            {"К", {"Қ", "Ҝ"}},
            {"Н", {"Ң", "Һ"}}, // Һ Is Not H
            {"О", {"Ә", "Ө"}},
            {"Ч", {"Ҷ", "Ҹ"}},
            {"Ь", {"Ъ"}},
            //
            // Arabic
            //
            // This renders weirdly in text editors, but is valid code.
            {"ا", {"أ", "إ", "آ", "ء"}},
            {"ب", {"پ"}},
            {"ج", {"چ"}},
            {"ز", {"ژ"}},
            {"ف", {"ڤ"}},
            {"ك", {"گ"}},
            {"ل", {"لا"}},
            {"ه", {"ه"}},
            {"و", {"ؤ"}},
            //
            // Hebrew
            //
            // Likewise, this will render oddly, but is still valid code.
            {"ג", {"ג׳"}},
            {"ז", {"ז׳"}},
            {"ח", {"ח׳"}},
            {"צ׳", {"צ׳"}},
            {"ת", {"ת׳"}},
            {"י", {"ײַ"}},
            {"י", {"ײ"}},
            {"ח", {"ױ"}},
            {"ו", {"װ"}},
            //
            // Numbers
            //
            {"0", {"∅", "ⁿ", "⁰"}},
            {"1", {"¹", "½", "⅓", "¼", "⅕", "⅙", "⅐", "⅛", "⅑", "⅒"}},
            {"2", {"²", "⅖", "⅔"}},
            {"3", {"³", "⅗", "¾", "⅜"}},
            {"4", {"⁴", "⅘", "⁵", "⅝", "⅚"}},
            {"5", {"⁵", "⅝", "⅚"}},
            {"6", {"⁶"}},
            {"7", {"⁷", "⅞"}},
            {"8", {"⁸"}},
            {"9", {"⁹"}},
            //
            // Punctuation
            //
            {R"(-)", {"—", "–", "·"}},
            {R"(?)", {"¿", "‽"}},
            {R"(')", {"‘", "’", "‚", "‹", "›"}},
            {R"(!)", {"¡"}},
            {R"(")", {"“", "”", "„", "«", "»"}},
            {R"(/)", {"÷"}},
            {R"(#)", {"№"}},
            {R"(%)", {"‰", "℅"}},
            {R"(^)", {"↑", "←", "→", "↓"}},
            {R"(+)", {"±"}},
            {R"(<)", {"«", "≤", "‹", "⟨"}},
            {R"(=)", {"∞", "≠", "≈"}},
            {R"(>)", {"⟩", "»", "≥", "›"}},
            //
            // Currency
            //
            {"$", {"¢", "€", "£", "¥", "₹", "₽", "₺", "₩", "₱", "₿"}},
        };
    {
        auto value = config.entries.mutableValue();
        for (const auto &[key, candidates] : data) {
            LongPressEntryConfig entry;
            entry.key.setValue(key);
            entry.candidates.setValue(candidates);
            value->emplace_back(std::move(entry));
        }
    }
    config.syncDefaultValueToCurrent();
}

std::unordered_map<std::string, std::vector<std::string>>
longPressData(const LongPressConfig &config) {
    std::unordered_map<std::string, std::vector<std::string>> result;
    for (const auto &entry : *config.entries) {
        if (!*entry.enable || entry.candidates->empty()) {
            continue;
        }
        result[*entry.key] = *entry.candidates;
    }
    return result;
}

} // namespace fcitx
