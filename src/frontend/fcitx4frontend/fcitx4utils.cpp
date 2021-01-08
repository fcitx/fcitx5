/*
 * SPDX-FileCopyrightText: 2020-2021 Vifly <viflythink@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "fcitx4utils.h"

size_t cStrLens(size_t n, const char **str_list, size_t *size_list) {
    size_t i;
    size_t total = 0;
    for (i = 0; i < n; i++) {
        total += (size_list[i] = str_list[i] ? strlen(str_list[i]) : 0);
    }
    return total + 1;
}

void catCStr(char *out, size_t n, const char **str_list,
             const size_t *size_list) {
    size_t i = 0;
    for (i = 0; i < n; i++) {
        if (!size_list[i])
            continue;
        memcpy(out, str_list[i], size_list[i]);
        out += size_list[i];
    }
    *out = '\0';
}

char **FcitxXDGGetPath(size_t *len, const char *homeEnv,
                       const char *homeDefault, const char *suffixHome,
                       const char *dirsDefault, const char *suffixGlobal) {
    char cwd[1024];
    cwd[1023] = '\0';
    const char *xdgDirHome = getenv(homeEnv);
    const char *dirHome;
    char *home_buff;
    size_t dh_len;

    if (xdgDirHome && xdgDirHome[0]) {
        home_buff = nullptr;
        dirHome = xdgDirHome;
        dh_len = strlen(dirHome);
    } else {
        const char *env_home = getenv("HOME");
        if (!(env_home && env_home[0])) {
            getcwd(cwd, sizeof(cwd) - 1);
            env_home = cwd;
        }
        size_t he_len = strlen(env_home);
        size_t hd_len = strlen(homeDefault);
        dh_len = he_len + hd_len + 1;
        home_buff = static_cast<char *>(malloc(dh_len + 1));
        dirHome = home_buff;
        combinePathWithLen(home_buff, env_home, he_len, homeDefault, hd_len);
    }

    char *dirs;
    char **dirsArray;
    size_t sh_len = strlen(suffixHome);
    size_t orig_len1 = dh_len + sh_len;
    if (dirsDefault) {
        size_t dd_len = strlen(dirsDefault);
        size_t sg_len = strlen(suffixGlobal);
        *len = 2;
        dirs = static_cast<char *>(malloc(orig_len1 + dd_len + sg_len + 4));
        dirsArray = static_cast<char **>(malloc(2 * sizeof(char *)));
        dirsArray[0] = dirs;
        dirsArray[1] = dirs + orig_len1 + 2;
        combinePathWithLen(dirs, dirHome, dh_len, suffixHome, sh_len);
        combinePathWithLen(dirsArray[1], dirsDefault, dd_len, suffixGlobal,
                           sg_len);
    } else {
        *len = 1;
        dirs = static_cast<char *>(malloc(orig_len1 + 2));
        dirsArray = static_cast<char **>(malloc(sizeof(char *)));
        dirsArray[0] = dirs;
        combinePathWithLen(dirs, dirHome, dh_len, suffixHome, sh_len);
    }
    if (home_buff) {
        free(home_buff);
    }
    return dirsArray;
}

FILE *Fcitx4XDGGetFile(const char *fileName, char **path, const char *mode,
                       size_t len, char **retFile) {
    size_t i;
    FILE *fp = nullptr;

    if (len <= 0) {
        if (retFile && (strchr(mode, 'w') || strchr(mode, 'a'))) {
            *retFile = strdup(fileName);
        }
        return nullptr;
    }

    if (!mode) {
        if (retFile) {
            if (fileName[0] == '/') {
                *retFile = strdup(fileName);
            } else {
                allocCatCStr(*retFile, path[0], "/", fileName);
            }
        }
        return nullptr;
    }

    /* check absolute path */
    if (fileName[0] == '/') {
        fp = fopen(fileName, mode);

        if (retFile) {
            *retFile = strdup(fileName);
        }

        return fp;
    }

    /* check empty file name */
    if (!fileName[0]) {
        if (retFile) {
            *retFile = strdup(path[0]);
        }
        if (strchr(mode, 'w') || strchr(mode, 'a')) {
            fcitx::fs::makePath(path[0]);
        }
        return nullptr;
    }

    // when we reach here, path is valid, fileName is valid, mode is valid.
    char *buf = nullptr;
    for (i = 0; i < len; i++) {
        allocCatCStr(buf, path[i], "/", fileName);
        fp = fopen(buf, mode);
        if (fp) {
            break;
        } else {
            free(buf);
        }
    }

    if (!fp) {
        if (strchr(mode, 'w') || strchr(mode, 'a')) {
            allocCatCStr(buf, path[0], "/", fileName);
            char *dirc = strdup(buf);
            char *dir = dirname(dirc);
            fcitx::fs::makePath(dir);
            free(dirc);
            fp = fopen(buf, mode);
        } else {
            buf = nullptr;
        }
    }

    if (retFile) {
        *retFile = buf;
    } else if (buf) {
        free(buf);
    }
    return fp;
}

char **Fcitx4XDGGetPathUserWithPrefix(size_t *len, const char *prefix) {
    char *prefixpath;
    char **result;
    allocCatCStr(prefixpath, PACKAGE, "/", prefix);
    result = FcitxXDGGetPath(len, "XDG_CONFIG_HOME", ".config", prefixpath,
                             nullptr, nullptr);
    free(prefixpath);
    return result;
}

FILE *Fcitx4XDGGetFileUserWithPrefix(const char *prefix, const char *fileName,
                                     const char *mode, char **retFile) {
    size_t len;
    char **path = Fcitx4XDGGetPathUserWithPrefix(&len, prefix);

    FILE *fp = Fcitx4XDGGetFile(fileName, path, mode, len, retFile);

    if (path) {
        free(path[0]);
        free(path);
    }

    return fp;
}
