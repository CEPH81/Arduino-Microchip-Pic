#!/usr/bin/env python3
"""
gen_boards.py - Genera pic/boards.txt.

Cada chip es una "placa". Los menús:
  Reloj        oscilador y frecuencia (define F_CPU y las macros PIC_OSC_*)
  Programador  PICkit 3 (por defecto), PICkit 4/5, SNAP, ICD, PKOB o la unidad
               CURIOSITY de las placas Curiosity Nano
  Alimentación si el programador alimenta la placa (solo PICkit / ICD)

  python3 tools/gen_boards.py
"""
import os

M = 1000000

# ---- menús de reloj: (clave, texto, F_CPU, flags)
def xt(mhz):  return ("xt%d" % mhz, "Cristal %d MHz (XT)" % mhz, mhz * M, "-DPIC_OSC_XT")
def hs(mhz):  return ("hs%d" % mhz, "Cristal %d MHz (HS)" % mhz, mhz * M, "-DPIC_OSC_HS")
def irc(mhz, ircf, extra="", txt=""):
    return ("int%d" % mhz, "Interno %d MHz%s" % (mhz, txt), mhz * M, "-DPIC_OSC_INT -DPIC_OSC_IRCF=%d%s" % (ircf, extra))
def hf(mhz, div):
    return ("int%d" % mhz, "Interno %d MHz" % mhz, mhz * M, "-DPIC_OSC_INT -DPIC_OSC_DIV=%d" % div)

CLK_84   = [xt(4), hs(8), hs(10)]
CLK_84A  = [xt(4), hs(8), hs(10), hs(16), hs(20)]
CLK_87X  = [xt(4), hs(8), hs(10), hs(16), hs(20)]
CLK_88   = [irc(8, 7), irc(4, 6), xt(4), hs(8), hs(16), hs(20)]
CLK_690  = [irc(8, 7), irc(4, 6), xt(4), hs(8), hs(16), hs(20)]
CLK_1947 = [irc(16, 15), irc(32, 14, " -DPIC_OSC_PLL", " (8 MHz x4)"), irc(8, 14), irc(4, 13), hs(16), hs(20)]
CLK_NEW  = [hf(32, 0), hf(16, 1), hf(8, 2), hf(4, 3), hs(16), hs(20)]
CLK_18   = [irc(8, 7), irc(32, 7, " -DPIC_OSC_PLL", " (8 MHz x4)"), irc(4, 6), xt(4), hs(8), hs(16), hs(20),
            ("hspll40", "Cristal 10 MHz x4 = 40 MHz (HSPLL)", 40 * M, "-DPIC_OSC_HSPLL")]
CLK_USB  = [irc(8, 7), ("usb48x20", "Cristal 20 MHz + PLL = 48 MHz", 48 * M, "-DPIC_OSC_USB48 -DPIC_OSC_PLLDIV=5"),
            ("usb48x8", "Cristal 8 MHz + PLL = 48 MHz", 48 * M, "-DPIC_OSC_USB48 -DPIC_OSC_PLLDIV=2"),
            ("usb48x4", "Cristal 4 MHz + PLL = 48 MHz", 48 * M, "-DPIC_OSC_USB48 -DPIC_OSC_PLLDIV=1"),
            irc(4, 6), xt(4), hs(8), hs(20)]
CLK_Q84  = [hf(64, 0), hf(32, 1), hf(16, 2), hf(8, 3)]
CLK_24FJ = [("pll", "Interno 8 MHz x4 (FCY = 16 MHz)", 16 * M, "-DPIC_OSC_PLL"), ("frc", "Interno 8 MHz (FCY = 4 MHz)", 4 * M, "")]
CLK_33EP = [("frc", "Interno 7.37 MHz (FCY = 3.685 MHz)", 3685000, ""), ("pll", "Interno + PLL (FCY = 70 MHz)", 70015000, "-DPIC_OSC_PLL")]
CLK_33CK = [("frc", "Interno 8 MHz (FCY = 4 MHz)", 4 * M, ""), ("pll", "Interno + PLL (FCY = 100 MHz)", 100 * M, "-DPIC_OSC_PLL")]
CLK_32MM = [("frc", "Interno 8 MHz", 8 * M, ""), ("pll", "Interno 8 MHz x3 = 24 MHz", 24 * M, "-DPIC_OSC_PLL")]

TOOLS = [("pk3", "PICkit 3", "PK3"), ("pk4", "PICkit 4", "PK4"), ("pk5", "PICkit 5", "PK5"), ("snap", "MPLAB SNAP", "SNAP"),
         ("icd3", "ICD 3", "ICD3"), ("icd4", "ICD 4", "ICD4"), ("pkob", "PKOB (programador de la placa)", "PKOB"),
         ("pkob4", "PKOB4 (programador de la placa)", "PKOB4"),
         ("curiosity", "Unidad CURIOSITY (Curiosity Nano, arrastrar y soltar)", "CURIOSITY")]
POWER = [("no", "No (placa con su propia alimentación)", ""), ("v5", "5.0 V", "5.0"), ("v33", "3.3 V", "3.3")]

XC8 = dict(tc="xc8", xc="{picbuild.xc8}", cflags="-O2 -std=c99", ldflags="")
def xc16(mcu):
    return dict(tc="xc16", xc="{picbuild.xc16}",
                cflags="-mcpu=%s -omf=elf -O1 -std=gnu99 -msmart-io=1 -ffunction-sections" % mcu,
                ldflags="-Wl,--script=p%s.gld,--stack=16,--data-init,--pack-data,--handles,--isr,--gc-sections,--undefined=_main" % mcu)
def xc32(mcu):
    return dict(tc="xc32", xc="{picbuild.xc32}",
                cflags="-mprocessor=%s -O1 -std=gnu99 -ffunction-sections -fdata-sections" % mcu,
                ldflags="-Wl,--gc-sections,--defsym=_min_heap_size=0", dfp="PIC32MM-GPM-0XX_DFP")

# (id, nombre, mcu, familia, relojes, flash, ram, programador por defecto)
B = [
  ("pic16f84",    "PIC16F84",    "16F84",    XC8, CLK_84,   1024,  68,   "pk3"),
  ("pic16f84a",   "PIC16F84A",   "16F84A",   XC8, CLK_84A,  1024,  68,   "pk3"),
  ("pic16f88",    "PIC16F88",    "16F88",    XC8, CLK_88,   4096,  368,  "pk3"),
  ("pic16f690",   "PIC16F690",   "16F690",   XC8, CLK_690,  4096,  256,  "pk3"),
  ("pic16f870",   "PIC16F870",   "16F870",   XC8, CLK_87X,  2048,  128,  "pk3"),
  ("pic16f872",   "PIC16F872",   "16F872",   XC8, CLK_87X,  2048,  128,  "pk3"),
  ("pic16f874",   "PIC16F874",   "16F874",   XC8, CLK_87X,  4096,  192,  "pk3"),
  ("pic16f874a",  "PIC16F874A",  "16F874A",  XC8, CLK_87X,  4096,  192,  "pk3"),
  ("pic16f877",   "PIC16F877",   "16F877",   XC8, CLK_87X,  8192,  368,  "pk3"),
  ("pic16f877a",  "PIC16F877A",  "16F877A",  XC8, CLK_87X,  8192,  368,  "pk3"),
  ("pic16f1947",  "PIC16F1947",  "16F1947",  XC8, CLK_1947, 16384, 1024, "pk3"),
  ("pic16f15385", "PIC16F15385", "16F15385", XC8, CLK_NEW,  16384, 2048, "pk3"),
  ("pic16f18877", "PIC16F18877", "16F18877", XC8, CLK_NEW,  32768, 4096, "pk3"),
  ("pic18f2420",  "PIC18F2420",  "18F2420",  XC8, CLK_18,   16384, 768,  "pk3"),
  ("pic18f2520",  "PIC18F2520",  "18F2520",  XC8, CLK_18,   32768, 1536, "pk3"),
  ("pic18f2455",  "PIC18F2455",  "18F2455",  XC8, CLK_USB,  24576, 2048, "pk3"),
  ("pic18f2550",  "PIC18F2550",  "18F2550",  XC8, CLK_USB,  32768, 2048, "pk3"),
  ("pic18f57q84", "PIC18F57Q84 (Curiosity Nano)", "18F57Q84", XC8, CLK_Q84, 131072, 12800, "curiosity"),
  ("pic24fj16ga002",  "PIC24FJ16GA002",    "24FJ16GA002", xc16("24FJ16GA002"), CLK_24FJ, 16384, 4096, "pk3"),
  ("dspic33ep32gs502", "dsPIC33EP32GS502", "33EP32GS502", xc16("33EP32GS502"), CLK_33EP, 32768, 4096, "pk3"),
  ("dspic33ep64mc206", "dsPIC33EP64MC206", "33EP64MC206", xc16("33EP64MC206"), CLK_33EP, 65536, 8192, "pk3"),
  ("dspic33ck64mc105", "dsPIC33CK64MC105 (Curiosity Nano)", "33CK64MC105", xc16("33CK64MC105"), CLK_33CK, 65536, 8192, "curiosity"),
  ("pic32mm0256gpm064", "PIC32MM0256GPM064 (PIC32MM USB Curiosity)", "32MM0256GPM064", xc32("32MM0256GPM064"), CLK_32MM, 262144, 32768, "pk3"),
]

def main():
    o = ["# Generado por tools/gen_boards.py. No editar a mano.", "",
         "menu.clock=Reloj", "menu.tool=Programador", "menu.power=Alimentar desde el programador", ""]
    for bid, name, mcu, fam, clocks, flash, ram, deftool in B:
        p = lambda k, v: o.append("%s.%s=%s" % (bid, k, v))
        p("name", name)
        p("build.core", "pic")
        p("build.board", "PIC_" + mcu)
        p("build.mcu", mcu)
        p("build.tc", fam["tc"])
        p("build.xc", fam["xc"])
        p("build.cflags", fam["cflags"])
        p("build.ldflags", fam["ldflags"])
        if "dfp" in fam:
            p("build.dfp", fam["dfp"])
        p("upload.tool", "picbuild")
        p("upload.tool.default", "picbuild")
        p("upload.protocol", "icsp")
        p("upload.maximum_size", flash)
        p("upload.maximum_data_size", ram)
        for key, label, f, flags in clocks:
            p("menu.clock.%s" % key, label)
            p("menu.clock.%s.build.f_cpu" % key, "%dUL" % f)
            p("menu.clock.%s.build.clock_flags" % key, flags)
        tools = sorted(TOOLS, key=lambda t: t[0] != deftool)   # el de la placa primero (por defecto)
        for key, label, code in tools:
            p("menu.tool.%s" % key, label)
            p("menu.tool.%s.upload.tool_code" % key, code)
        for key, label, v in POWER:
            p("menu.power.%s" % key, label)
            p("menu.power.%s.upload.power" % key, v)
        o.append("")
    dst = os.path.join(os.path.dirname(__file__), "..", "pic", "boards.txt")
    open(dst, "w").write("\n".join(o))
    print("boards.txt:", len(B), "placas")

if __name__ == "__main__":
    main()
