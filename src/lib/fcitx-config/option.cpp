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

#include "option.h"
#include "configuration.h"

namespace fcitx
{
OptionBase::OptionBase(Configuration *parent, std::string path, std::string description) :
    m_parent(parent), m_path(path), m_description(description)
{
    m_parent->addOption(this);
}

OptionBase::~OptionBase()
{

}


bool OptionBase::isDefault() const
{
    return false;
}

const std::string& OptionBase::path() const
{
    return m_path;
}

const std::string& OptionBase::description() const
{
    return m_description;
}

}
