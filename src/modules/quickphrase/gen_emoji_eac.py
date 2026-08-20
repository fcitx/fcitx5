#!/usr/bin/env python3
#
# Helper to regenerate src/modules/quickphrase/quickphrase.d/emoji-eac.mb
# from the delthas/gemoji-json fork of github/gemoji (Emoji 17.0).
#
import os

import requests

EMOJI_JSON_URL = "https://raw.githubusercontent.com/delthas/gemoji-json/master/emoji.json"
OUTPUT = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "src/modules/quickphrase/quickphrase.d/emoji-eac.mb",
)

print(f"Downloading {EMOJI_JSON_URL}")
response = requests.get(EMOJI_JSON_URL)
response.raise_for_status()

emojis = response.json()
print(f"Parsing {len(emojis)} emoji entries")

lines = []
for entry in emojis:
    for alias in entry["aliases"]:
        lines.append(f":{alias}: {entry['emoji']}")

with open(OUTPUT, "w", encoding="utf-8") as stream:
    stream.write("\n".join(lines) + "\n")

print(f"Wrote {len(lines)} entries to {OUTPUT}")
