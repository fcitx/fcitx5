/*
 * SPDX-FileCopyrightText: 2021~2021 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "fcitx-utils/log.h"
#include "fcitx-utils/semver.h"

using namespace fcitx;

int main() {
    SemanticVersion sem;
    FCITX_ASSERT(sem.toString() == "0.1.0");

    const char *versionString[] = {
        "1.0.0-alpha", "1.0.0-alpha.1", "1.0.0-alpha.beta",
        "1.0.0-beta",  "1.0.0-beta.2",  "1.0.0-beta.11",
        "1.0.0-rc.1",  "1.0.0",         "2.0.0",
        "2.1.0",       "2.1.1",         "2.1.9",
        "2.3.0"};
    std::vector<SemanticVersion> versions;
    for (auto *str : versionString) {
        auto result = SemanticVersion::parse(str);
        FCITX_ASSERT(result) << str;
        FCITX_ASSERT(result.value().toString() == str) << str;
        versions.emplace_back(result.value());
    }

    for (size_t i = 1; i < versions.size(); i++) {
        FCITX_ASSERT(versions[i - 1] < versions[i])
            << versions[i - 1] << " " << versions[i];
        FCITX_ASSERT(versions[i] > versions[i - 1]);
        FCITX_ASSERT(versions[i] >= versions[i - 1]);
        FCITX_ASSERT(versions[i - 1] <= versions[i]);
        FCITX_ASSERT(versions[i] != versions[i - 1]);
    }

    const char *withBuild[] = {"1.0.0-alpha+001", "1.0.0+20130313144700",
                               "1.0.0-beta+exp.sha.5114f85",
                               "1.0.0+21AF26D3---117B344092BD"};
    for (const char *str : withBuild) {
        auto result = SemanticVersion::parse(str);
        FCITX_ASSERT(result) << str;
        FCITX_ASSERT(result.value().toString() == str) << str;
    }

    FCITX_ASSERT(SemanticVersion::parse("1.0.0+20130313144700") ==
                 SemanticVersion::parse("1.0.0"));
    auto ver = SemanticVersion::parse("1.2.3-beta.alpha+exp.sha.5114f85");
    FCITX_ASSERT(ver);
    FCITX_ASSERT((ver->major)() == 1);
    FCITX_ASSERT((ver->minor)() == 2);
    FCITX_ASSERT(ver->patch() == 3);
    FCITX_ASSERT(
        ver->preReleaseIds() ==
        std::vector<PreReleaseId>{PreReleaseId("beta"), PreReleaseId("alpha")});
    FCITX_ASSERT(ver->buildIds() ==
                 std::vector<std::string>{"exp", "sha", "5114f85"});

    const char *badVersion[] = {"1",         "1.",           "a",
                                "1.a",       "1.2.3-",       "1.2.3+",
                                "1.2.3-+-",  "1.2.3-a..b",   "1.2.3-a.b+c..d",
                                "1.2.3-a.$", "1.2.3-a.b+c.."};
    for (const char *str : badVersion) {
        FCITX_ASSERT(!SemanticVersion::parse(str)) << str;
    }
    return 0;
}
