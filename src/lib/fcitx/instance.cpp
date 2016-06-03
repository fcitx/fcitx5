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

#include "instance.h"

namespace fcitx {

class InstancePrivate {
public:
    EventLoop *eventLoop;
    InputContextManager *icManager;
    AddonManager* addonManager;
};

Instance::Instance() {
}

Instance::~Instance()
{
}


EventLoop *Instance::eventLoop() {
    FCITX_D();
    return d->eventLoop;
}

InputContextManager *Instance::inputContextManager() {
    FCITX_D();
    return d->icManager;
}

AddonManager * Instance::addonManager()
{
    FCITX_D();
    return d->addonManager;
}

void Instance::activate()
{
}

std::string Instance::addonForInputMethod(const std::string& imName)
{
    return { };
}

void Instance::configure()
{
}

void Instance::configureAddon(const std::string& addon)
{
}

void Instance::configureInputMethod(const std::string& imName)
{
}

std::string Instance::currentInputMethod()
{
    // FIXME
    return {};
}

std::string Instance::currentUI()
{
    // FIXME
    return {};
}

void Instance::deactivate()
{
}

void Instance::exit()
{
}

void Instance::reloadAddonConfig(const std::string& addonName)
{
}

void Instance::reloadConfig()
{
}

void Instance::resetInputMethodList()
{
}

void Instance::restart()
{
}

void Instance::setCurrentInputMethod(const std::string &imName)
{
}

int Instance::state()
{
    return 0;
}

void Instance::toggle()
{
}

}
