#!/usr/bin/env python3
#
# Helper to compare the local emoji-eac.mb against fcitx5 master.
#
import os

import requests

UPSTREAM_URL = "https://raw.githubusercontent.com/fcitx/fcitx5/master/src/modules/quickphrase/quickphrase.d/emoji-eac.mb"
LOCAL = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "src/modules/quickphrase/quickphrase.d/emoji-eac.mb",
)

print(f"Downloading {UPSTREAM_URL}")
upstream = set(requests.get(UPSTREAM_URL).text.splitlines())

with open(LOCAL, encoding="utf-8") as stream:
    local = set(stream.read().splitlines())

common = local & upstream
only_local = local - upstream
only_upstream = upstream - local

print(f"upstream: {len(upstream)} lines")
print(f"local:    {len(local)} lines")
print(f"common:   {len(common)} lines")
print(f"local only ({len(only_local)}):")
for line in sorted(only_local):
    print(f"  + {line}")
print(f"upstream only ({len(only_upstream)}):")
for line in sorted(only_upstream):
    print(f"  - {line}")