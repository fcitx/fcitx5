/*
* Copyright (C) 2017~2017 by CSSlayer
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
#include "icontheme.h"
#include "fcitx-config/iniparser.h"
#include "fcitx-config/marshallfunction.h"
#include "fcitx-utils/fs.h"
#include "fcitx-utils/log.h"
#include <fcntl.h>
#include <fstream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unordered_set>

namespace fcitx {

std::string pathToRoot(const RawConfig &config) {
    std::string path;
    auto pConfig = &config;
    while (pConfig) {
        if (pConfig->parent() && !path.empty()) {
            path = "/" + path;
        }
        path = pConfig->name() + path;
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
            unmarshallOption(size_, *subConfig);
        }
        if (size_ <= 0) {
            throw std::invalid_argument("Invalid size");
        }

        if (auto subConfig = config.get("Scale")) {
            unmarshallOption(size_, *subConfig);
        }
        if (auto subConfig = config.get("Context")) {
            unmarshallOption(context_, *subConfig);
        }
        if (auto subConfig = config.get("Type")) {
            unmarshallOption(type_, *subConfig);
        }
        if (auto subConfig = config.get("MaxSize")) {
            unmarshallOption(maxSize_, *subConfig);
        }
        if (auto subConfig = config.get("MinSize")) {
            unmarshallOption(minSize_, *subConfig);
        }
        if (auto subConfig = config.get("Threshold")) {
            unmarshallOption(threshold_, *subConfig);
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
        return std::abs(size() * scale() - iconsize * iconscale);
    case IconThemeDirectoryType::Scalable:
        if (iconsize * iconscale < minSize() * scale()) {
            return minSize() * scale() - iconsize * iconscale;
        }
        if (iconsize * iconscale > maxSize() * scale()) {
            return iconsize * iconscale - maxSize() * scale();
        }
        return 0;
    case IconThemeDirectoryType::Threshold:
        if (iconsize * iconscale < (size() - threshold()) * scale()) {
            return (size() - threshold()) * scale() - iconsize * iconscale;
        }
        if (iconsize * iconscale > (size() + threshold()) * scale()) {
            return iconsize * iconscale - (size() - threshold()) * scale();
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

bool timespecLess(const timespec &lhs, const timespec &rhs) {
    if (lhs.tv_sec != rhs.tv_sec) {
        return lhs.tv_sec < rhs.tv_sec;
    }
    return lhs.tv_nsec < rhs.tv_nsec;
}

static uint32_t iconNameHash(const char *p) {
    uint32_t h = static_cast<signed char>(*p);
    for (p += 1; *p != '\0'; p++)
        h = (h << 5) - h + *p;
    return h;
}

class IconThemeCache {
public:
    IconThemeCache(const std::string &filename) {
        if (!fs::isreg(filename)) {
            return;
        }
        struct stat st;
        if (stat(filename.c_str(), &st) != 0) {
            return;
        }
        struct stat dirSt;
        auto dirName = fs::dirName(filename);
        if (stat(dirName.c_str(), &dirSt) != 0) {
            return;
        }
        if (timespecLess(st.st_mtim, dirSt.st_mtim)) {
            return;
        }

        auto fd = UnixFD::own(open(filename.c_str(), O_RDONLY));
        memory_ = static_cast<uint8_t *>(
            mmap(nullptr, st.st_size, PROT_READ, MAP_SHARED, fd.fd(), 0));
        if (!memory_) {
            throw std::runtime_error("Failed to mmap");
        }
        size_ = st.st_size;

        if (readWord(0) != 1 || readWord(2) != 0) { // version major minor
            return;
        }

        isValid_ = true;

        // Check that all the directories are older than the cache
        uint32_t dirListOffset = readDoubleWord(8);
        uint32_t dirListLen = readDoubleWord(dirListOffset);
        for (uint32_t i = 0; i < dirListLen; ++i) {
            uint32_t offset = readDoubleWord(dirListOffset + 4 + 4 * i);
            if (!isValid_ || offset >= size_) {
                isValid_ = false;
                return;
            }
            struct stat subDirSt;
            auto dir = checkString(offset);
            if (!dir ||
                stat(stringutils::joinPath(dirName, dir).c_str(), &subDirSt) !=
                    0) {
                isValid_ = false;
                return;
            }
            if (timespecLess(st.st_mtim, subDirSt.st_mtim)) {
                isValid_ = false;
                return;
            }
        }
    }

    IconThemeCache() = default;
    IconThemeCache(IconThemeCache &&other)
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
        if (memory_) {
            munmap(memory_, size_);
        }
    }

    uint16_t readWord(uint32_t offset) const {
        if (offset > size_ - 2 || (offset % 2)) {
            isValid_ = false;
            return 0;
        }
        return memory_[offset + 1] | memory_[offset] << 8;
    }
    uint32_t readDoubleWord(uint offset) const {
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
            if (c == '\0')
                break;
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
    uint32_t bucketOffset = readDoubleWord(hashOffset + 4 + bucketIndex * 4);
    while (bucketOffset > 0 && bucketOffset <= size_ - 12) {
        uint32_t nameOff = readDoubleWord(bucketOffset + 4);
        auto namePtr = checkString(nameOff);
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
                uint32_t dirIndex = readWord(listOffset + 4 + 8 * j);
                uint32_t o = readDoubleWord(dirListOffset + 4 + dirIndex * 4);
                if (!isValid_ || dirIndex >= dirListLen || o >= size_) {
                    isValid_ = false;
                    return ret;
                }
                if (auto str = checkString(o)) {
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
        const char *home = getenv("HOME");
        if (home) {
            home_ = home;
        }
    }

    void loadFile(int fd) { readFromIni(config_, fd); }

    void parse(IconTheme *parent) {
        if (!parent) {
            subThemeNames_.insert(internalName_);
        }

        auto section = config_.get("Icon Theme");
        if (!section) {
            // If it's top level theme, make it fallback to hicolor.
            if (!parent) {
                addInherit("hicolor");
            }
            return;
        }

        if (auto nameSection = section->get("Name")) {
            unmarshallOption(name_, *nameSection);
        }

        if (auto commentSection = section->get("Comment")) {
            unmarshallOption(comment_, *commentSection);
        }

        auto parseDirectory = [this, section](
            const char *name, std::vector<IconThemeDirectory> &dir) {
            if (auto subConfig = section->get(name)) {
                std::string directories;
                unmarshallOption(directories, *subConfig);
                for (const auto &directory :
                     stringutils::split(directories, ",")) {
                    if (auto directoryConfig = config_.get(directory)) {
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
        if (auto subConfig = section->get("Hidden")) {
            unmarshallOption(hidden_, *subConfig);
        }
        if (auto subConfig = section->get("Example")) {
            unmarshallOption(example_, *subConfig);
        }
        if (auto subConfig = section->get("Inherits")) {
            std::string inherits;
            unmarshallOption(inherits, *subConfig);
            for (const auto &inherit : stringutils::split(inherits, ",")) {
                if (!parent) {
                    addInherit(inherit);
                } else {
                    parent->d_ptr->addInherit(inherit);
                }
            }
        }

        // Always inherit hicolor.
        if (!parent && !subThemeNames_.count("hicolor")) {
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

        for (auto &inherit : inherits_) {
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

        auto checkDirectory = [&extensions, &iconname, size, scale, this](
            const IconThemeDirectory &directory,
            std::string baseDir) -> std::string {
            baseDir = stringutils::joinPath(baseDir, directory.path());
            if (!fs::isdir(baseDir)) {
                return {};
            }

            for (auto &ext : extensions) {
                auto defaultPath =
                    stringutils::joinPath(baseDir, iconname);
                defaultPath += ext;
                if (fs::isreg(defaultPath)) {
                    return defaultPath;
                }
            }
            return {};
        };

        for (auto &baseDir : baseDirs_) {
            bool hasCache = false;
            std::unordered_set<std::string> dirFilter;
            if (baseDir.second.isValid()) {
                dirFilter = baseDir.second.lookup(iconname);
                hasCache = true;
            }
            for (auto &directory : directories_) {
                if ((hasCache && !dirFilter.count(directory.path())) ||
                    !directory.matchesSize(size, scale)) {
                    continue;
                }
                auto path = checkDirectory(directory, baseDir.first);
                if (!path.empty()) {
                    return path;
                }
            }

            if (scale != 1) {
                for (auto &directory : scaledDirectories_) {
                    if ((hasCache && !dirFilter.count(directory.path())) ||
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

        for (auto &baseDir : baseDirs_) {
            bool hasCache = false;
            std::unordered_set<std::string> dirFilter;
            if (baseDir.second.isValid()) {
                dirFilter = baseDir.second.lookup(iconname);
                hasCache = true;
            }

            auto checkDirectoryWithSize =
                [&checkDirectory, &closestFilename, &dirFilter, hasCache, size,
                 scale, &minSize, &baseDir](const IconThemeDirectory &dir) {
                    if (hasCache && !dirFilter.count(dir.path())) {
                        return;
                    }
                    auto distance = dir.sizeDistance(size, scale);
                    if (distance < minSize) {
                        auto path = checkDirectory(dir, baseDir.first);
                        if (!path.empty()) {
                            closestFilename = path;
                            minSize = distance;
                        }
                    }
                };

            for (auto &directory : directories_) {
                checkDirectoryWithSize(directory);
            }

            if (scale != 1) {
                for (auto &directory : scaledDirectories_) {
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
        for (auto &ext : extensions) {
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

    void prepare() {
        auto path = stringutils::joinPath(home_, ".icons", internalName_);
        if (fs::isdir(path)) {
            baseDirs_.emplace_back(std::piecewise_construct,
                                   std::forward_as_tuple(path),
                                   std::forward_as_tuple(stringutils::joinPath(
                                       path, "icon-theme.cache")));
        }
        for (auto &dataDir :
             standardPath_.directories(StandardPath::Type::Data)) {
            auto path = stringutils::joinPath(dataDir, "icons", internalName_);
            if (fs::isdir(path)) {
                baseDirs_.emplace_back(
                    std::piecewise_construct, std::forward_as_tuple(path),
                    std::forward_as_tuple(
                        stringutils::joinPath(path, "icon-theme.cache")));
            }
        }
    }

    std::string home_;
    std::string internalName_;
    const StandardPath &standardPath_;
    RawConfig config_;
    I18NString name_;
    I18NString comment_;
    std::vector<IconTheme> inherits_;
    std::vector<IconThemeDirectory> directories_;
    std::vector<IconThemeDirectory> scaledDirectories_;
    std::unordered_set<std::string> subThemeNames_;
    std::vector<std::pair<std::string, IconThemeCache>> baseDirs_;
    bool hidden_;
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

    for (auto iter = files.rbegin(), end = files.rend(); iter != end; iter++) {
        d->loadFile(iter->fd());
    }
    auto path = stringutils::joinPath(d->home_, ".icons", name, "index.theme");
    auto fd = UnixFD::own(open(path.c_str(), O_RDONLY));
    if (fd.fd() >= 0) {
        d->loadFile(fd.fd());
    }

    d->parse(parent);
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
FCITX_DEFINE_READ_ONLY_PROPERTY_PRIVATE(IconTheme, bool, hidden);
FCITX_DEFINE_READ_ONLY_PROPERTY_PRIVATE(IconTheme, std::string, example);

std::string IconTheme::findIcon(const std::string &iconName, uint desiredSize,
                                int scale,
                                const std::vector<std::string> &extensions) {
    FCITX_D();
    return d->findIcon(iconName, desiredSize, scale, extensions);
}

enum class DesktopType {
    KDE5,
    KDE4,
    GNOME,
    Cinnamon,
    MATE,
    LXDE,
    XFCE,
    Unknown
};

DesktopType getDesktopType() {
    std::string desktop;
    auto desktopEnv = getenv("XDG_CURRENT_DESKTOP");
    if (desktopEnv) {
        desktop = desktopEnv;
    }
    if (desktop == "KDE") {
        auto version = getenv("KDE_SESSION_VERSION");
        auto versionInt = 0;
        if (version) {
            try {
                versionInt = std::stoi(version);
            } catch (...) {
            }
        }
        if (versionInt == 4) {
            return DesktopType::KDE4;
        } else if (versionInt == 5) {
            return DesktopType::KDE5;
        }
    } else if (desktop == "X-Cinnamon") {
        return DesktopType::Cinnamon;
    } else if (desktop == "LXDE") {
        return DesktopType::LXDE;
    } else if (desktop == "MATE") {
        return DesktopType::MATE;
    } else if (desktop == "Gnome") {
        return DesktopType::GNOME;
    } else if (desktop == "XFCE") {
        return DesktopType::XFCE;
    }
    return DesktopType::Unknown;
}

std::string getKdeTheme(int fd) {
    RawConfig rawConfig;
    readFromIni(rawConfig, fd);
    if (auto icons = rawConfig.get("Icons")) {
        if (auto theme = icons->get("Theme")) {
            if (!theme->value().empty() &&
                theme->value().find("/") == std::string::npos) {
                return theme->value();
            }
        }
    }
    return "";
}

std::string getGtk3Theme(int fd) {
    RawConfig rawConfig;
    readFromIni(rawConfig, fd);
    if (auto settings = rawConfig.get("Settings")) {
        if (auto theme = settings->get("gtk-icon-theme-name")) {
            if (!theme->value().empty() &&
                theme->value().find("/") == std::string::npos) {
                return theme->value();
            }
        }
    }
    return "";
}

std::string getGtk2Theme(const std::string filename) {
    // Evil Gtk2 use an non standard "ini-like" rc file.
    // Grep it and try to find the line we want ourselves.
    std::ifstream fin(filename, std::ios::in | std::ios::binary);
    std::string line;
    while (std::getline(fin, line)) {
        auto tokens = stringutils::split(line, "=");
        if (tokens.size() == 2 &&
            stringutils::trim(tokens[0]) == "gtk-icon-theme-name") {
            auto value = stringutils::trim(tokens[1]);
            if (!value.empty() && value.find("/") == std::string::npos) {
                return value;
            }
        }
    }
    return "";
}

std::string IconTheme::defaultIconThemeName() {
    DesktopType desktopType = getDesktopType();
    switch (desktopType) {
    case DesktopType::KDE5: {
        auto files = StandardPath::global().openAll(StandardPath::Type::Config,
                                                    "kdeglobals", O_RDONLY);
        for (auto &file : files) {
            auto theme = getKdeTheme(file.fd());
            if (!theme.empty()) {
                return theme;
            }
        }

        return "oxygen";
    }
    case DesktopType::KDE4: {
        const char *home = getenv("HOME");
        if (home && home[0]) {
            std::string files[] = {
                stringutils::joinPath(home, ".kde4/share/config/kdeglobals"),
                stringutils::joinPath(home, ".kde/share/config/kdeglobals"),
                "/etc/kde4/kdeglobals"};
            for (auto &file : files) {
                auto fd = UnixFD::own(open(file.c_str(), O_RDONLY));
                auto theme = getKdeTheme(fd.fd());
                if (!theme.empty()) {
                    return theme;
                }
            }
        }
        return "breeze";
    }
    case DesktopType::GNOME:
    case DesktopType::Cinnamon:
    case DesktopType::LXDE:
    case DesktopType::MATE:
    case DesktopType::XFCE: {
        auto files = StandardPath::global().openAll(
            StandardPath::Type::Config, "gtk-3.0/settings.ini", O_RDONLY);
        for (auto &file : files) {
            auto theme = getGtk3Theme(file.fd());
            if (!theme.empty()) {
                return theme;
            }
        }
        auto fd = UnixFD::own(open("/etc/gtk-3.0/settings.ini", O_RDONLY));
        auto theme = getGtk3Theme(fd.fd());
        if (!theme.empty()) {
            return theme;
        }
        const char *home = getenv("HOME");
        if (home && home[0]) {
            std::string homeStr(home);
            std::string files[] = {stringutils::joinPath(homeStr, ".gtkrc-2.0"),
                                   "/etc/gtk-2.0/gtkrc"};
            for (auto &file : files) {
                auto theme = getGtk2Theme(file);
                if (!theme.empty()) {
                    return theme;
                }
            }
        }

        return "gnome";
    }
    default:
        break;
    }

    return "Tango";
}
}
