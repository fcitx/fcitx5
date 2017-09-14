/*
 * Copyright (C) 2017~2017 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
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

#include "element.h"
#include <algorithm>
#include <unordered_map>

namespace fcitx {

template <typename T>
class OrderedSet {
    typedef std::list<T> OrderList;

public:
    bool insert(const T &before, const T &v) {
        if (dict_.count(v)) {
            return false;
        }
        auto iter =
            std::find_if(order_.begin(), order_.end(),
                         [before](const auto &t) { return (t == before); });
        auto newIter = order_.insert(iter, v);
        dict_.insert(std::make_pair(v, newIter));
        return true;
    }

    bool contains(const T &v) const { return !!dict_.count(v); }

    bool remove(const T &v) {
        auto iter = dict_.find(v);
        if (iter == dict_.end()) {
            return false;
        }
        order_.erase(iter->second);
        dict_.erase(iter);
        return true;
    }

    const OrderList &order() const { return order_; }

private:
    std::unordered_map<T, typename OrderList::iterator> dict_;
    OrderList order_;
};

class ElementPrivate {
    typedef std::list<Element *> ElementList;

public:
    OrderedSet<Element *> parents_, childs_;
};

Element::Element() : d_ptr(std::make_unique<ElementPrivate>()) {}

Element::~Element() {
    removeAllParent();
    removeAllChild();
}

const std::list<Element *> &Element::childs() const {
    FCITX_D();
    return d->childs_.order();
}

void Element::addChild(Element *child) {
    addEdge(this, child, nullptr, nullptr);
}
void Element::addParent(Element *parent) {
    addEdge(parent, this, nullptr, nullptr);
}

bool Element::isChild(const Element *child) const {
    FCITX_D();
    return d->childs_.contains(const_cast<Element *>(child));
}
bool Element::isParent(const Element *parent) const {
    FCITX_D();
    return d->parents_.contains(const_cast<Element *>(parent));
}

const std::list<Element *> &Element::parents() const {
    FCITX_D();
    return d->parents_.order();
}

void Element::removeChild(Element *child) { removeEdge(this, child); }

void Element::removeParent(Element *parent) { removeEdge(parent, this); }

void Element::insertChild(Element *before, Element *child) {
    addEdge(this, child, before, nullptr);
}

void Element::insertParent(Element *before, Element *parent) {
    addEdge(parent, this, nullptr, before);
}

void Element::addEdge(Element *parent, Element *child, Element *beforeChild,
                      Element *beforeParent) {
    // Try not to invalidate the list iterator of elements.
    if (parent->d_func()->childs_.contains(child)) {
        return;
    }
    removeEdge(parent, child);
    parent->d_func()->childs_.insert(beforeChild, child);
    child->d_func()->parents_.insert(beforeParent, parent);
}

void Element::removeEdge(Element *parent, Element *child) {
    parent->d_func()->childs_.remove(child);
    child->d_func()->parents_.remove(parent);
}

void Element::removeAllParent() {
    while (parents().size()) {
        removeParent(parents().front());
    }
}

void Element::removeAllChild() {
    while (childs().size()) {
        childs().front()->removeParent(this);
    }
}
}
