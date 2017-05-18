#!/usr/bin/env python3

# get the emoji alpha codes (short codes) from https://github.com/Ranks/emoji-alpha-codes
# these are licensed under the MIT license (http://opensource.org/licenses/MIT)

import csv
import urllib.request

resp = urllib.request.urlopen("https://github.com/Ranks/emoji-alpha-codes/raw/master/eac.csv")
codes_csv = resp.read().decode()
reader = csv.reader(codes_csv.splitlines(), delimiter=',')
next(reader) # skip header
for row in reader:
    codes = [row[2]] + (row[3].split("|") if row[3] else [])
    for code in codes:
        c = "".join([chr(int(codepoint,16)) for codepoint in row[0].split("-")])
        print("{} {}".format(code, c))
