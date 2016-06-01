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
#ifndef _FCITX_UTILS_STANDARDPATH_H_
#define _FCITX_UTILS_STANDARDPATH_H_

#include "fcitxutils_export.h"
#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <map>
#include "macros.h"
#include "flags.h"
#include "stringutils.h"

namespace fcitx {

namespace filter {

template <typename... Types>
class Chainer;

template <>
class Chainer<> {
public:
    bool operator()(const std::string &, const std::string &, bool) {
        return true;
    }
};

template <typename First, typename... Rest>
class Chainer<First, Rest...> : Chainer<Rest...> {
    typedef Chainer<Rest...> super_class;

public:
    Chainer(First first, Rest... rest) : super_class(rest...), filter(first) {}

    bool operator()(const std::string &path, const std::string &dir,
                    bool user) {
        if (!filter(path, dir, user)) {
            return false;
        }
        return super_class::operator()(path, dir, user);
    }

private:
    First filter;
};

template <typename T>
struct NotFilter {
    NotFilter(T filter_) : filter(filter_) {}

    bool operator()(const std::string &path, const std::string &dir,
                    bool isUser) {
        return !filter(path, dir, isUser);
    }

private:
    T filter;
};

template <typename T>
NotFilter<T> Not(T t) {
    return {t};
}

struct User {
    bool operator()(const std::string &, const std::string &, bool isUser) {
        return isUser;
    }
};

struct Prefix {
    Prefix(const std::string &prefix_) : prefix(prefix_) {}

    bool operator()(const std::string &path, const std::string &, bool) {
        return stringutils::startsWith(path, prefix);
    }

    std::string prefix;
};

struct Suffix {
    Suffix(const std::string &suffix_) : suffix(suffix_) {}

    bool operator()(const std::string &path, const std::string &, bool) {
        return stringutils::endsWith(path, suffix);
    }

    std::string suffix;
};
}

class StandardPathPrivate;

typedef std::pair<int, std::string> StandardPathFile;

class FCITXUTILS_EXPORT StandardPath {
public:
    enum class Type { Config, Data, Cache, Runtime, Addon };

    enum class FilterFlag {
        Writable = (1 << 0),
        Append = (1 << 1),
        Prefix = (1 << 2),
        Suffix = (1 << 3),
        Callback = (1 << 4),
        Sort = (1 << 5),
        LocateAll = (1 << 6),
        Write = Writable | Append,
    };

    typedef Flags<FilterFlag> FilterFlags;

    StandardPath(bool skipFcitxPath = false);
    virtual ~StandardPath();

    static std::string fcitxPath(const char *path);

    void scanDirectories(
        Type type,
        std::function<bool(const std::string &path, bool user)> scanner);
    // scan file under dir/path, path is supposed to be a directory.
    void
    scanFiles(Type type, const std::string &path,
              std::function<bool(const std::string &path,
                                 const std::string &dir, bool user)> scanner);

    std::string userDirectory(Type type) const;
    std::vector<std::string> directories(Type type) const;

    // Open the first matched and succeed
    StandardPathFile open(Type type, const std::string &path, int flags);
    // Open first match for
    std::map<std::string, StandardPathFile> openMultipleFilesFilter(
        Type type, const std::string &path, int flags,
        std::function<bool(const std::string &path, const std::string &dir,
                           bool user)> filter);
    template <typename... Args>
    std::map<std::string, StandardPathFile>
    openMultipleFiles(Type type, const std::string &path, int flags,
                      Args... args) {
        return openMultipleFilesFilter(type, path, flags,
                                       filter::Chainer<Args...>(args...));
    }

private:
    std::unique_ptr<StandardPathPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(StandardPath);
};
}

#endif // _FCITX_UTILS_STANDARDPATH_H_
