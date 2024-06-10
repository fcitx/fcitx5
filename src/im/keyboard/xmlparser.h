/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_IM_KEYBOARD_XMLPARSER_H_
#define _FCITX_IM_KEYBOARD_XMLPARSER_H_

#include <string>
#include <expat.h>

namespace fcitx {

class XMLParser {
public:
    virtual ~XMLParser() {}
    bool parse(const std::string &name);

protected:
    virtual void startElement(const char *name, const char **attrs) = 0;
    virtual void endElement(const char *) = 0;
    virtual void characterData(const char *ch, int len) = 0;
};
} // namespace fcitx

#endif // _FCITX_IM_KEYBOARD_XMLPARSER_H_
