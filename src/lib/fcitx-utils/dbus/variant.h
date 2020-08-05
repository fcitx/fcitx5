/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_DBUS_VARIANT_H_
#define _FCITX_UTILS_DBUS_VARIANT_H_

#include <memory>
#include <string>
#include "fcitxutils_export.h"
#include "message.h"

namespace fcitx {
namespace dbus {

class VariantTypeRegistryPrivate;

/// We need to "predefine some of the variant type that we want to handle".
class FCITXUTILS_EXPORT VariantTypeRegistry {
public:
    static VariantTypeRegistry &defaultRegistry();

    template <typename TypeName>
    void registerType() {
        using SignatureType = typename DBusSignatureTraits<TypeName>::signature;
        using PureType = FCITX_STRING_TO_DBUS_TYPE(SignatureType::str());
        static_assert(
            std::is_same<TypeName, PureType>::value,
            "Type is not pure enough, remove the redundant tuple from it");
        registerTypeImpl(DBusSignatureTraits<TypeName>::signature::data(),
                         std::make_shared<VariantHelper<TypeName>>());
    }

    std::shared_ptr<VariantHelperBase>
    lookupType(const std::string &signature) const;

private:
    void registerTypeImpl(const std::string &signature,
                          std::shared_ptr<VariantHelperBase>);
    VariantTypeRegistry();
    std::unique_ptr<VariantTypeRegistryPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(VariantTypeRegistry);
};

class FCITXUTILS_EXPORT Variant {
public:
    Variant() = default;
    template <
        typename Value,
        typename Dummy = std::enable_if_t<
            !std::is_same_v<std::remove_cv_t<std::remove_reference_t<Value>>,
                            Variant>,
            void>>
    explicit Variant(Value &&value) {
        setData(std::forward<Value>(value));
    }

    Variant(const Variant &v) : signature_(v.signature_), helper_(v.helper_) {
        if (helper_) {
            data_ = helper_->copy(v.data_.get());
        }
    }

    Variant(Variant &&v) = default;
    Variant &operator=(const Variant &v) {
        if (&v == this) {
            return *this;
        }
        signature_ = v.signature_;
        helper_ = v.helper_;
        if (helper_) {
            data_ = helper_->copy(v.data_.get());
        }
        return *this;
    }
    Variant &operator=(Variant &&v) = default;

    template <typename Value,
              typename = std::enable_if_t<!std::is_same<
                  std::remove_cv_t<std::remove_reference_t<Value>>,
                  dbus::Variant>::value>>
    void setData(Value &&value);

    void setData(const Variant &v) { *this = v; }

    void setData(Variant &&v) { *this = std::move(v); }

    void setData(const char *str) { setData(std::string(str)); }

    void setRawData(std::shared_ptr<void> data,
                    std::shared_ptr<VariantHelperBase> helper) {
        data_ = std::move(data);
        helper_ = std::move(helper);
        if (helper_) {
            signature_ = helper_->signature();
        }
    }

    template <typename Value>
    const Value &dataAs() const {
        assert(signature() == DBusSignatureTraits<Value>::signature::data());
        return *static_cast<Value *>(data_.get());
    }

    void writeToMessage(dbus::Message &msg) const;

    const std::string &signature() const { return signature_; }

    void printData(LogMessageBuilder &builder) const {
        if (helper_) {
            helper_->print(builder, data_.get());
        }
    }

private:
    std::string signature_;
    std::shared_ptr<void> data_;
    std::shared_ptr<const VariantHelperBase> helper_;
};

template <typename Value, typename>
void Variant::setData(Value &&value) {
    typedef std::remove_cv_t<std::remove_reference_t<Value>> value_type;
    signature_ = DBusSignatureTraits<value_type>::signature::data();
    data_ = std::make_shared<value_type>(std::forward<Value>(value));
    helper_ = std::make_shared<VariantHelper<value_type>>();
}

static inline LogMessageBuilder &operator<<(LogMessageBuilder &builder,
                                            const Variant &var) {
    builder << "Variant(sig=" << var.signature() << ", content=";
    var.printData(builder);
    builder << ")";
    return builder;
}

} // namespace dbus
} // namespace fcitx

#endif // _FCITX_UTILS_DBUS_VARIANT_H_
