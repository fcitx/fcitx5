/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_CONFIG_OPTION_DETAILS_H_
#define _FCITX_CONFIG_OPTION_DETAILS_H_

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

class FCITXCONFIG_EXPORT OptionBaseV2 : public OptionBase {
public:
    using OptionBase::OptionBase;
    virtual void syncDefaultValueToCurrent() = 0;
};

template <typename T>
struct RemoveVector {
    typedef T type;
};

template <typename T>
struct RemoveVector<std::vector<T>> {
    typedef typename RemoveVector<T>::type type;
};

template <typename T>
void dumpDescriptionHelper(RawConfig &, T *) {}

} // namespace fcitx

#endif // _FCITX_CONFIG_OPTION_DETAILS_H_
