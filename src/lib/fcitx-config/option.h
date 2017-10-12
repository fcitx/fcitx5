/*
 * Copyright (C) 2015~2015 by CSSlayer
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
#ifndef _FCITX_CONFIG_OPTION_H_
#define _FCITX_CONFIG_OPTION_H_

#include "fcitxconfig_export.h"

#include <functional>
#include <limits>
#include <string>
#include <type_traits>

#include <fcitx-config/marshallfunction.h>
#include <fcitx-config/optiontypename.h>
#include <fcitx-config/rawconfig.h>

namespace fcitx {

class Configuration;

class FCITXCONFIG_EXPORT OptionBase {
public:
    OptionBase(Configuration *parent, std::string path,
               std::string description);
    virtual ~OptionBase();

    const std::string &path() const;
    const std::string &description() const;
    virtual std::string typeString() const = 0;
    virtual void reset() = 0;
    virtual bool isDefault() const = 0;

    virtual void marshall(RawConfig &config) const = 0;
    virtual bool unmarshall(const RawConfig &config) = 0;
    virtual Configuration *subConfigSkeleton() const = 0;

    virtual bool equalTo(const OptionBase &other) const = 0;
    virtual void copyFrom(const OptionBase &other) = 0;
    bool operator==(const OptionBase &other) const { return equalTo(other); }
    bool operator!=(const OptionBase &other) const {
        return !operator==(other);
    }

    virtual void dumpDescription(RawConfig &config) const;

private:
    Configuration *parent_;
    std::string path_;
    std::string description_;
};

template <typename T>
struct NoConstrain {
    bool check(const T &) const { return true; }
    void dumpDescription(RawConfig &) const {}
};

class IntConstrain {
public:
    IntConstrain(int min = std::numeric_limits<int>::min(),
                 int max = std::numeric_limits<int>::max())
        : min_(min), max_(max) {}
    bool check(int value) const { return value >= min_ && value <= max_; }
    void dumpDescription(RawConfig &config) const {
        if (min_ != std::numeric_limits<int>::min()) {
            marshallOption(config["IntMin"], min_);
        }
        if (max_ != std::numeric_limits<int>::max()) {
            marshallOption(config["IntMax"], max_);
        }
    }

private:
    int min_;
    int max_;
};

template <typename T>
struct DefaultMarshaller {
    virtual void marshall(RawConfig &config, const T &value) const {
        return marshallOption(config, value);
    }
    virtual bool unmarshall(T &value, const RawConfig &config) const {
        return unmarshallOption(value, config);
    }
};

template <typename T>
struct RemoveVector {
    typedef T type;
};

template <typename T>
struct RemoveVector<std::vector<T>> {
    typedef typename RemoveVector<T>::type type;
};

template <typename T, typename = void>
struct ExtractSubConfig {
    static Configuration *get() { return nullptr; }
};

template <typename T>
struct ExtractSubConfig<std::vector<T>> {
    static Configuration *get() { return ExtractSubConfig<T>::get(); }
};

template <typename T>
struct ExtractSubConfig<
    T,
    typename std::enable_if<std::is_base_of<Configuration, T>::value>::type> {
    static Configuration *get() { return new T; }
};

template <typename T>
void dumpDescriptionHelper(RawConfig &, T *) {}

template <typename T, typename Constrain = NoConstrain<T>,
          typename Marshaller = DefaultMarshaller<T>>
class Option : public OptionBase {
public:
    Option(Configuration *parent, std::string path, std::string description,
           const T &defaultValue = T(), Constrain constrain = Constrain(),
           Marshaller marshaller = Marshaller())
        : OptionBase(parent, path, description), defaultValue_(defaultValue),
          value_(defaultValue), marshaller_(marshaller), constrain_(constrain) {
        if (!constrain_.check(defaultValue_)) {
            throw std::invalid_argument(
                "defaultValue doesn't satisfy constrain");
        }
    }

    virtual std::string typeString() const override {
        return OptionTypeName<T>::get();
    }

    virtual void dumpDescription(RawConfig &config) const override {
        OptionBase::dumpDescription(config);
        marshaller_.marshall(config["DefaultValue"], defaultValue_);
        constrain_.dumpDescription(config);
        using ::fcitx::dumpDescriptionHelper;
        dumpDescriptionHelper(
            config, static_cast<typename RemoveVector<T>::type *>(nullptr));
    }

    virtual Configuration *subConfigSkeleton() const override {
        return ExtractSubConfig<T>::get();
    }

    virtual bool isDefault() const override { return defaultValue_ == value_; }

    virtual void reset() override { value_ = defaultValue_; }

    const T &value() const { return value_; }

    const T &defaultValue() const { return defaultValue_; }

    const T &operator*() const { return value(); }
    const T *operator->() const { return &value_; }

    template <typename U>
    bool setValue(U &&value) {
        if (!constrain_.check(value)) {
            return false;
        }
        value_ = value;
        return true;
    }

    void marshall(RawConfig &config) const override {
        return marshaller_.marshall(config, value_);
    }
    bool unmarshall(const RawConfig &config) override {
        T tempValue{};
        if (!marshaller_.unmarshall(tempValue, config)) {
            return false;
        }
        return setValue(tempValue);
    }

    virtual bool equalTo(const OptionBase &other) const override {
        auto otherP = static_cast<const Option *>(&other);
        return value_ == otherP->value_;
    }

    virtual void copyFrom(const OptionBase &other) override {
        auto otherP = static_cast<const Option *>(&other);
        value_ = otherP->value_;
    }

private:
    T defaultValue_;
    T value_;
    Marshaller marshaller_;
    Constrain constrain_;
};
}

#endif // _FCITX_CONFIG_OPTION_H_
