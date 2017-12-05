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
    virtual bool unmarshall(const RawConfig &config, bool partial) = 0;
    virtual std::unique_ptr<Configuration> subConfigSkeleton() const = 0;

    virtual bool equalTo(const OptionBase &other) const = 0;
    virtual void copyFrom(const OptionBase &other) = 0;
    bool operator==(const OptionBase &other) const { return equalTo(other); }
    bool operator!=(const OptionBase &other) const {
        return !operator==(other);
    }

    virtual bool skipDescription() const = 0;
    virtual bool skipSave() const = 0;
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

struct NoAnnotation {
    bool skipDescription() const { return false; }
    bool skipSave() const { return false; }
    void dumpDescription(RawConfig &) const {}
};

struct HideInDescription {
    bool skipDescription() const { return true; }
    bool skipSave() const { return false; }
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
    virtual bool unmarshall(T &value, const RawConfig &config,
                            bool partial) const {
        return unmarshallOption(value, config, partial);
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
    static std::unique_ptr<Configuration> get() { return nullptr; }
};

template <typename T>
struct ExtractSubConfig<std::vector<T>> {
    static std::unique_ptr<Configuration> get() {
        return ExtractSubConfig<T>::get();
    }
};

template <typename T>
struct ExtractSubConfig<
    T,
    typename std::enable_if<std::is_base_of<Configuration, T>::value>::type> {
    static std::unique_ptr<Configuration> get() {
        return std::make_unique<T>();
    }
};

template <typename T>
void dumpDescriptionHelper(RawConfig &, T *) {}

template <typename OptionType>
class MutableOption {
public:
    using value_type = typename OptionType::value_type;

    MutableOption(OptionType *option = nullptr)
        : option_(option), value_(option ? option->value() : value_type()) {}

    ~MutableOption() {
        if (option_) {
            option_->setValue(std::move(value_));
        }
    }

    MutableOption(MutableOption &&other)
        : option_(other.option_), value_(std::move(other.value_)) {
        other.option_ = nullptr;
    }

    MutableOption &operator=(MutableOption &&other) {
        option_ = other.option_;
        value_ = std::move(other.value_);
        other.option_ = nullptr;
    }

    value_type &operator*() { return value_; }
    value_type *operator->() { return &value_; }

private:
    OptionType *option_;
    value_type value_;
};

template <typename T, typename Constrain = NoConstrain<T>,
          typename Marshaller = DefaultMarshaller<T>,
          typename Annotation = NoAnnotation>
class Option : public OptionBase {
public:
    using value_type = T;
    using constrain_type = Constrain;

    Option(Configuration *parent, std::string path, std::string description,
           const T &defaultValue = T(), Constrain constrain = Constrain(),
           Marshaller marshaller = Marshaller(),
           Annotation annotation = Annotation())
        : OptionBase(parent, path, description), defaultValue_(defaultValue),
          value_(defaultValue), marshaller_(marshaller), constrain_(constrain),
          annotation_(annotation) {
        if (!constrain_.check(defaultValue_)) {
            throw std::invalid_argument(
                "defaultValue doesn't satisfy constrain");
        }
    }

    std::string typeString() const override { return OptionTypeName<T>::get(); }

    void dumpDescription(RawConfig &config) const override {
        OptionBase::dumpDescription(config);
        marshaller_.marshall(config["DefaultValue"], defaultValue_);
        constrain_.dumpDescription(config);
        annotation_.dumpDescription(config);
        using ::fcitx::dumpDescriptionHelper;
        dumpDescriptionHelper(
            config, static_cast<typename RemoveVector<T>::type *>(nullptr));
    }

    std::unique_ptr<Configuration> subConfigSkeleton() const override {
        return ExtractSubConfig<T>::get();
    }

    bool isDefault() const override { return defaultValue_ == value_; }
    void reset() override { value_ = defaultValue_; }

    const T &value() const { return value_; }

    const T &defaultValue() const { return defaultValue_; }

    const T &operator*() const { return value(); }
    const T *operator->() const { return &value_; }

    template <typename U>
    bool setValue(U &&value) {
        if (!constrain_.check(value)) {
            return false;
        }
        value_ = std::forward<U>(value);
        return true;
    }

    template <typename Dummy = int,
              std::enable_if_t<!std::is_same<Constrain, NoConstrain<T>>::value,
                               Dummy> = 0>
    MutableOption<Option> mutableValue() {
        return {this};
    }

    template <typename Dummy = int,
              std::enable_if_t<std::is_same<Constrain, NoConstrain<T>>::value,
                               Dummy> = 0>
    T *mutableValue() {
        return &value_;
    }

    void marshall(RawConfig &config) const override {
        return marshaller_.marshall(config, value_);
    }
    bool unmarshall(const RawConfig &config, bool partial) override {
        T tempValue{};
        if (partial) {
            tempValue = value_;
        }
        if (!marshaller_.unmarshall(tempValue, config, partial)) {
            return false;
        }
        return setValue(tempValue);
    }

    bool equalTo(const OptionBase &other) const override {
        auto otherP = static_cast<const Option *>(&other);
        return value_ == otherP->value_;
    }

    void copyFrom(const OptionBase &other) override {
        auto otherP = static_cast<const Option *>(&other);
        value_ = otherP->value_;
    }

    bool skipDescription() const override {
        return annotation_.skipDescription();
    }

    bool skipSave() const override { return annotation_.skipSave(); }

private:
    T defaultValue_;
    T value_;
    Marshaller marshaller_;
    Constrain constrain_;
    Annotation annotation_;
};

template <typename T, typename Annotation>
using OptionWithAnnotation =
    Option<T, NoConstrain<T>, DefaultMarshaller<T>, Annotation>;

template <typename T>
using HiddenOption = OptionWithAnnotation<T, HideInDescription>;
}

#endif // _FCITX_CONFIG_OPTION_H_
