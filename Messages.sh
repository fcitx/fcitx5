#!/bin/bash
DOMAIN=$(basename $PWD)
POT_FILE=po/$DOMAIN.pot
set -x
XGETTEXT="xgettext --add-comments --sort-output --msgid-bugs-address=fcitx-dev@googlegroups.com"
source_files=$(find . -name \*.cpp -o -name \*.h)
$XGETTEXT --keyword=_ --keyword=N_ --language=C++ -o ${POT_FILE} $source_files
desktop_files=$(find . -name \*.conf.in -o -name \*.desktop)
$XGETTEXT --language=Desktop $desktop_files -j -o ${POT_FILE}

sed -i 's|^"Content-Type: text/plain; charset=CHARSET\\n"|"Content-Type: text/plain; charset=utf-8\\n"|g' ${POT_FILE}

# Due to transifex problem, delete the date.
#sed -i '/^"PO-Revision-Date/d' ${POT_FILE}
sed -i 's|^"Language: \\n"|"Language: LANG\\n"|g' ${POT_FILE}

echo > po/LINGUAS

for pofile in $(ls po/*.po | sort); do
  pofilebase=$(basename $pofile)
  pofilebase=${pofilebase/.po/}
  msgmerge -U --backup=none $pofile ${POT_FILE}
  project_line=$(grep "Project-Id-Version" ${POT_FILE} | head -n 1 | tr --delete '\n' | sed -e 's/[\/&]/\\&/g')
  sed -i "s|.*Project-Id-Version.*|$project_line|g" $pofile
  echo $pofilebase >> po/LINGUAS
done
