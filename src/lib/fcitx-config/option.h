/*
 * SPDX-FileCopyrightText: 2015-2015 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_CONFIG_OPTION_H_
#define _FCITX_CONFIG_OPTION_H_

#include "fcitxconfig_export.h"

#include <functional>
#include <limits>
#include <string>
#include <type_traits>
#include <fcitx-config/marshallfunction.h>
#include <fcitx-config/option_details.h>
#include <fcitx-config/optiontypename.h>
#include <fcitx-config/rawconfig.h>

namespace fcitx {

/// An option that launches external tool.
class FCITXCONFIG_EXPORT ExternalOption : public OptionBase {
public:
    ExternalOption(Configuration *parent, std::string path,
                   std::string description, std::string uri);

    std::string typeString() const override;
    void reset() override;
    bool isDefault() const override;

    void marshall(RawConfig &config) const override;
    bool unmarshall(const RawConfig &config, bool partial) override;
    std::unique_ptr<Configuration> subConfigSkeleton() const override;

    bool equalTo(const OptionBase &other) const override;
    void copyFrom(const OptionBase &other) override;

    bool skipDescription() const override;
    bool skipSave() const override;
    void dumpDescription(RawConfig &config) const override;

private:
    std::string externalUri_;
};

/// An option that launches external tool.
class FCITXCONFIG_EXPORT SubConfigOption : public ExternalOption {
public:
    using ExternalOption::ExternalOption;
    void dumpDescription(RawConfig &config) const override;
};

/// Default Constrain with no actual constrain.
template <typename T>
struct NoConstrain {
    using Type = T;
    bool check(const T &) const { return true; }
    void dumpDescription(RawConfig &) const {}
};

/// Default Annotation with no options.
struct NoAnnotation {
    bool skipDescription() { return false; }
    bool skipSave() { return false; }
    void dumpDescription(RawConfig &) const {}
};

/// Annotation to display a tooltip in configtool.
struct ToolTipAnnotation {
    ToolTipAnnotation(std::string tooltip) : tooltip_(std::move(tooltip)) {}

    bool skipDescription() { return false; }
    bool skipSave() { return false; }
    void dumpDescription(RawConfig &config) const {
        config.setValueByPath("Tooltip", tooltip_);
    }

private:
    std::string tooltip_;
};

/// For a list of sub config, the field that should be used for display.
struct ListDisplayOptionAnnotation {
    ListDisplayOptionAnnotation(std::string option)
        : option_(std::move(option)) {}

    bool skipDescription() { return false; }
    bool skipSave() { return false; }
    void dumpDescription(RawConfig &config) const {
        config.setValueByPath("ListDisplayOption", option_);
    }

private:
    std::string option_;
};

/**
 * Annotation to be used against String type to indicate this is a Font.
 */
struct FontAnnotation {
    bool skipDescription() { return false; }
    bool skipSave() { return false; }
    void dumpDescription(RawConfig &config) {
        config.setValueByPath("Font", "True");
    }
};

/**
 * Annotation to be used against String type, for those type of string
 * that should shown as a combobox, but the value is run time based.
 * User of this annotation should take a sub class of it and set
 * Enum/n, and EnumI18n/n correspondingly.
 */
struct EnumAnnotation {
    bool skipDescription() { return false; }
    bool skipSave() { return false; }
    void dumpDescription(RawConfig &config) const {
        config.setValueByPath("IsEnum", "True");
    }
};

/**
 * Option that will not shown in UI.
 *
 * You may want to use HiddenOption instead.
 *
 * @see HiddenOption
 */
struct HideInDescription {
    bool skipDescription() { return true; }
    bool skipSave() { return false; }
    void dumpDescription(RawConfig &) const {}
};

template <typename Annotation>
struct HideInDescriptionAnnotation : public Annotation {
    using Annotation::Annotation;
    bool skipDescription() { return true; }
    using Annotation::dumpDescription;
    using Annotation::skipSave;
};

/**
 * List Constrain that applies the constrain to all element.
 */
template <typename SubConstrain>
struct ListConstrain {
    ListConstrain(SubConstrain sub = SubConstrain()) : sub_(std::move(sub)) {}

    using ElementType = typename SubConstrain::Type;
    using Type = std::vector<ElementType>;
    bool check(const Type &value) {
        return std::all_of(
            value.begin(), value.end(),
            [this](const ElementType &ele) { return sub_.check(ele); });
    }

    void dumpDescription(RawConfig &config) const {
        sub_.dumpDescription(*config.get("ListConstrain", true));
    }

private:
    SubConstrain sub_;
};

/// Integer type constrain with a lower and a upper bound.
class IntConstrain {
public:
    using Type = int;
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

/// Key option constrain flag.
enum class KeyConstrainFlag {
    /// The key can be modifier only, like Control_L.
    AllowModifierOnly = (1 << 0),
    /// The key can be modifier less (Key that usually produce character).
    AllowModifierLess = (1 << 1),
};

using KeyConstrainFlags = Flags<KeyConstrainFlag>;

/// Key option constrain.
class KeyConstrain {
public:
    using Type = Key;
    KeyConstrain(KeyConstrainFlags flags) : flags_(flags) {}

    bool check(const Key &key) const {
        if (!flags_.test(KeyConstrainFlag::AllowModifierLess) &&
            key.states() == 0) {
            return false;
        }

        if (!flags_.test(KeyConstrainFlag::AllowModifierOnly) &&
            key.isModifier()) {
            return false;
        }

        return true;
    }

    void dumpDescription(RawConfig &config) const {
        if (flags_.test(KeyConstrainFlag::AllowModifierLess)) {
            config["AllowModifierLess"] = "True";
        }
        if (flags_.test(KeyConstrainFlag::AllowModifierOnly)) {
            config["AllowModifierOnly"] = "True";
        }
    }

private:
    KeyConstrainFlags flags_;
};

/// Default marshaller that write the config RawConfig.
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

/// A helper class provide writing ability to option value.
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

    MutableOption(MutableOption &&other) noexcept
        : option_(other.option_), value_(std::move(other.value_)) {
        other.option_ = nullptr;
    }

    MutableOption &operator=(MutableOption &&other) noexcept {
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

/**
 * Represent a Configuration option.
 *
 */
template <typename T, typename Constrain = NoConstrain<T>,
          typename Marshaller = DefaultMarshaller<T>,
          typename Annotation = NoAnnotation>
class Option : public OptionBaseV2 {
public:
    using value_type = T;
    using constrain_type = Constrain;

    Option(Configuration *parent, std::string path, std::string description,
           const T &defaultValue = T(), Constrain constrain = Constrain(),
           Marshaller marshaller = Marshaller(),
           Annotation annotation = Annotation())
        : OptionBaseV2(parent, std::move(path), std::move(description)),
          defaultValue_(defaultValue), value_(defaultValue),
          marshaller_(marshaller), constrain_(constrain),
          annotation_(annotation) {
        if (!constrain_.check(defaultValue_)) {
            throw std::invalid_argument(
                "defaultValue doesn't satisfy constrain");
        }
    }

    std::string typeString() const override { return OptionTypeName<T>::get(); }

    void dumpDescription(RawConfig &config) const override {
        OptionBase::dumpDescription(config);
        if constexpr (not std::is_base_of_v<Configuration, T>) {
            marshaller_.marshall(config["DefaultValue"], defaultValue_);
        }
        constrain_.dumpDescription(config);
        annotation_.dumpDescription(config);
        using ::fcitx::dumpDescriptionHelper;
        dumpDescriptionHelper(
            config, static_cast<typename RemoveVector<T>::type *>(nullptr));
    }

    std::unique_ptr<Configuration> subConfigSkeleton() const override {
        if constexpr (std::is_base_of_v<Configuration, T>) {
            auto skeleton = std::make_unique<T>(defaultValue_);
            skeleton->syncDefaultValueToCurrent();
            return skeleton;
        }
        if constexpr (std::is_base_of_v<Configuration,
                                        typename RemoveVector<T>::type>) {
            return std::make_unique<typename RemoveVector<T>::type>();
        }

        return nullptr;
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

    void syncDefaultValueToCurrent() override {
        defaultValue_ = value_;
        if constexpr (std::is_base_of_v<Configuration, T>) {
            value_.syncDefaultValueToCurrent();
            defaultValue_.syncDefaultValueToCurrent();
        }
    }

    auto &annotation() const { return annotation_; }

private:
    T defaultValue_;
    T value_;
    Marshaller marshaller_;
    Constrain constrain_;
    mutable Annotation annotation_;
};

/// Shorthand if you want a option type with only custom annotation.
template <typename T, typename Annotation>
using OptionWithAnnotation =
    Option<T, NoConstrain<T>, DefaultMarshaller<T>, Annotation>;

/// Shorthand for KeyList option with constrain.
using KeyListOption = Option<KeyList, ListConstrain<KeyConstrain>,
                             DefaultMarshaller<KeyList>, NoAnnotation>;

/// Shorthand for create a key list constrain.
static inline ListConstrain<KeyConstrain>
KeyListConstrain(KeyConstrainFlags flags = KeyConstrainFlags()) {
    return ListConstrain<KeyConstrain>(KeyConstrain(flags));
}

/// Shorthand for option that will not show in UI.
template <typename T, typename Constrain = NoConstrain<T>,
          typename Marshaller = DefaultMarshaller<T>,
          typename Annotation = NoAnnotation>
using HiddenOption =
    Option<T, Constrain, Marshaller, HideInDescriptionAnnotation<Annotation>>;

template <bool hidden, typename T>
struct ConditionalHiddenHelper;

template <typename T, typename Constrain, typename Marshaller,
          typename Annotation>
struct ConditionalHiddenHelper<false,
                               Option<T, Constrain, Marshaller, Annotation>> {
    using OptionType = Option<T, Constrain, Marshaller, Annotation>;
};

template <typename T, typename Constrain, typename Marshaller,
          typename Annotation>
struct ConditionalHiddenHelper<true,
                               Option<T, Constrain, Marshaller, Annotation>> {
    using OptionType = HiddenOption<T, Constrain, Marshaller, Annotation>;
};

template <bool hidden, typename T>
using ConditionalHidden =
    typename ConditionalHiddenHelper<hidden, T>::OptionType;

} // namespace fcitx

#endif // _FCITX_CONFIG_OPTION_H_
