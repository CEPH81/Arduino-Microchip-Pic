#!/bin/bash
# Sonda de desarrollo: copia a la rama "sonda" los textos de licencia que
# traen los compiladores instalados (para saber si se pueden redistribuir).
O=$PWD/salida; mkdir -p $O
for d in /opt/microchip/xc8/* /opt/microchip/xc16/* /opt/microchip/xc32/*; do
  n=$(echo $d | sed 's|/opt/microchip/||; s|/|_|g'); mkdir -p $O/$n
  find $d -maxdepth 3 \( -iname '*licen*' -o -iname '*eula*' -o -iname 'copying*' -o -iname '*notice*' -o -iname '*agreement*' \) -type f -size -2000k 2>/dev/null | head -40 > $O/$n/lista.txt
  i=0; while read f; do i=$((i+1)); cp "$f" "$O/$n/$(printf %02d $i)_$(basename "$f")" 2>/dev/null; done < $O/$n/lista.txt
  ls $d $d/docs 2>/dev/null | head -80 > $O/$n/ls.txt
  du -sh $d > $O/$n/du.txt 2>/dev/null
done
for f in $(find $O -iname '*.pdf'); do pdftotext "$f" "$f.txt" 2>/dev/null; done
du -sh $O/*
