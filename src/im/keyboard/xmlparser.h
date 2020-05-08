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
    virtual void startElement(const XML_Char *, const XML_Char **) {}
    virtual void endElement(const XML_Char *) {}
    virtual void characterData(const XML_Char *, int) {}
};
} // namespace fcitx

#endif // _FCITX_IM_KEYBOARD_XMLPARSER_H_
