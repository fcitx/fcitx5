//
// Copyright (C) 2017~2017 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//
#ifndef _FCITX_UTILS_ELEMENT_H_
#define _FCITX_UTILS_ELEMENT_H_

#include "fcitxutils_export.h"
#include <fcitx-utils/connectableobject.h>
#include <unordered_set>

/// \addtogroup FcitxUtils
/// \{
/// \file
/// \brief Utility class that provides a hierarchy between multiple objects.

namespace fcitx {

class ElementPrivate;

/// \brief Base class that can be used for UI composition or graph.
class FCITXUTILS_EXPORT Element : public ConnectableObject {
public:
    Element();
    ~Element();

    /// \brief Enable query between different elements.
    bool isChild(const Element *element) const;

    /// \brief Enable query between different elements.
    bool isParent(const Element *element) const;

protected:
    /// \brief List all parents.
    ///
    /// For the sake of type safety, list parents are protected by default.
    const std::list<Element *> &parents() const;

    /// \brief List all childs
    ///
    /// \see parents
    const std::list<Element *> &childs() const;

    // Sub class may use these functions carefully if they intends
    // to have single type of childs.
    void addChild(Element *child);
    void addParent(Element *parent);

    void insertChild(Element *before, Element *child);
    void insertParent(Element *before, Element *parent);

    void removeParent(Element *parent);
    void removeChild(Element *child);

    void removeAllChild();
    void removeAllParent();

    static void addEdge(Element *parent, Element *child, Element *beforeChild,
                        Element *beforeParent);
    static void removeEdge(Element *parent, Element *child);

private:
    std::unique_ptr<ElementPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(Element);
};
} // namespace fcitx

#endif // _FCITX_UTILS_ELEMENT_H_
