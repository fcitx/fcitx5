#!/usr/bin/python
# -*- coding: utf-8 -*-
#
# This script generates a data file containing all Unicode information needed by KCharSelect.
#
#############################################################################
# Copyright (C) 2007 Daniel Laidig <d.laidig@gmx.de>
#
# This script is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This script is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public License
# along with this library; see the file COPYING.LIB.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301, USA.
#############################################################################
#
# The current directory must contain the following files that can be found at
# http://www.unicode.org/Public/UNIDATA/:
# - UnicodeData.txt
# - Unihan_Readings.txt (you need to uncompress it from Unihan.zip)
# - NamesList.txt
# - Blocks.txt
#
# The generated file is named "kcharselect-data" and has to be put in kdelibs/kdeui/widgets/.
# Additionally a translation dummy named "kcharselect-translation.cpp" is generated and has
# to be placed in the same directory.
#
# FILE STRUCTURE
#
# The generated file is a binary file. The first 40 bytes are the header
# and contain the position of each part of the file. Each entry is uint32.
#
# pos   content
# 0     names strings begin
# 4     names offsets begin
# 8     details strings begin
# 12    details offsets begin
# 16    block strings begin
# 20    block offsets begin
# 24    section strings begin
# 28    section offsets begin
# 32    unihan strings begin
# 36    unihan offsets begin
#
# The string parts always contain all strings in a row, followed by a 0x00 byte.
# There is one exception: The data for seeAlso in details is only 2 bytes (as is always is _one_
# unicode character) and _not_ followed by a 0x00 byte.
#
# The offset parts contain entries with a fixed length. Unicode characters are always uint16 and offsets uint32.
# Offsets are positions in the data file.
#
# names_offsets:
# each entry 6 bytes
# 16bit: unicode
# 32bit: offset to name in names_strings
#
# names_strings:
# the first byte is the category (same values as QChar::Category),
# directly followed by the character name (terminated by 0x00)
#
# nameslist_offsets:
# char, alias, alias_count, note, note_count, approxEquiv, approxEquiv_coutn, equiv, equiv_count, seeAlso, seeAlso_count
# 16    32     8            32    8           32           8                  32     8            32       8
# => each entry 27 bytes
#
# blocks_offsets:
# each entry 4 bytes
# 16bit: start unicode
# 16bit: end unicode
# Note that there is no string offset.
#
# section_offsets:
# each entry 4 bytes
# 16bit: section offset
# 16bit: block offset
# Note that these offsets are _not_ positions in the data file but indexes.
# For example 0x0403 means the fourth section includes the third block.
#
# unihan_offsets:
# each entry 30 bytes
# 16bit: unicode
# 32bit: offset to unihan_strings for Definition
# 32bit: offset to unihan_strings for Cantonese
# 32bit: offset to unihan_strings for Mandarin
# 32bit: offset to unihan_strings for Tang
# 32bit: offset to unihan_strings for Korean
# 32bit: offset to unihan_strings for JapaneseKun
# 32bit: offset to unihan_strings for JapaneseOn

from struct import *
import sys
import re
import io

# based on http://www.unicode.org/charts/
sectiondata = '''
SECTION European Scripts
Armenian
Caucasian Albanian
Cypriot Syllabary
Cyrillic
Cyrillic Supplement
Cyrillic Extended-A
Cyrillic Extended-B
Cyrillic Extended-C
Elbasan
Georgian
Georgian Supplement
Glagolitic
Glagolitic Supplement
Gothic
Greek and Coptic
Greek Extended
Ancient Greek Numbers
Basic Latin
Latin-1 Supplement
Latin Extended-A
Latin Extended-B
Latin Extended-C
Latin Extended-D
Latin Extended-E
Latin Extended Additional
IPA Extensions
Phonetic Extensions
Phonetic Extensions Supplement
Linear A
Linear B Syllabary
Linear B Ideograms
Aegean Numbers
Ogham
Old Hungarian
Old Italic
Old Permic
Phaistos Disc
Runic
Shavian

SECTION Modifier Letters
Modifier Tone Letters
Spacing Modifier Letters
Superscripts and Subscripts

SECTION Combining Marks
Combining Diacritical Marks
Combining Diacritical Marks Extended
Combining Diacritical Marks Supplement
Combining Diacritical Marks for Symbols
Combining Half Marks

SECTION African Scripts
Adlam
Bamum
Bamum Supplement
Bassa Vah
Coptic
Coptic Epact Numbers
Egyptian Hieroglyphs
Ethiopic
Ethiopic Supplement
Ethiopic Extended
Ethiopic Extended-A
Mende Kikakui
Meroitic Cursive
Meroitic Hieroglyphs
NKo
Osmanya
Tifinagh
Vai

SECTION Middle Eastern Scripts
Anatolian Hieroglyphs
Arabic
Arabic Supplement
Arabic Extended-A
Arabic Presentation Forms-A
Arabic Presentation Forms-B
Imperial Aramaic
Avestan
Carian
Cuneiform
Cuneiform Numbers and Punctuation
Early Dynastic Cuneiform
Old Persian
Ugaritic
Hatran
Hebrew
Lycian
Lydian
Mandaic
Nabataean
Old North Arabian
Old South Arabian
Inscriptional Pahlavi
Psalter Pahlavi
Palmyrene
Inscriptional Parthian
Phoenician
Samaritan
Syriac

SECTION Central Asian Scripts
Manichaean
Marchen
Mongolian
Mongolian Supplement
Old Turkic
Phags-pa
Tibetan

SECTION South Asian Scripts
Ahom
Bengali
Bhaiksuki
Brahmi
Chakma
Devanagari
Devanagari Extended
Grantha
Gujarati
Gurmukhi
Kaithi
Kannada
Kharoshthi
Khojki
Khudawadi
Lepcha
Limbu
Mahajani
Malayalam
Meetei Mayek
Meetei Mayek Extensions
Modi
Mro
Multani
Newa
Ol Chiki
Oriya
Saurashtra
Sharada
Siddham
Sinhala
Sinhala Archaic Numbers
Sora Sompeng
Syloti Nagri
Takri
Tamil
Telugu
Thaana
Tirhuta
Vedic Extensions
Warang Citi

SECTION Southeast Asian Scripts
Cham
Kayah Li
Khmer
Khmer Symbols
Lao
Myanmar
Myanmar Extended-A
Myanmar Extended-B
New Tai Lue
Pahawh Hmong
Pau Cin Hau
Tai Le
Tai Tham
Tai Viet
Thai

SECTION Indonesia &amp; Oceania Scripts
Balinese
Batak
Buginese
Buhid
Hanunoo
Javanese
Rejang
Sundanese
Sundanese Supplement
Tagalog
Tagbanwa

SECTION East Asian Scripts
Bopomofo
Bopomofo Extended
CJK Unified Ideographs
CJK Unified Ideographs Extension A
CJK Unified Ideographs Extension B
CJK Unified Ideographs Extension C
CJK Unified Ideographs Extension D
CJK Unified Ideographs Extension E
CJK Compatibility Ideographs
CJK Compatibility Ideographs Supplement
Kangxi Radicals
CJK Radicals Supplement
CJK Strokes
Ideographic Description Characters
Hangul Jamo
Hangul Jamo Extended-A
Hangul Jamo Extended-B
Hangul Compatibility Jamo
Hangul Syllables
Hiragana
Katakana
Katakana Phonetic Extensions
Kana Supplement
Kanbun
Lisu
Miao
Tangut
Tangut Components
Yi Syllables
Yi Radicals

SECTION American Scripts
Cherokee
Cherokee Supplement
Deseret
Osage
Unified Canadian Aboriginal Syllabics
Unified Canadian Aboriginal Syllabics Extended

SECTION Other
Alphabetic Presentation Forms
Halfwidth and Fullwidth Forms

SECTION Notational Systems
Braille Patterns
Musical Symbols
Ancient Greek Musical Notation
Byzantine Musical Symbols
Duployan
Shorthand Format Controls
Sutton SignWriting

SECTION Punctuation
General Punctuation
Supplemental Punctuation
CJK Symbols and Punctuation
Ideographic Symbols and Punctuation
CJK Compatibility Forms
Halfwidth and Fullwidth Forms
Small Form Variants
Vertical Forms

SECTION Alphanumeric Symbols
Letterlike Symbols
Mathematical Alphanumeric Symbols
Arabic Mathematical Alphabetic Symbols
Enclosed Alphanumerics
Enclosed Alphanumeric Supplement
Enclosed CJK Letters and Months
Enclosed Ideographic Supplement
CJK Compatibility

SECTION Technical Symbols
Control Pictures
Miscellaneous Technical
Optical Character Recognition

SECTION Numbers &amp; Digits
Common Indic Number Forms
Coptic Epact Numbers
Counting Rod Numerals
Cuneiform Numbers and Punctuation
Number Forms
Rumi Numeral Symbols
Sinhala Archaic Numbers

SECTION Mathematical Symbols
Arrows
Supplemental Arrows-A
Supplemental Arrows-B
Supplemental Arrows-C
Miscellaneous Symbols and Arrows
Mathematical Alphanumeric Symbols
Arabic Mathematical Alphabetic Symbols
Letterlike Symbols
Mathematical Operators
Supplemental Mathematical Operators
Miscellaneous Mathematical Symbols-A
Miscellaneous Mathematical Symbols-B
Geometric Shapes
Box Drawing
Block Elements
Geometric Shapes Extended

SECTION Emoji & Pictographs
Dingbats
Ornamental Dingbats
Emoticons
Miscellaneous Symbols
Miscellaneous Symbols and Pictographs
Supplemental Symbols and Pictographs
Transport and Map Symbols

SECTION Other Symbols
Alchemical Symbols
Ancient Symbols
Currency Symbols
Domino Tiles
Mahjong Tiles
Playing Cards
Miscellaneous Symbols and Arrows
Yijing Hexagram Symbols
Tai Xuan Jing Symbols

SECTION Specials
Specials
Tags
Variation Selectors
Variation Selectors Supplement

SECTION Private Use
Private Use Area
Supplementary Private Use Area-A
Supplementary Private Use Area-B

SECTION Surrogates
High Surrogates
High Private Use Surrogates
Low Surrogates
'''
# TODO: rename "Other Scripts" to "American Scripts"

categoryMap = { # same values as QChar::Category
    "Mn": 1,
    "Mc": 2,
    "Me": 3,
    "Nd": 4,
    "Nl": 5,
    "No": 6,
    "Zs": 7,
    "Zl": 8,
    "Zp": 9,
    "Cc": 10,
    "Cf": 11,
    "Cs": 12,
    "Co": 13,
    "Cn": 14,
    "Lu":  15,
    "Ll":  16,
    "Lt":  17,
    "Lm":  18,
    "Lo":  19,
    "Pc":  20,
    "Pd":  21,
    "Ps":  22,
    "Pe":  23,
    "Pi":  24,
    "Pf":  25,
    "Po":  26,
    "Sm":  27,
    "Sc":  28,
    "Sk":  29,
    "So":  30
}


class Names:
    def __init__(self):
        self.names = []
        self.controlpos = -1
    def addName(self, uni, name, category):
        self.names.append([uni, name, category])

    def calculateStringSize(self):
        size = 0
        hadcontrol = False
        for entry in self.names:
            if entry[1] == "<control>":
                if not hadcontrol:
                    size += len(entry[1].encode('utf-8')) + 2
                    hadcontrol = True
            else:
                size += len(entry[1].encode('utf-8')) + 2
        return size

    def calculateOffsetSize(self):
        return len(self.names)*8

    def writeStrings(self, out, pos):
        hadcontrol = False
        for entry in self.names:
            if entry[1] == "<control>":
                if not hadcontrol:
                    out.write(pack("=b", entry[2]))
                    out.write(entry[1].encode('utf-8') + b"\0")
                    size = len(entry[1].encode('utf-8')) + 2
                    entry[1] = pos
                    self.controlpos = pos
                    pos += size
                    hadcontrol = True
                else:
                    entry[1] = self.controlpos
            else:
                out.write(pack("=b", entry[2]))
                out.write(entry[1].encode('utf-8') + b"\0")
                size = len(entry[1].encode('utf-8')) + 2
                entry[1] = pos
                pos += size
        return pos

    def writeOffsets(self, out, pos):
        for entry in self.names:
            out.write(pack("=II", int(entry[0], 16), entry[1]))
            pos += 8
        return pos

class Details:
    def __init__(self):
        self.details = {}
    def addEntry(self, char, category, text):
        if char not in self.details:
            self.details[char] = {}
        if category not in self.details[char]:
            self.details[char][category] = []
        self.details[char][category].append(text)

    def calculateStringSize(self):
        size = 0
        for char in self.details.values():
            for cat in char.values():
                for s in cat:
                    if type(s) is str:
                        size += len(s.encode('utf-8')) + 1
                    else:
                        size += 4
        return size

    def calculateOffsetSize(self):
        return len(self.details)*29

    def writeStrings(self, out, pos):
        for char in self.details.values():
            for cat in char.values():
                for i in range(0, len(cat)):
                    s = cat[i]
                    if type(s) is str:
                        out.write(s.encode('utf-8') + b"\0")
                        size = len(s.encode('utf-8')) + 1
                    else:
                        out.write(pack("=I", s))
                        size = 4
                    cat[i] = pos
                    pos += size
        return pos

    def writeOffsets(self, out, pos):
        for char in self.details.keys():
            alias = 0
            alias_count = 0
            note = 0
            note_count = 0
            approxEquiv = 0
            approxEquiv_count = 0
            equiv = 0
            equiv_count = 0
            seeAlso = 0
            seeAlso_count = 0
            if "alias" in self.details[char]:
                alias = self.details[char]["alias"][0]
                alias_count = len(self.details[char]["alias"])

            if "note" in self.details[char]:
                note = self.details[char]["note"][0]
                note_count = len(self.details[char]["note"])

            if "approxEquiv" in self.details[char]:
                approxEquiv = self.details[char]["approxEquiv"][0]
                approxEquiv_count = len(self.details[char]["approxEquiv"])

            if "equiv" in self.details[char]:
                equiv = self.details[char]["equiv"][0]
                equiv_count = len(self.details[char]["equiv"])

            if "seeAlso" in self.details[char]:
                seeAlso = self.details[char]["seeAlso"][0]
                seeAlso_count = len(self.details[char]["seeAlso"])

            out.write(pack("=IIbIbIbIbIb", char, alias, alias_count, note, note_count, approxEquiv, approxEquiv_count, equiv, equiv_count, seeAlso, seeAlso_count))
            pos += 29

        return pos

class SectionsBlocks:
    def __init__(self):
        self.sections = []
        self.blocks = []
        self.blockList = []
        self.sectionList = []

    def addBlock(self, begin, end, name):
        self.blocks.append([begin, end, name])
        self.blockList.append(name)

    def addSection(self, section, block):
        self.sections.append([section, block])
        if not section in self.sectionList:
            self.sectionList.append(section)

    def calculateBlockStringSize(self):
        size = 0
        for block in self.blocks:
            size += len(block[2].encode('utf-8')) + 1
        return size

    def calculateBlockOffsetSize(self):
        return len(self.blocks) * 8

    def calculateSectionStringSize(self):
        size = 0
        lastsection = ""
        for section in self.sections:
            if section[0] != lastsection:
                size += len(section[0].encode('utf-8')) + 1
                lastsection = section[0]
        return size

    def calculateSectionOffsetSize(self):
        return len(self.sections) * 8

    def writeBlockStrings(self, out, pos):
        index = 0
        for block in self.blocks:
            out.write(block[2].encode('utf-8') + b"\0")
            size = len(block[2].encode('utf-8')) + 1
            found = False
            for section in self.sections:
                print(section)
                if section[1] == block[2]:
                    print("found", section)
                    section[1] = int(index)
                    found = True
            if not found:
                print("Error: Did not find any category for block \""+block[2]+"\"")
                sys.exit(1)
            block[2] = index
            pos += size
            index += 1
        return pos

    def writeBlockOffsets(self, out, pos):
        for block in self.blocks:
            out.write(pack("=II", int(block[0], 16), int(block[1], 16)))
            pos += 8
        return pos

    def writeSectionStrings(self, out, pos):
        lastsection = ""
        lastpos = 0
        index = -1
        for section in self.sections:
            if section[0] != lastsection:
                index += 1
                lastsection = section[0]
                out.write(section[0].encode('utf-8') + b"\0")
                size = len(section[0].encode('utf-8')) + 1
                section[0] = index
                lastpos = pos
                pos += size
            else:
                section[0] = index
        return pos

    def writeSectionOffsets(self, out, pos):
        print(self.sections)
        for section in self.sections:
            out.write(pack("=II", section[0], section[1]))
            pos += 8
        return pos

    def getBlockList(self):
        return self.blockList

    def getSectionList(self):
        return self.sectionList

class Unihan:
    def __init__(self):
        self.unihan = {}

    def addUnihan(self, uni, category, value):
        uni = int(uni, 16)
        if category != "kDefinition" and category != "kCantonese" and category != "kMandarin" and category != "kTang" and category != "kKorean" and category != "kJapaneseKun" and category != "kJapaneseOn":
            return
        if uni not in self.unihan:
            self.unihan[uni] = [None, None, None, None, None, None, None]
        if category == "kDefinition":
            self.unihan[uni][0] = value
        elif category == "kCantonese":
            self.unihan[uni][1] = value
        elif category == "kMandarin":
            self.unihan[uni][2] = value
        elif category == "kTang":
            self.unihan[uni][3] = value
        elif category == "kKorean":
            self.unihan[uni][4] = value
        elif category == "kJapaneseKun":
            self.unihan[uni][5] = value
        elif category == "kJapaneseOn":
            self.unihan[uni][6] = value

    def calculateStringSize(self):
        size = 0
        for char in self.unihan.keys():
            for entry in self.unihan[char]:
                if entry != None:
                    size += len(entry.encode('utf-8')) + 1
        return size

    def calculateOffsetSize(self):
        return len(self.unihan) * 32

    def writeStrings(self, out, pos):
        for char in self.unihan.keys():
            for i in range(0, 7):
                if self.unihan[char][i] != None:
                    out.write(self.unihan[char][i].encode('utf-8') + b"\0")
                    size = len(self.unihan[char][i].encode('utf-8')) + 1
                    self.unihan[char][i] = pos
                    pos += size
        return pos

    def writeOffsets(self, out, pos):
        for char in self.unihan.keys():
            out.write(pack("=I", char))
            for i in range(0, 7):
                if self.unihan[char][i] != None:
                    out.write(pack("=I", self.unihan[char][i]))
                else:
                    out.write(pack("=I", 0))
            pos += 32
        return pos

class Parser:
    def parseUnicodeData(self, inUnicodeData, names):
        regexp = re.compile(r'^([^;]+);([^;]+);([^;]+)')
        for line in inUnicodeData:
            line = line[:-1]
            m = regexp.match(line)
            if not m:
                continue
            uni = m.group(1)
            name = m.group(2)
            category = m.group(3)
            if len(uni) > 8:
                continue
            names.addName(uni, name, categoryMap[category])

    def parseDetails(self, inNamesList, details):
        invalidRegexp = re.compile(r'^@')
        unicodeRegexp = re.compile(r'^([0-9A-F]+)')

        aliasRegexp = re.compile(r'^\s+=\s+(.+)$') #equal
        seeAlsoRegexp = re.compile(r'^\s+x\s+.*([0-9A-F]{4,6})\)$') #ex
        noteRegexp = re.compile(r'^\s+\*\s+(.+)$') #star
        approxEquivalentRegexp = re.compile(r'^\s+#\s+(.+)$') #pound
        equivalentRegexp = re.compile(r'^\s+:\s+(.+)$') #colon

        drop = 0
        currChar = 0

        for line in inNamesList:
            line = line[:-1]
            m1 = unicodeRegexp.match(line)
            m2 = aliasRegexp.match(line)
            m3 = noteRegexp.match(line)
            m4 = approxEquivalentRegexp.match(line)
            m5 = equivalentRegexp.match(line)
            m6 = seeAlsoRegexp.match(line)
            if invalidRegexp.match(line):
                continue
            elif m1:
                currChar = int(m1.group(1), 16)
                if len(m1.group(1)) > 8: #limit to 32bit
                    drop = 1
                    continue
            elif drop == 1:
                continue
            elif m2:
                value = m2.group(1)
                details.addEntry(currChar, "alias", value)
            elif m3:
                value = m3.group(1)
                details.addEntry(currChar, "note", value)
            elif m4:
                value = m4.group(1)
                details.addEntry(currChar, "approxEquiv", value)
            elif m5:
                value = m5.group(1)
                details.addEntry(currChar, "equiv", value)
            elif m6:
                value = int(m6.group(1), 16)
                details.addEntry(currChar, "seeAlso", value)
    def parseBlocks(self, inBlocks, sectionsBlocks):
        regexp = re.compile(r'^([0-9A-F]+)\.\.([0-9A-F]+); (.+)$')
        for line in inBlocks:
            line = line[:-1]
            m = regexp.match(line)
            if not m:
                continue
            if len(m.group(1)) > 8:
                continue
            sectionsBlocks.addBlock(m.group(1), m.group(2), m.group(3))
    def parseSections(self, inSections, sectionsBlocks):
        currSection = ""
        for line in inSections:
            line = line[:-1]
            if len(line) == 0:
                continue
            temp = line.split(" ")
            if temp[0] == "SECTION":
                currSection = line[8:]
            elif currSection != "" and line != "":
                sectionsBlocks.addSection(currSection, line)
            else:
                print("error in data file")
                sys.exit(1)
    def parseUnihan(self, inUnihan, unihan):
        regexp = re.compile(r'^U\+([0-9A-F]+)\s+([^\s]+)\s+(.+)$')
        count = 0
        for line in inUnihan:
            if count % 100000 == 0:
                print("\b.", end=' ')
                sys.stdout.flush()
            count += 1
            line = line[:-1]
            m = regexp.match(line)
            if not m:
                continue
            if len(m.group(1)) <= 4:
                unihan.addUnihan(m.group(1), m.group(2), m.group(3))

def writeTranslationDummy(out, data):
    out.write(b"""/* This file is part of the KDE libraries

   Copyright (C) 2007 Daniel Laidig <d.laidig@gmx.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.

   This file is autogenerated by kdeutils/kcharselect/kcharselect-generate-datafile.py
*/\n\n""")
    for group in data:
        for entry in group[1]:
            out.write(b"I18N_NOOP2(\""+group[0].encode('utf-8')+b"\", \""+entry.encode('utf-8')+b"\");\n")

out = open("kcharselect-data", "wb")
outTranslationDummy = open("kcharselect-translation.cpp", "wb")

inUnicodeData = open("UnicodeData.txt", "r")
inNamesList = open("NamesList.txt", "r")
inBlocks = open("Blocks.txt", "r")
inSections = io.StringIO(sectiondata)
inUnihan = open("Unihan_Readings.txt", "r")

if calcsize('=H') != 2 or calcsize('=I') != 4:
    print("Error: Sizes of ushort and uint are not 16 and 32 bit as expected")
    sys.exit(1)

names = Names()
details = Details()
sectionsBlocks = SectionsBlocks()
unihan = Unihan()

parser = Parser()

print("========== parsing files ===================")
parser.parseUnicodeData(inUnicodeData, names)
print(".", end=' ')
sys.stdout.flush()
parser.parseDetails(inNamesList, details)
print("\b.", end=' ')
sys.stdout.flush()
parser.parseBlocks(inBlocks, sectionsBlocks)
print("\b.", end=' ')
sys.stdout.flush()
parser.parseSections(inSections, sectionsBlocks)
print("\b.", end=' ')
sys.stdout.flush()
parser.parseUnihan(inUnihan, unihan)
print("\b.", end=' ')
sys.stdout.flush()

print("done.")

pos = 0

#write header, size: 40 bytes
print("========== writing header ==================")
out.write(pack("=I", 40))
print("names strings begin", 40)

namesOffsetBegin = names.calculateStringSize() + 40
out.write(pack("=I", namesOffsetBegin))
print("names offsets begin", namesOffsetBegin)

detailsStringBegin = namesOffsetBegin + names.calculateOffsetSize()
out.write(pack("=I", detailsStringBegin))
print("details strings begin", detailsStringBegin)

detailsOffsetBegin = detailsStringBegin + details.calculateStringSize()
out.write(pack("=I", detailsOffsetBegin))
print("details offsets begin", detailsOffsetBegin)

blocksStringBegin = detailsOffsetBegin + details.calculateOffsetSize()
out.write(pack("=I", blocksStringBegin))
print("block strings begin", blocksStringBegin)

blocksOffsetBegin = blocksStringBegin + sectionsBlocks.calculateBlockStringSize()
out.write(pack("=I", blocksOffsetBegin))
print("block offsets begin", blocksOffsetBegin)

sectionStringBegin = blocksOffsetBegin + sectionsBlocks.calculateBlockOffsetSize()
out.write(pack("=I", sectionStringBegin))
print("section strings begin", sectionStringBegin)

sectionOffsetBegin = sectionStringBegin + sectionsBlocks.calculateSectionStringSize()
out.write(pack("=I", sectionOffsetBegin))
print("section offsets begin", sectionOffsetBegin)

unihanStringBegin = sectionOffsetBegin + sectionsBlocks.calculateSectionOffsetSize()
out.write(pack("=I", unihanStringBegin))
print("unihan strings begin", unihanStringBegin)

unihanOffsetBegin = unihanStringBegin + unihan.calculateStringSize()
out.write(pack("=I", unihanOffsetBegin))
print("unihan offsets begin", unihanOffsetBegin)

end = unihanOffsetBegin + unihan.calculateOffsetSize()
print("end should be", end)

pos += 40

print("========== writing data ====================")

pos = names.writeStrings(out, pos)
print("names strings written, position", pos)
pos = names.writeOffsets(out, pos)
print("names offsets written, position", pos)
pos = details.writeStrings(out, pos)
print("details strings written, position", pos)
pos = details.writeOffsets(out, pos)
print("details offsets written, position", pos)
pos = sectionsBlocks.writeBlockStrings(out, pos)
print(sectionsBlocks.sections)
print("block strings written, position", pos)
pos = sectionsBlocks.writeBlockOffsets(out, pos)
print("block offsets written, position", pos)
pos = sectionsBlocks.writeSectionStrings(out, pos)
print("section strings written, position", pos)
pos = sectionsBlocks.writeSectionOffsets(out, pos)
print("section offsets written, position", pos)
pos = unihan.writeStrings(out, pos)
print("unihan strings written, position", pos)
pos = unihan.writeOffsets(out, pos)
print("unihan offsets written, position", pos)

print("========== writing translation dummy  ======")
translationData = [["KCharSelect section name", sectionsBlocks.getSectionList()], ["KCharselect unicode block name",sectionsBlocks.getBlockList()]]
writeTranslationDummy(outTranslationDummy, translationData)
print("done. make sure to copy both kcharselect-data and kcharselect-translation.cpp.")
