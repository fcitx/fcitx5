#!/bin/bash
POT_FILE=po/fcitx5.pot
set -x
source_files=$(find . -name \*.cpp -o -name \*.h)
xgettext --keyword=_ --language=C++ --add-comments --sort-output -o ${POT_FILE} $source_files
desktop_files=$(find . -name \*.conf.in)
xgettext --language=Desktop $desktop_files --keyword= --keyword=GeneralName --keyword=Comment -j -o ${POT_FILE}

echo > po/LINGUAS

for pofile in $(ls po/*.po | sort); do
  pofilebase=$(basename $pofile)
  pofilebase=${pofilebase/.po/}
  msgmerge -U $pofile ${POT_FILE}
  echo $pofilebase >> po/LINGUAS
done
