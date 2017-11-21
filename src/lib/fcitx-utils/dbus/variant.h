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
#ifndef _FCITX_UTILS_DBUS_VARIANT_H_
#define _FCITX_UTILS_DBUS_VARIANT_H_

#include "fcitxutils_export.h"
#include "message.h"
#include <memory>
#include <string>

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
    template <typename Value>
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
        data_ = data;
        helper_ = helper;
        if (helper_) {
            signature_ = helper->signature();
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
