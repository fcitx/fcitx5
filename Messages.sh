#!/bin/bash
set -x
source_files=$(find . -name \*.cpp -o -name \*.h)
xgettext --keyword=_ --language=C++ --add-comments --sort-output -o po/fcitx.pot $source_files
desktop_files=$(find . -name \*.conf.in)
xgettext --language=Desktop $desktop_files --keyword= --keyword=GeneralName --keyword=Comment -j -o po/fcitx.pot

echo > po/LINGUAS

for pofile in $(ls po/*.po | sort); do
  pofilebase=$(basename $pofile)
  pofilebase=${pofilebase/.po/}
  msgmerge -U $pofile po/fcitx.pot
  echo $pofilebase >> po/LINGUAS
done
