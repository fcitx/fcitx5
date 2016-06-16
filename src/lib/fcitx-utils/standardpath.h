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
#include <unordered_map>
#include "macros.h"
#include "flags.h"
#include "stringutils.h"
#include "unixfd.h"

namespace fcitx {

namespace filter {

template <typename... Types>
class Chainer;

template <>
class Chainer<> {
public:
    bool operator()(const std::string &, const std::string &, bool) { return true; }
};

template <typename First, typename... Rest>
class Chainer<First, Rest...> : Chainer<Rest...> {
    typedef Chainer<Rest...> super_class;

public:
    Chainer(First first, Rest... rest) : super_class(rest...), filter(first) {}

    bool operator()(const std::string &path, const std::string &dir, bool user) {
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

    bool operator()(const std::string &path, const std::string &dir, bool isUser) { return !filter(path, dir, isUser); }

private:
    T filter;
};

template <typename T>
NotFilter<T> Not(T t) {
    return {t};
}

struct FCITXUTILS_EXPORT User {
    bool operator()(const std::string &, const std::string &, bool isUser) { return isUser; }
};

struct FCITXUTILS_EXPORT Prefix {
    Prefix(const std::string &prefix_) : prefix(prefix_) {}

    bool operator()(const std::string &path, const std::string &, bool) {
        return stringutils::startsWith(path, prefix);
    }

    std::string prefix;
};

struct FCITXUTILS_EXPORT Suffix {
    Suffix(const std::string &suffix_) : suffix(suffix_) {}

    bool operator()(const std::string &path, const std::string &, bool) { return stringutils::endsWith(path, suffix); }

    std::string suffix;
};
}

class FCITXUTILS_EXPORT StandardPathTempFile
{
public:
    StandardPathTempFile(int fd = -1, const std::string &realFile = {}, const std::string &tempPath = {}) : m_fd(UnixFD::own(fd)), m_path(realFile), m_tempPath(tempPath) { }
    StandardPathTempFile(StandardPathTempFile &&other) = default;
    virtual ~StandardPathTempFile();

    int fd() const {
        return m_fd.fd();
    }

    const std::string &path() const { return m_path; }
    const std::string &tempPath() const { return m_tempPath; }

    int release();
    void close();

private:
    UnixFD m_fd;
    std::string m_path;
    std::string m_tempPath;
};

class FCITXUTILS_EXPORT StandardPathFile
{
public:
    StandardPathFile(int fd = -1, const std::string &path = {}) : m_fd(UnixFD::own(fd)), m_path(path) { }
    StandardPathFile(StandardPathFile &&other) = default;
    virtual ~StandardPathFile();

    int fd() const {
        return m_fd.fd();
    }

    const std::string &path() const { return m_path; }

    int release();

private:
    UnixFD m_fd;
    std::string m_path;
};

class StandardPathPrivate;

typedef std::unordered_map<std::string, StandardPathFile> StandardPathFileMap;
typedef std::unordered_map<std::string, std::vector<StandardPathFile>> StandardPathFilesMap;

class FCITXUTILS_EXPORT StandardPath {
public:
    enum class Type { Config, Data, Cache, Runtime, Addon };

    StandardPath(bool skipFcitxPath = false);
    virtual ~StandardPath();

    // return a global default so we can share it, C++11 static initialization is thread-safe
    static const StandardPath &global();

    static std::string fcitxPath(const char *path);

    void scanDirectories(Type type, std::function<bool(const std::string &path, bool user)> scanner) const;
    // scan file under dir/path, path is supposed to be a directory.
    void scanFiles(Type type, const std::string &path,
                   std::function<bool(const std::string &path, const std::string &dir, bool user)> scanner) const;

    std::string userDirectory(Type type) const;
    std::vector<std::string> directories(Type type) const;

    std::string locate(Type type, const std::string &path) const;
    std::vector<std::string> locateAll(Type type, const std::string &path) const;
    // Open the first matched and succeed
    StandardPathFile open(Type type, const std::string &path, int flags) const;
    StandardPathFile openUser(Type type, const std::string &path, int flags) const;
    StandardPathTempFile openUserTemp(Type type, const std::string &path) const;
    // Open first match for
    StandardPathFileMap
    multiOpenFilter(Type type, const std::string &path, int flags,
                    std::function<bool(const std::string &path, const std::string &dir, bool user)> filter) const;
    template <typename... Args>
    StandardPathFileMap multiOpen(Type type, const std::string &path, int flags, Args... args) const {
        return multiOpenFilter(type, path, flags, filter::Chainer<Args...>(args...));
    }
    StandardPathFilesMap
    multiOpenAllFilter(Type type, const std::string &path, int flags,
                       std::function<bool(const std::string &path, const std::string &dir, bool user)> filter) const;
    template <typename... Args>
    StandardPathFilesMap multiOpenAll(Type type, const std::string &path, int flags, Args... args) const {
        return multiOpenAllFilter(type, path, flags, filter::Chainer<Args...>(args...));
    }

private:
    std::unique_ptr<StandardPathPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(StandardPath);
};
}

#endif // _FCITX_UTILS_STANDARDPATH_H_
