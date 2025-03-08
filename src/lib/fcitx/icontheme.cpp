/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "icontheme.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <ios>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>
#include "fcitx-config/iniparser.h"
#include "fcitx-config/marshallfunction.h"
#include "fcitx-config/rawconfig.h"
#include "fcitx-utils/environ.h"
#include "fcitx-utils/fs.h"
#include "fcitx-utils/i18nstring.h"
#include "fcitx-utils/macros.h"
#include "fcitx-utils/standardpath.h"
#include "fcitx-utils/stringutils.h"
#include "config.h" // IWYU pragma: keep
#include "misc_p.h"

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

namespace fcitx {

std::string pathToRoot(const RawConfig &config) {
    std::string path;
    const auto *pConfig = &config;
    size_t length = 0;
    while (pConfig) {
        if (pConfig->parent() && length) {
            length += 1; // For "/";
        }
        length += pConfig->name().size();
        pConfig = pConfig->parent();
    }

    pConfig = &config;
    path.resize(length);
    size_t currentLength = 0;
    while (pConfig) {
        if (pConfig->parent() && currentLength) {
            currentLength += 1; // For "/";
            path[length - currentLength] = '/';
        }
        const auto &seg = pConfig->name();
        currentLength += seg.size();
        path.replace(length - currentLength, seg.size(), seg);
        pConfig = pConfig->parent();
    }
    return path;
}

class IconThemeDirectoryPrivate {
public:
    IconThemeDirectoryPrivate(const RawConfig &config)
        : path_(pathToRoot(config)) {
        if (path_.empty() || path_[0] == '/') {
            throw std::invalid_argument("Invalid path.");
        }

        if (auto subConfig = config.get("Size")) {
            unmarshallOption(size_, *subConfig, false);
        }
        if (size_ <= 0) {
            throw std::invalid_argument("Invalid size");
        }

        if (auto subConfig = config.get("Scale")) {
            unmarshallOption(scale_, *subConfig, false);
        }
        if (auto subConfig = config.get("Context")) {
            unmarshallOption(context_, *subConfig, false);
        }
        if (auto subConfig = config.get("Type")) {
            unmarshallOption(type_, *subConfig, false);
        }
        if (auto subConfig = config.get("MaxSize")) {
            unmarshallOption(maxSize_, *subConfig, false);
        }
        if (auto subConfig = config.get("MinSize")) {
            unmarshallOption(minSize_, *subConfig, false);
        }
        if (auto subConfig = config.get("Threshold")) {
            unmarshallOption(threshold_, *subConfig, false);
        }

        if (maxSize_ <= 0) {
            maxSize_ = size_;
        }

        if (minSize_ <= 0) {
            minSize_ = size_;
        }
    }

    FCITX_INLINE_DEFINE_DEFAULT_DTOR_COPY_AND_MOVE_WITHOUT_SPEC(
        IconThemeDirectoryPrivate);

    std::string path_;
    int size_ = 0;
    int scale_ = 1;
    std::string context_;
    IconThemeDirectoryType type_ = IconThemeDirectoryType::Threshold;
    int maxSize_ = 0;
    int minSize_ = 0;
    int threshold_ = 2;
};

IconThemeDirectory::IconThemeDirectory(const RawConfig &config)
    : d_ptr(std::make_unique<IconThemeDirectoryPrivate>(config)) {}

FCITX_DEFINE_DPTR_COPY_AND_DEFAULT_DTOR_AND_MOVE(IconThemeDirectory);

bool IconThemeDirectory::matchesSize(int iconsize, int iconscale) const {
    if (scale() != iconscale) {
        return false;
    }
    switch (type()) {
    case IconThemeDirectoryType::Fixed:
        return iconsize == size();
    case IconThemeDirectoryType::Scalable:
        return minSize() <= iconsize && iconsize <= maxSize();
    case IconThemeDirectoryType::Threshold:
        return size() - threshold() <= iconsize &&
               iconsize <= size() + threshold();
    }
    return false;
}

int IconThemeDirectory::sizeDistance(int iconsize, int iconscale) const {
    switch (type()) {
    case IconThemeDirectoryType::Fixed:
        return std::abs((size() * scale()) - (iconsize * iconscale));
    case IconThemeDirectoryType::Scalable:
        if (iconsize * iconscale < minSize() * scale()) {
            return (minSize() * scale()) - (iconsize * iconscale);
        }
        if (iconsize * iconscale > maxSize() * scale()) {
            return (iconsize * iconscale) - (maxSize() * scale());
        }
        return 0;
    case IconThemeDirectoryType::Threshold:
        if (iconsize * iconscale < (size() - threshold()) * scale()) {
            return ((size() - threshold()) * scale()) - (iconsize * iconscale);
        }
        if (iconsize * iconscale > (size() + threshold()) * scale()) {
            return (iconsize * iconscale) - ((size() - threshold()) * scale());
        }
    }
    return 0;
}

FCITX_DEFINE_READ_ONLY_PROPERTY_PRIVATE(IconThemeDirectory, std::string, path);
FCITX_DEFINE_READ_ONLY_PROPERTY_PRIVATE(IconThemeDirectory, int, size);
FCITX_DEFINE_READ_ONLY_PROPERTY_PRIVATE(IconThemeDirectory, int, scale);
FCITX_DEFINE_READ_ONLY_PROPERTY_PRIVATE(IconThemeDirectory, std::string,
                                        context);
FCITX_DEFINE_READ_ONLY_PROPERTY_PRIVATE(IconThemeDirectory,
                                        IconThemeDirectoryType, type);
FCITX_DEFINE_READ_ONLY_PROPERTY_PRIVATE(IconThemeDirectory, int, maxSize);
FCITX_DEFINE_READ_ONLY_PROPERTY_PRIVATE(IconThemeDirectory, int, minSize);
FCITX_DEFINE_READ_ONLY_PROPERTY_PRIVATE(IconThemeDirectory, int, threshold);

static uint32_t iconNameHash(const char *p) {
    uint32_t h = static_cast<signed char>(*p);
    for (p += 1; *p != '\0'; p++) {
        h = (h << 5) - h + *p;
    }
    return h;
}

class IconThemeCache {
public:
    IconThemeCache(const std::filesystem::path &filename) {
#ifdef _WIN32
        FCITX_UNUSED(filename);
#else
        auto fd = UnixFD::own(open(filename.c_str(), O_RDONLY));
        if (!fd.isValid()) {
            return;
        }
        auto fileModifiedTime = fs::modifiedTime(filename);
        if (fileModifiedTime == 0) {
            return;
        }
        auto dirName = filename.parent_path();
        auto dirModifiedTime = fs::modifiedTime(dirName);
        if (dirModifiedTime == 0) {
            return;
        }
        if (fileModifiedTime < dirModifiedTime) {
            return;
        }

        struct stat st;
        if (fstat(fd.fd(), &st) < 0) {
            return;
        }

        memory_ = static_cast<uint8_t *>(
            mmap(nullptr, st.st_size, PROT_READ, MAP_SHARED, fd.fd(), 0));
        if (!memory_) {
            // Failed to mmap is not critical here.
            return;
        }
        size_ = st.st_size;

        if (readWord(0) != 1 || readWord(2) != 0) { // version major minor
            return;
        }

        isValid_ = true;

        // Check that all the directories are older than the cache
        uint32_t dirListOffset = readDoubleWord(8);
        uint32_t dirListLen = readDoubleWord(dirListOffset);
        // We assume it won't be so long just in case we hit a corruptted file.
        if (dirListLen > 4096) {
            isValid_ = false;
            return;
        }
        for (uint32_t i = 0; i < dirListLen; ++i) {
            uint32_t offset = readDoubleWord(dirListOffset + 4 + (4 * i));
            if (!isValid_ || offset >= size_) {
                isValid_ = false;
                return;
            }
            auto *dir = checkString(offset);
            if (!dir) {
                isValid_ = false;
                return;
            }
            std::filesystem::path subdirName(dir);
            if (subdirName.is_absolute()) {
                isValid_ = false;
                return;
            }
            auto subDirTime = fs::modifiedTime(dirName / subdirName);
            if (fileModifiedTime < subDirTime) {
                isValid_ = false;
                return;
            }
        }
#endif
    }

    IconThemeCache() = default;
    IconThemeCache(IconThemeCache &&other) noexcept
        : isValid_(other.isValid_), memory_(other.memory_), size_(other.size_) {
        other.isValid_ = false;
        other.memory_ = nullptr;
        other.size_ = 0;
    }

    IconThemeCache &operator=(IconThemeCache other) {
        std::swap(other.isValid_, isValid_);
        std::swap(other.size_, size_);
        std::swap(other.memory_, memory_);
        return *this;
    }

    bool isValid() const { return isValid_; }

    ~IconThemeCache() {
#ifndef _WIN32
        if (memory_) {
            munmap(memory_, size_);
        }
#endif
    }

    uint16_t readWord(uint32_t offset) const {
        if (offset > size_ - 2 || (offset % 2)) {
            isValid_ = false;
            return 0;
        }
        return memory_[offset + 1] | memory_[offset] << 8;
    }
    uint32_t readDoubleWord(unsigned int offset) const {
        if (offset > size_ - 4 || (offset % 4)) {
            isValid_ = false;
            return 0;
        }
        return memory_[offset + 3] | memory_[offset + 2] << 8 |
               memory_[offset + 1] << 16 | memory_[offset] << 24;
    }

    char *checkString(uint32_t offset) const {
        // assume string length is less than 1k.
        for (uint32_t i = 0; i < 1024; i++) {
            if (offset + i >= size_) {
                return nullptr;
            }
            auto c = memory_[offset + i];
            if (c == '\0') {
                break;
            }
        }
        return reinterpret_cast<char *>(memory_ + offset);
    }

    std::unordered_set<std::string> lookup(const std::string &name) const;

    mutable bool isValid_ = false;
    uint8_t *memory_ = nullptr;
    size_t size_ = 0;
};

std::unordered_set<std::string>
IconThemeCache::lookup(const std::string &name) const {
    std::unordered_set<std::string> ret;
    auto hash = iconNameHash(name.c_str());

    uint32_t hashOffset = readDoubleWord(4);
    uint32_t hashBucketCount = readDoubleWord(hashOffset);

    if (!isValid_ || hashBucketCount == 0) {
        isValid_ = false;
        return ret;
    }

    uint32_t bucketIndex = hash % hashBucketCount;
    uint32_t bucketOffset = readDoubleWord(hashOffset + 4 + (bucketIndex * 4));
    while (bucketOffset > 0 && bucketOffset <= size_ - 12) {
        uint32_t nameOff = readDoubleWord(bucketOffset + 4);
        auto *namePtr = checkString(nameOff);
        if (nameOff < size_ && namePtr && name == namePtr) {
            uint32_t dirListOffset = readDoubleWord(8);
            uint32_t dirListLen = readDoubleWord(dirListOffset);

            uint32_t listOffset = readDoubleWord(bucketOffset + 8);
            uint32_t listLen = readDoubleWord(listOffset);

            if (!isValid_ || listOffset + 4 + 8 * listLen > size_) {
                isValid_ = false;
                return ret;
            }

            ret.reserve(listLen);
            for (uint32_t j = 0; j < listLen && isValid_; ++j) {
                uint32_t dirIndex = readWord(listOffset + 4 + (8 * j));
                uint32_t o = readDoubleWord(dirListOffset + 4 + (dirIndex * 4));
                if (!isValid_ || dirIndex >= dirListLen || o >= size_) {
                    isValid_ = false;
                    return ret;
                }
                if (auto *str = checkString(o)) {
                    ret.emplace(str);
                } else {
                    return {};
                }
            }
            return ret;
        }
        bucketOffset = readDoubleWord(bucketOffset);
    }
    return ret;
}

class IconThemePrivate : QPtrHolder<IconTheme> {
public:
    IconThemePrivate(IconTheme *q, const StandardPath &path)
        : QPtrHolder(q), standardPath_(path) {
        if (auto home = getEnvironment("HOME")) {
            home_ = *home;
        }
    }

    void parse(const RawConfig &config, IconTheme *parent) {
        if (!parent) {
            subThemeNames_.insert(internalName_);
        }

        auto section = config.get("Icon Theme");
        if (!section) {
            // If it's top level theme, make it fallback to hicolor.
            if (!parent) {
                addInherit("hicolor");
            }
            return;
        }

        if (auto nameSection = section->get("Name")) {
            unmarshallOption(name_, *nameSection, false);
        }

        if (auto commentSection = section->get("Comment")) {
            unmarshallOption(comment_, *commentSection, false);
        }

        auto parseDirectory = [&config,
                               section](const char *name,
                                        std::vector<IconThemeDirectory> &dir) {
            if (auto subConfig = section->get(name)) {
                std::string directories;
                unmarshallOption(directories, *subConfig, false);
                for (const auto &directory :
                     stringutils::split(directories, ",")) {
                    if (auto directoryConfig = config.get(directory)) {
                        try {
                            dir.emplace_back(*directoryConfig);
                        } catch (...) {
                        }
                    }
                }
            }
        };
        parseDirectory("Directories", directories_);
        parseDirectory("ScaledDirectories", scaledDirectories_);
        // if (auto subConfig = section->get("Hidden")) {
        //    unmarshallOption(hidden_, *subConfig, false);
        // }
        if (auto subConfig = section->get("Example")) {
            unmarshallOption(example_, *subConfig, false);
        }
        if (auto subConfig = section->get("Inherits")) {
            std::string inherits;
            unmarshallOption(inherits, *subConfig, false);
            for (const auto &inherit : stringutils::split(inherits, ",")) {
                if (!parent) {
                    addInherit(inherit);
                } else {
                    parent->d_ptr->addInherit(inherit);
                }
            }
        }

        // Always inherit hicolor.
        if (!parent && !subThemeNames_.contains("hicolor")) {
            addInherit("hicolor");
        }
    }

    void addInherit(const std::string &inherit) {
        if (subThemeNames_.insert(inherit).second) {
            try {
                inherits_.push_back(IconTheme(inherit, q_ptr, standardPath_));
            } catch (...) {
            }
        }
    }

    std::string findIcon(const std::string &icon, int size, int scale,
                         const std::vector<std::string> &extensions) const {
        // Respect absolute path.
        if (icon.empty() || icon[0] == '/') {
            return icon;
        }

        std::string filename;
        // If we're a empty theme, only look at fallback icon.
        if (!internalName_.empty()) {
            auto filename = findIconHelper(icon, size, scale, extensions);
            if (!filename.empty()) {
                return filename;
            }
        }
        filename = lookupFallbackIcon(icon, extensions);

        // We tries to violate the spec same as LXDE, only lookup dash fallback
        // after looking through all existing themes.
        if (filename.empty()) {
            auto dashPos = icon.rfind('-');
            if (dashPos != std::string::npos) {
                filename =
                    findIcon(icon.substr(0, dashPos), size, scale, extensions);
            }
        }
        return filename;
    }

    std::string
    findIconHelper(const std::string &icon, int size, int scale,
                   const std::vector<std::string> &extensions) const {
        auto filename = lookupIcon(icon, size, scale, extensions);
        if (!filename.empty()) {
            return filename;
        }

        for (const auto &inherit : inherits_) {
            filename =
                inherit.d_func()->lookupIcon(icon, size, scale, extensions);
            if (!filename.empty()) {
                return filename;
            }
        }
        return {};
    }

    std::string lookupIcon(const std::string &iconname, int size, int scale,
                           const std::vector<std::string> &extensions) const {

        auto checkDirectory = [&extensions,
                               &iconname](const IconThemeDirectory &directory,
                                          std::string baseDir) -> std::string {
            baseDir = stringutils::joinPath(baseDir, directory.path());
            if (!fs::isdir(baseDir)) {
                return {};
            }

            for (const auto &ext : extensions) {
                auto defaultPath = stringutils::joinPath(baseDir, iconname);
                defaultPath += ext;
                if (fs::isreg(defaultPath)) {
                    return defaultPath;
                }
            }
            return {};
        };

        for (const auto &baseDir : baseDirs_) {
            bool hasCache = false;
            std::unordered_set<std::string> dirFilter;
            if (baseDir.second.isValid()) {
                dirFilter = baseDir.second.lookup(iconname);
                hasCache = true;
            }
            for (const auto &directory : directories_) {
                if ((hasCache && !dirFilter.contains(directory.path())) ||
                    !directory.matchesSize(size, scale)) {
                    continue;
                }
                auto path = checkDirectory(directory, baseDir.first);
                if (!path.empty()) {
                    return path;
                }
            }

            if (scale != 1) {
                for (const auto &directory : scaledDirectories_) {
                    if ((hasCache && !dirFilter.contains(directory.path())) ||
                        !directory.matchesSize(size, scale)) {
                        continue;
                    }
                    auto path = checkDirectory(directory, baseDir.first);
                    if (!path.empty()) {
                        return path;
                    }
                }
            }
        }

        auto minSize = std::numeric_limits<int>::max();
        std::string closestFilename;

        for (const auto &baseDir : baseDirs_) {
            bool hasCache = false;
            std::unordered_set<std::string> dirFilter;
            if (baseDir.second.isValid()) {
                dirFilter = baseDir.second.lookup(iconname);
                hasCache = true;
            }

            auto checkDirectoryWithSize =
                [&checkDirectory, &closestFilename, &dirFilter, hasCache, size,
                 scale, &minSize, &baseDir](const IconThemeDirectory &dir) {
                    if (hasCache && !dirFilter.contains(dir.path())) {
                        return;
                    }
                    auto distance = dir.sizeDistance(size, scale);
                    if (distance < minSize) {
                        if (auto path = checkDirectory(dir, baseDir.first);
                            !path.empty()) {
                            closestFilename = std::move(path);
                            minSize = distance;
                        }
                    }
                };

            for (const auto &directory : directories_) {
                checkDirectoryWithSize(directory);
            }

            if (scale != 1) {
                for (const auto &directory : scaledDirectories_) {
                    checkDirectoryWithSize(directory);
                }
            }
        }

        return closestFilename;
    }

    std::string
    lookupFallbackIcon(const std::string &iconname,
                       const std::vector<std::string> &extensions) const {
        auto defaultBasePath = stringutils::joinPath(home_, ".icons", iconname);
        for (const auto &ext : extensions) {
            auto path = defaultBasePath + ext;
            if (fs::isreg(path)) {
                return path;
            }
            path = standardPath_.locate(
                StandardPath::Type::Data,
                stringutils::joinPath("icons", iconname + ext));
            if (!path.empty()) {
                return path;
            }
        }
        return {};
    }

    void addBaseDir(const std::string &path) {
        if (!fs::isdir(path)) {
            return;
        }
        baseDirs_.emplace_back(std::piecewise_construct,
                               std::forward_as_tuple(path),
                               std::forward_as_tuple(stringutils::joinPath(
                                   path, "icon-theme.cache")));
    }

    void prepare() {
        if (!home_.empty()) {
            addBaseDir(stringutils::joinPath(home_, ".icons", internalName_));
        }
        if (auto userDir =
                standardPath_.userDirectory(StandardPath::Type::Data);
            !userDir.empty()) {
            addBaseDir(stringutils::joinPath(userDir, "icons", internalName_));
        }

        for (auto &dataDir :
             standardPath_.directories(StandardPath::Type::Data)) {
            addBaseDir(stringutils::joinPath(dataDir, "icons", internalName_));
        }
    }

    std::string home_;
    std::string internalName_;
    const StandardPath &standardPath_;
    I18NString name_;
    I18NString comment_;
    std::vector<IconTheme> inherits_;
    std::vector<IconThemeDirectory> directories_;
    std::vector<IconThemeDirectory> scaledDirectories_;
    std::unordered_set<std::string> subThemeNames_;
    std::vector<std::pair<std::string, IconThemeCache>> baseDirs_;
    // Not really useful for our usecase.
    // bool hidden_;
    std::string example_;
};

IconTheme::IconTheme(const std::string &name, const StandardPath &standardPath)
    : IconTheme(name, nullptr, standardPath) {}

IconTheme::IconTheme(const std::string &name, IconTheme *parent,
                     const StandardPath &standardPath)
    : IconTheme(standardPath) {
    FCITX_D();
    auto files = standardPath.openAll(
        StandardPath::Type::Data,
        stringutils::joinPath("icons", name, "index.theme"), O_RDONLY);

    RawConfig config;
    for (auto iter = files.rbegin(), end = files.rend(); iter != end; iter++) {
        readFromIni(config, iter->fd());
    }
    auto path = stringutils::joinPath(d->home_, ".icons", name, "index.theme");
    auto fd = UnixFD::own(open(path.c_str(), O_RDONLY));
    if (fd.fd() >= 0) {
        readFromIni(config, fd.fd());
    }

    d->parse(config, parent);
    d->internalName_ = name;
    d->prepare();
}

IconTheme::IconTheme(const StandardPath &standardPath)
    : d_ptr(std::make_unique<IconThemePrivate>(this, standardPath)) {}

FCITX_DEFINE_DEFAULT_DTOR_AND_MOVE(IconTheme);

FCITX_DEFINE_READ_ONLY_PROPERTY_PRIVATE(IconTheme, std::string, internalName);
FCITX_DEFINE_READ_ONLY_PROPERTY_PRIVATE(IconTheme, I18NString, name);
FCITX_DEFINE_READ_ONLY_PROPERTY_PRIVATE(IconTheme, I18NString, comment);
FCITX_DEFINE_READ_ONLY_PROPERTY_PRIVATE(IconTheme, std::vector<IconTheme>,
                                        inherits);
FCITX_DEFINE_READ_ONLY_PROPERTY_PRIVATE(IconTheme,
                                        std::vector<IconThemeDirectory>,
                                        directories);
FCITX_DEFINE_READ_ONLY_PROPERTY_PRIVATE(IconTheme,
                                        std::vector<IconThemeDirectory>,
                                        scaledDirectories);
FCITX_DEFINE_READ_ONLY_PROPERTY_PRIVATE(IconTheme, std::string, example);

std::string IconTheme::findIcon(const std::string &iconName,
                                unsigned int desiredSize, int scale,
                                const std::vector<std::string> &extensions) {
    return std::as_const(*this).findIcon(iconName, desiredSize, scale,
                                         extensions);
}

std::string
IconTheme::findIcon(const std::string &iconName, unsigned int desiredSize,
                    int scale,
                    const std::vector<std::string> &extensions) const {
    FCITX_D();
    return d->findIcon(iconName, desiredSize, scale, extensions);
}

std::string getKdeTheme(int fd) {
    RawConfig rawConfig;
    readFromIni(rawConfig, fd);
    if (auto icons = rawConfig.get("Icons")) {
        if (auto theme = icons->get("Theme")) {
            if (!theme->value().empty() &&
                theme->value().find('/') == std::string::npos) {
                return theme->value();
            }
        }
    }
    return "";
}

std::string getGtkTheme(const std::string &filename) {
    // Evil Gtk2 use an non standard "ini-like" rc file.
    // Grep it and try to find the line we want ourselves.
    std::ifstream fin(filename, std::ios::in | std::ios::binary);
    std::string line;
    while (std::getline(fin, line)) {
        auto tokens = stringutils::split(line, "=");
        if (tokens.size() == 2 &&
            stringutils::trim(tokens[0]) == "gtk-icon-theme-name") {
            auto value = stringutils::trim(tokens[1]);
            if (value.size() >= 2 && value.front() == '"' &&
                value.back() == '"') {
                value = value.substr(1, value.size() - 2);
            }
            if (!value.empty() && value.find('/') == std::string::npos) {
                return value;
            }
        }
    }
    return "";
}

std::string IconTheme::defaultIconThemeName() {
    DesktopType desktopType = getDesktopType();
    switch (desktopType) {
    case DesktopType::KDE6:
    case DesktopType::KDE5: {
        auto files = StandardPath::global().openAll(StandardPath::Type::Config,
                                                    "kdeglobals", O_RDONLY);
        for (auto &file : files) {
            auto theme = getKdeTheme(file.fd());
            if (!theme.empty()) {
                return theme;
            }
        }

        return "breeze";
    }
    case DesktopType::KDE4: {
        auto home = getEnvironment("HOME");
        if (home && !home->empty()) {
            std::string files[] = {
                stringutils::joinPath(*home, ".kde4/share/config/kdeglobals"),
                stringutils::joinPath(*home, ".kde/share/config/kdeglobals"),
                "/etc/kde4/kdeglobals"};
            for (auto &file : files) {
                auto fd = UnixFD::own(open(file.c_str(), O_RDONLY));
                auto theme = getKdeTheme(fd.fd());
                if (!theme.empty()) {
                    return theme;
                }
            }
        }
        return "oxygen";
    }
    default: {
        auto files = StandardPath::global().locateAll(
            StandardPath::Type::Config, "gtk-3.0/settings.ini");
        for (auto &file : files) {
            auto theme = getGtkTheme(file);
            if (!theme.empty()) {
                return theme;
            }
        }
        auto theme = getGtkTheme("/etc/gtk-3.0/settings.ini");
        if (!theme.empty()) {
            return theme;
        }
        auto home = getEnvironment("HOME");
        if (home && !home->empty()) {
            std::string files[] = {stringutils::joinPath(*home, ".gtkrc-2.0"),
                                   "/etc/gtk-2.0/gtkrc"};
            for (auto &file : files) {
                auto theme = getGtkTheme(file);
                if (!theme.empty()) {
                    return theme;
                }
            }
        }
    } break;
    }

    if (desktopType == DesktopType::Unknown) {
        return "Tango";
    }
    if (desktopType == DesktopType::GNOME) {
        return "Adwaita";
    }
    return "gnome";
}

/// Rename fcitx-* icon to org.fcitx.Fcitx5.fcitx-* if in flatpak
std::string IconTheme::iconName(const std::string &icon, bool inFlatpak) {
    constexpr std::string_view fcitxIconPrefix = "fcitx";
    if (inFlatpak && stringutils::startsWith(icon, fcitxIconPrefix)) {
        // Map "fcitx" to org.fcitx.Fcitx5
        // And map fcitx* to org.fcitx.Fcitx5.fcitx*
        if (icon.size() == fcitxIconPrefix.size()) {
            return "org.fcitx.Fcitx5";
        }
        return stringutils::concat("org.fcitx.Fcitx5.", icon);
    }
    return icon;
}
} // namespace fcitx
