/*
 * Copyright (C) 2015~2015 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; see the file COPYING. If not,
 * see <http://www.gnu.org/licenses/>.
 */
#ifndef _FCITX_CONFIG_INIPARSER_H_
#define _FCITX_CONFIG_INIPARSER_H_

#include "fcitxconfig_export.h"
#include "rawconfig.h"

namespace fcitx {
FCITXCONFIG_EXPORT void readFromIni(RawConfig &config, std::istream &in);
FCITXCONFIG_EXPORT void writeAsIni(const RawConfig &config, std::ostream &out);
}


#endif // _FCITX_CONFIG_INIPARSER_H_
