#include <algorithm>
#include "macros.h"
#include "stringutils.h"
#include "charutils.h"

namespace fcitx {
namespace stringutils {
std::pair<std::string::size_type, std::string::size_type>
trimInplace(const std::string &str) {
    auto start = str.find_first_not_of(FCITX_WHITE_SPACE);
    if (start == std::string::npos) {
        return std::pair<std::string::size_type, std::string::size_type>(
            str.size(), str.size());
    }

    auto end = str.size();
    while (end > start && charutils::isspace(str[end - 1]))
        --end;

    return std::pair<std::string::size_type, std::string::size_type>(start,
                                                                     end);
}

std::vector<std::string> split(const std::string &str,
                               const std::string &delim) {
    std::vector<std::string> strings;

    auto lastPos = str.find_first_not_of(delim, 0);
    auto pos = str.find_first_of(delim, lastPos);

    while (std::string::npos != pos || std::string::npos != lastPos) {
        strings.push_back(str.substr(lastPos, pos - lastPos));
        lastPos = str.find_first_not_of(delim, pos);
        pos = str.find_first_of(delim, lastPos);
    }

    return strings;
}

#define MAX_REPLACE_INDICES_NUM 128

std::string replaceAll(std::string str, const std::string &before, const std::string &after)
{
    if (before.size() == 0) {
        return str;
    }


    size_t pivot = 0;
    std::string newString;
    size_t lastLen = 0;
    int indices[MAX_REPLACE_INDICES_NUM];

    int newStringPos = 0;
    int oldStringPos = 0;

    do {
        int nIndices = 0;
        while (nIndices < MAX_REPLACE_INDICES_NUM) {
            pivot = str.find(before, pivot);
            if (pivot == std::string::npos) {
                break;
            }

            indices[nIndices++] = pivot;
            pivot += before.size();
        }

        if (nIndices) {
            if (!lastLen) {
                lastLen = str.size() + nIndices * (after.size() - before.size());
                newString.resize(lastLen);
            } else {
                size_t newLen = lastLen + nIndices * (after.size() - before.size());
                lastLen = newLen;
                newString.resize(newLen);
            }

#define _COPY_AND_MOVE_ON(s, pos, LEN) \
    do { \
        int diffLen = (LEN); \
        if ((LEN) == 0) { \
            break; \
        } \
        newString.replace(newStringPos, diffLen, s, pos, diffLen); \
        newStringPos += diffLen; \
    } while(0)

            // string s is split as
            // oldStringPos, indices[0], indices[0] + before.size(), indices[1], indices[1] + before.size()
            // .... indices[nIndices - 1], indices[nIndices - 1] + before.size()
            _COPY_AND_MOVE_ON(str, oldStringPos, indices[0] - oldStringPos);
            _COPY_AND_MOVE_ON(after, 0, after.size());

            for (int i = 1; i < nIndices; i ++) {
                _COPY_AND_MOVE_ON(str, indices[i] + before.size(), indices[i] - (indices[i - 1] + before.size()));
                _COPY_AND_MOVE_ON(after, 0, after.size());
            }

            oldStringPos = indices[nIndices - 1] + before.size();
        }
    } while(pivot != std::string::npos);

    if (!lastLen) {
        return str;
    } else {
        _COPY_AND_MOVE_ON(str, oldStringPos, str.size() - oldStringPos);
        newString.resize(newStringPos);
    }

    return newString;
}


}
}
