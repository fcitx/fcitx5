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

#include "fcitx/element.h"
#include <cassert>

namespace test {

class Element : public fcitx::Element {
public:
    using fcitx::Element::addParent;
    using fcitx::Element::removeParent;
    using fcitx::Element::addChild;
    using fcitx::Element::removeChild;
    using fcitx::Element::insertParent;
    using fcitx::Element::insertChild;
};
}

int main() {
    using test::Element;
    {
        Element e, e2;
        e.addParent(&e2);
        assert(e.parents().size() == 1);
        assert(e.childs().size() == 0);
        assert(e2.parents().size() == 0);
        assert(e2.childs().size() == 1);
    }
    {
        Element e, e2;
        e.addParent(&e2);
        e2.addParent(&e);
        assert(e.parents().size() == 1);
        assert(e.childs().size() == 1);
        assert(e2.parents().size() == 1);
        assert(e2.childs().size() == 1);
    }
    {
        Element e, *e2 = new Element;
        e.addParent(e2);
        assert(e.parents().size() == 1);
        assert(e.childs().size() == 0);
        assert(e2->parents().size() == 0);
        assert(e2->childs().size() == 1);
        delete e2;

        assert(e.parents().size() == 0);
        assert(e.childs().size() == 0);
    }
    {
        Element e, e2, e3;
        e.addChild(&e2);
        assert(e.childs().front() == &e2);
        e.addChild(&e3);
        assert(e.childs().front() == &e2);
        assert(e.childs().back() == &e3);
        e.insertChild(&e2, &e3);
        assert(e.childs().front() == &e3);
        assert(e.childs().back() == &e2);
        assert(e.childs().size() == 2);
    }
    return 0;
}
