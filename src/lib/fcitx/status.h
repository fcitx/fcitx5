/*
 * Copyright (C) 2016~2016 by CSSlayer
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
#ifndef _FCITX_STATUS_H_
#define _FCITX_STATUS_H_

#include <fcitx-utils/macros.h>
#include <memory>
#include "fcitxcore_export.h"

namespace fcitx
{
class StatusPrivate;

class FCITXCORE_EXPORT Status {
public:

    virtual ~Status();

    std::string text() const;
    void setText(const std::string &text);
    std::string icon() const;
    void setIcon(const std::string &icon);

    bool isCheckable() const;
    void setCheckable(bool checkable);

    bool isChecked() const;
    void setChecked(bool checked);

    void activate();

private:
    std::unique_ptr<StatusPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(Status);
};

}

#endif // _FCITX_STATUS_H_
