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
#ifndef _FCITX_USERINTERFACE_H_
#define _FCITX_USERINTERFACE_H_

#include <fcitx/addoninstance.h>
#include <fcitx-utils/macros.h>
#include <memory>

namespace fcitx
{

class UserIntefacePrivate;

class FCITXCORE_EXPORT UserInteface : public AddonInstance {
public:

    virtual ~UserInteface();

    virtual bool availableFor(const std::string &displayServer) const = 0;
    virtual void suspend() = 0;
    virtual void resume() = 0;

private:
    std::unique_ptr<UserIntefacePrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(UserInteface);
};

};

#endif // _FCITX_USERINTERFACE_H_
