#!/bin/bash
# Instala XC8, XC16 y XC32 en /opt/microchip (runner de GitHub Actions).
# Uso: instalar_xc.sh [xc8] [xc16] [xc32]   (sin argumentos: los tres)
set -e
BASE=https://ww1.microchip.com/downloads/aemDocuments/documents/DEV/ProductDocuments/SoftwareTools
XC8=${XC8_VER:-v3.10}; XC16=${XC16_VER:-v2.10}; XC32=${XC32_VER:-v5.00}
uno() {   # nombre archivo destino
  echo "::group::$1"
  curl -fsSL -o /tmp/$1.run "$BASE/$2"
  chmod +x /tmp/$1.run
  sudo /tmp/$1.run --mode unattended --netservername localhost --prefix "$3" \
    || sudo /tmp/$1.run --mode unattended --prefix "$3"
  rm -f /tmp/$1.run
  echo "::endgroup::"
}
sudo dpkg --add-architecture i386
sudo apt-get update -qq
sudo apt-get install -y -qq libc6:i386 libx11-6:i386 libxext6:i386 libstdc++6:i386 libexpat1:i386
for c in ${*:-xc8 xc16 xc32}; do
  case $c in
    xc8)  uno xc8  xc8-$XC8-full-install-linux-x64-installer.run   /opt/microchip/xc8/$XC8 ;;
    xc16) uno xc16 xc16-$XC16-full-install-linux64-installer.run   /opt/microchip/xc16/$XC16 ;;
    xc32) uno xc32 xc32-$XC32-full-install-linux-x64-installer.run /opt/microchip/xc32/$XC32 ;;
  esac
done
sudo chmod -R a+rX /opt/microchip
