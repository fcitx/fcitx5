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
#ifndef _FCITX_CONFIG_OPTION_H_
#define _FCITX_CONFIG_OPTION_H_

#include "fcitxconfig_export.h"

#include <limits>
#include <string>
#include <functional>
#include <type_traits>

#include "optiontypename.h"
#include "rawconfig.h"
#include "marshallfunction.h"

namespace fcitx
{

class Configuration;

class FCITXCONFIG_EXPORT OptionBase
{
public:
    OptionBase(Configuration *parent, std::string path, std::string description);
    virtual ~OptionBase();

    const std::string &path() const;
    const std::string &description() const;
    virtual std::string typeString() const = 0;
    virtual void reset() = 0;
    virtual bool isDefault() const = 0;

    virtual void marshall(RawConfig &config) const = 0;
    virtual bool unmarshall(const RawConfig & config) = 0;
    virtual Configuration *subConfigSkeleton() const = 0;

    virtual bool equalTo(const OptionBase &other) const = 0;
    virtual void copyFrom(const OptionBase &other) = 0;
    bool operator==(const OptionBase &other) const {
        return equalTo(other);
    }
    bool operator!=(const OptionBase &other) const {
        return !operator==(other);
    }

    virtual void dumpDescription(RawConfig &config) const;

private:
    Configuration *m_parent;
    std::string m_path;
    std::string m_description;
};

template<typename T>
struct NoConstrain
{
    bool check(const T &) const { return true; }
    void dumpDescription(RawConfig &) const { }
};

class IntConstrain
{
public:
    IntConstrain(int min = std::numeric_limits<int>::min(), int max = std::numeric_limits<int>::max()) : m_min(min), m_max(max) {
    }
    bool check(int value) const { return value >= m_min && value <= m_max; }
    void dumpDescription(RawConfig &config) const {
        marshallOption(config["IntMin"], m_min);
        marshallOption(config["IntMax"], m_max);
    }
private:
    int m_min;
    int m_max;
};

template<typename T>
struct DefaultMarshaller
{
    virtual void marshall(RawConfig &config, const T &value) const {
        return marshallOption(config, value);
    }
    virtual bool unmarshall(T &value, const RawConfig & config) const {
        return unmarshallOption(value, config);
    }
};

template <typename T, typename = void>
struct ExtractSubConfig {
    static Configuration *get() {
        return nullptr;
    }
};

template <typename T>
struct ExtractSubConfig<std::vector<T>> {
    static Configuration *get() {
        return ExtractSubConfig<T>::get();
    }
};

template <typename T>
struct ExtractSubConfig<T, typename std::enable_if<std::is_base_of<Configuration, T>::value>::type> {
    static Configuration *get() {
        return new T;
    }
};

template<typename T, typename Constrain = NoConstrain<T>, typename Marshaller = DefaultMarshaller<T>>
class Option : public OptionBase
{
public:
    Option(Configuration *parent, std::string path, std::string description, T defaultValue = T(),
           Constrain constrain = Constrain(), Marshaller marshaller = Marshaller()) :
        OptionBase(parent, path, description)
      , m_defaultValue(defaultValue)
      , m_value(defaultValue)
      , m_marshaller(marshaller)
      , m_constrain(constrain)
    {
        if (!m_constrain.check(m_defaultValue)) {
            throw std::invalid_argument("defaultValue doesn't satisfy constrain");
        }
    }

    virtual std::string typeString() const override {
        return OptionTypeName<T>::get();
    }

    virtual void dumpDescription(RawConfig& config) const override {
        OptionBase::dumpDescription(config);
        m_marshaller.marshall(config["DefaultValue"], m_defaultValue);
        m_constrain.dumpDescription(config);
    }

    virtual Configuration* subConfigSkeleton() const override {
        return ExtractSubConfig<T>::get();
    }

    virtual bool isDefault() const override {
        return m_defaultValue == m_value;
    }

    virtual void reset() override {
        m_value = m_defaultValue;
    }

    const T &value() {
        return m_value;
    }

    const T &defaultValue() {
        return m_defaultValue;
    }

    bool setValue(const T &value) {
        if (!m_constrain.check(value)) {
            return false;
        }
        m_value = value;
        return true;
    }

    void marshall(RawConfig &config) const override{
        return m_marshaller.marshall(config, m_value);
    }
    bool unmarshall(const RawConfig & config) override{
        T tempValue;
        if (!m_marshaller.unmarshall(m_value, config)) {
            return false;
        }
        return setValue(tempValue);
    }

    virtual bool equalTo(const OptionBase& other) const override {
        auto otherP = reinterpret_cast<const Option<T, Constrain, Marshaller>*>(&other);
        return m_value == otherP->m_value;
    }

    virtual void copyFrom(const OptionBase& other) override {
        auto otherP = reinterpret_cast<const Option<T, Constrain, Marshaller>*>(&other);
        m_value = otherP->m_value;
    }



private:
    T m_defaultValue;
    T m_value;
    Marshaller m_marshaller;
    Constrain m_constrain;
};

}

#endif // _FCITX_CONFIG_OPTION_H_
