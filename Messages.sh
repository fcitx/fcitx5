#!/bin/bash
DOMAIN=$(basename $PWD)
POT_FILE=po/$DOMAIN.pot
set -x
XGETTEXT="xgettext --add-comments --sort-output --package-name=${DOMAIN} --msgid-bugs-address=fcitx-dev@googlegroups.com"
source_files=$(find . -name \*.cpp -o -name \*.h)
$XGETTEXT --keyword=_ --keyword=N_ --language=C++ -o ${POT_FILE} $source_files
desktop_files=$(find . -name \*.conf.in -o -name \*.desktop)
$XGETTEXT --language=Desktop $desktop_files -j -o ${POT_FILE}

echo > po/LINGUAS

for pofile in $(ls po/*.po | sort); do
  pofilebase=$(basename $pofile)
  pofilebase=${pofilebase/.po/}
  msgmerge -U --backup=none $pofile ${POT_FILE}
  project_line=$(grep "Project-Id-Version" ${POT_FILE} | head -n 1 | tr --delete '\n' | sed -e 's/[\/&]/\\&/g')
  sed -i "s|.*Project-Id-Version.*|$project_line|g" $pofile
  echo $pofilebase >> po/LINGUAS
done
