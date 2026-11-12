#!/bin/bash
# Copia a ./salida los headers y datos de configuración de los chips soportados.
O=$PWD/salida; mkdir -p $O
X8=$(ls -d /opt/microchip/xc8/v* | tail -1); X16=$(ls -d /opt/microchip/xc16/v* | tail -1); X32=$(ls -d /opt/microchip/xc32/v* | tail -1)
{
  for d in $X8 $X16 $X32; do echo "== $d"; ls $d; ls $d/bin | head -60; done
  $X8/bin/xc8-cc --version 2>&1 | head -3; $X16/bin/xc16-gcc --version 2>&1 | head -2; $X32/bin/xc32-gcc --version 2>&1 | head -2
  echo "== xc8 pic"; ls $X8/pic; ls $X8/pic/include | head -40; ls $X8/pic/dat
  echo "== xc16 support"; ls $X16/support; ls $X16/docs | head -30
  echo "== xc32"; ls $X32/pic32mx; ls $X32/pic32mx/include | head -30; ls $X32/docs | head -30
  find $X32 -iname '*32mm0256gpm064*' | head -40
  find $X32 -maxdepth 3 -type d | head -80
} > $O/info.txt 2>&1
mkdir -p $O/xc8 $O/xc16 $O/xc32
for c in 16f84 16f84a 16f88 16f690 16f870 16f872 16f874 16f874a 16f877 16f877a 16f1947 16f15385 16f18877 18f2520 18f2525 18f2550 18f2420 18f2455 18f57q84; do
  find $X8/pic -iname "pic$c.h" -exec cp {} $O/xc8/ \;
  find $X8/pic/dat -iname "$c.cfgdata" -exec cp {} $O/xc8/ \;
done
cp $X8/pic/include/xc.h $X8/pic/include/pic.h $X8/pic/include/pic18.h $X8/pic/include/htc.h $O/xc8/ 2>/dev/null
find $X8/pic/include -maxdepth 1 -name '*.h' | sed 's|.*/||' | tr '\n' ' ' > $O/xc8/lista_include.txt
$X8/bin/xc8-cc --help > $O/xc8/ayuda.txt 2>&1
for c in 24FJ16GA002 33EP32GS502 33EP64MC206 33CK64MC105; do
  find $X16/support -name "p$c.h" -exec cp {} $O/xc16/ \;
  find $X16/support -name "p$c.gld" -exec cp {} $O/xc16/ \;
  find $X16/docs -iname "*$c*" -exec cp {} $O/xc16/ \;
done
find $X16/support -maxdepth 2 | head -60 > $O/xc16/arbol.txt
for f in $(find $X32 -iname '*32mm0256gpm064*' -type f | grep -v '\.a$\|\.o$' | head -30); do cp $f $O/xc32/ 2>/dev/null; done
find $X32 -ipath '*32mm0256gpm064*' | head -60 > $O/xc32/arbol.txt
du -sh $O/*
