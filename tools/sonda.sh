#!/bin/bash
# Sonda de desarrollo: comprueba los enlaces de descarga de Microchip
# (instaladores de XC8/XC16/XC32 y MPLAB X para Windows, Linux y macOS).
O=$PWD/salida; mkdir -p $O
B=https://ww1.microchip.com/downloads/aemDocuments/documents/DEV/ProductDocuments/SoftwareTools
for f in \
  xc8-v2.50-full-install-windows-x64-installer.exe xc8-v2.50-full-install-linux-x64-installer.run xc8-v2.50-full-install-macos-x64-installer.dmg xc8-v2.50-full-install-osx-x64-installer.dmg \
  xc8-v3.10-full-install-windows-x64-installer.exe xc8-v3.10-full-install-linux-x64-installer.run xc8-v3.10-full-install-macos-x64-installer.dmg xc8-v3.10-full-install-macos-installer.dmg \
  xc16-v2.10-full-install-windows64-installer.exe xc16-v2.10-full-install-linux64-installer.run xc16-v2.10-full-install-osx64-installer.dmg \
  xc32-v5.00-full-install-windows-x64-installer.exe xc32-v5.00-full-install-linux-x64-installer.run xc32-v5.00-full-install-macos-x64-installer.dmg xc32-v5.00-full-install-osx-x64-installer.dmg \
  xc32-v4.45-full-install-windows-x64-installer.exe xc32-v4.45-full-install-linux-x64-installer.run xc32-v4.45-full-install-osx-x64-installer.dmg \
  MPLABX-v6.20-windows-installer.exe MPLABX-v6.20-linux-installer.tar MPLABX-v6.20-osx-installer.dmg \
  MPLABX-v6.15-windows-installer.exe MPLABX-v6.25-windows-installer.exe MPLABX-v6.25-linux-installer.tar ; do
  r=$(curl -sIL -m 40 "$B/$f" | tr -d '\r' | awk 'BEGIN{c="?";l="?"} /^HTTP/{c=$2} tolower($1)=="content-length:"{l=$2} END{print c, l}')
  echo "$f $r"
done > $O/enlaces.txt
cat $O/enlaces.txt
# DFP: índice de packs y licencia
curl -sL -m 60 https://packs.download.microchip.com/index.idx -o /tmp/idx.xml; ls -la /tmp/idx.xml
grep -o '<pdsc [^>]*name="Microchip\.\(PIC16Fxxx\|PIC16F1xxxx\|PIC18Fxxxx\|PIC18F-Q\|PIC12-16F1xxx\)_DFP[^>]*>' /tmp/idx.xml | head -20 > $O/dfp.txt
head -c 3000 /tmp/idx.xml > $O/idx_inicio.txt
