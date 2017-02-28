/*
 * Copyright (C) 2017~2017 by CSSlayer
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
#ifndef _FCITX_ELEMENT_H_
#define _FCITX_ELEMENT_H_

#include "fcitxcore_export.h"
#include <fcitx-utils/dynamictrackableobject.h>
#include <unordered_set>

namespace fcitx {

class ElementPrivate;

class FCITXCORE_EXPORT Element : public DynamicTrackableObject {
public:
    Element();
    ~Element();
    std::list<Element *> parents();
    std::list<Element *> childs();

protected:
    // Sub class may use these functions carefully if they intends
    // to have single type of childs.
    void addChild(Element *child);
    void addParent(Element *parent);

    void insertChild(Element *before, Element *child);
    void insertParent(Element *before, Element *parent);

    void removeParent(Element *parent);
    void removeChild(Element *child);

    static void addEdge(Element *parent, Element *child, Element *beforeChild, Element *beforeParent);
    static void removeEdge(Element *parent, Element *child);

private:
    std::unique_ptr<ElementPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(Element);
};
}

#endif // _FCITX_ELEMENT_H_
