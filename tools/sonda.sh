#!/bin/bash
# Sonda de desarrollo: copia a la rama "sonda" los headers de los chips
# soportados (para escribir el núcleo contra los nombres reales de registros).
O=$PWD/salida; mkdir -p $O/xc8 $O/xc16 $O/xc32
X8=/opt/microchip/xc8/v2.50; X16=$(ls -d /opt/microchip/xc16/v* | tail -1); X32=$(ls -d /opt/microchip/xc32/v* | tail -1)
for c in 16f84 16f84a 16f88 16f690 16f870 16f872 16f874 16f874a 16f877 16f877a 16f1947 16f15385 16f18877 18f2520 18f2525 18f2550 18f2420 18f2455 18f57q84; do
  find $X8 -iname "pic$c.h" -exec cp {} $O/xc8/ \;
  find $X8 -iname "$c.cfgdata" -exec cp {} $O/xc8/ \;
done
ls $X8/pic/include/proc | grep -i '18f25\|18f24' | tr '\n' ' ' > $O/xc8/lista_18f2xxx.txt
for c in 24FJ16GA002 33EP32GS502 33EP64MC206 33CK64MC105; do
  find $X16/support -name "p$c.h" -exec cp {} $O/xc16/ \;
  find $X16/support -name "p$c.gld" -exec cp {} $O/xc16/ \;
done
# formato del informe de memoria de XC16
cat > $O/xc16/t.c <<'XX'
#include <xc.h>
volatile int n;
int main(void) { for (;;) { n++; } }
XX
( cd $O/xc16 && $X16/bin/xc16-gcc -mcpu=24FJ16GA002 -omf=elf -O1 t.c -o t.elf -Wl,--script=p24FJ16GA002.gld,--report-mem > mem_stdout.txt 2> mem_stderr.txt; $X16/bin/xc16-size t.elf > size.txt 2>&1; $X16/bin/xc16-size -A t.elf > sizeA.txt 2>&1; rm -f t.elf )
# AVR y ARM en XC8/XC32: qué traen
{ ls $X8; ls $X8/avr 2>/dev/null | head; ls $X8/dfp 2>/dev/null | head -20; ls $X32; ls $X32/bin | head -80; ls $X32/arm 2>/dev/null | head; } > $O/info.txt 2>&1
du -sh $O/*
