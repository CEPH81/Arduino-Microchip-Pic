#!/bin/bash
# Sonda de desarrollo: prueba qué construcciones de C acepta XC8 y cuánto ocupan.
O=$PWD/salida; mkdir -p $O; cd $O
X8=/opt/microchip/xc8/v2.50/bin/xc8-cc
cat > base.h <<'XX'
#include <xc.h>
#include <stdint.h>
#pragma config FOSC = INTOSCIO, WDTE = OFF, LVP = OFF
void wr(uint8_t c) { while (!PIR1bits.TXIF) { } TXREG = c; }
void p_s(const char *s) { while (*s) wr((uint8_t)*s++); }
void p_c(char c) { wr((uint8_t)c); }
void p_u(unsigned long v, uint8_t base) { char b[33]; uint8_t i = 33; do { uint8_t d = (uint8_t)(v % base); v /= base; b[--i] = (char)(d < 10 ? '0' + d : 'A' + d - 10); } while (v); while (i < 33) wr((uint8_t)b[i++]); }
void p_i(long v, uint8_t base) { if (base == 10 && v < 0) { wr('-'); v = -v; } p_u((unsigned long)v, base); }
void p_f(double v, uint8_t dig) { unsigned long e = (unsigned long)v; p_u(e, 10); wr('.'); v -= (double)e; while (dig--) { v *= 10.0; uint8_t d = (uint8_t)v; wr((uint8_t)('0' + d)); v -= d; } }
void p_u10(unsigned long v) { p_u(v, 10); }
void p_i10(long v) { p_i(v, 10); }
void p_f2(double v) { p_f(v, 2); }
XX
prueba() {  # nombre, cuerpo de main
  { echo '#include "base.h"'; cat; } > $1.c
  if $X8 -mcpu=16F88 -O2 -std=c99 -o $1.elf $1.c > $1.log 2>&1; then
    echo "OK    $1: $(grep -o 'Program space *used *[0-9A-Fa-f]*h *( *[0-9]*)' $1.log | grep -o '( *[0-9]*)')"
  else
    echo "FALLA $1: $(grep -m2 -i 'error\|what()' $1.log | tr '\n' ' ' | cut -c1-200)"
  fi
}
prueba directo <<'XX'
unsigned int n; void main(void) { p_s("hola "); p_u10(n); for (;;) { } }
XX
prueba directo_float <<'XX'
unsigned int n; float f; void main(void) { p_s("hola "); p_u10(n); p_f2(f); for (;;) { } }
XX
prueba generic <<'XX'
#define P(x) _Generic((x), char *: p_s, const char *: p_s, char: p_c, float: p_f2, double: p_f2, unsigned char: p_u10, unsigned int: p_u10, unsigned long: p_u10, default: p_i10)(x)
unsigned int n; void main(void) { P("hola "); P(n); for (;;) { } }
XX
prueba generic_decay <<'XX'
#define P(x) _Generic((x), char: p_c, default: _Generic(((x) + 0), char *: p_s, const char *: p_s, float: p_f2, double: p_f2, unsigned int: p_u10, unsigned long: p_u10, default: p_i10))(x)
unsigned int n; char c; uint8_t b; void main(void) { P("hola "); P(n); P(c); P(b); P(-5); for (;;) { } }
XX
prueba generic_base <<'XX'
#define P2(x, b) _Generic(((x) + 0), float: p_f, double: p_f, unsigned int: p_u, unsigned long: p_u, default: p_i)((x), (b))
unsigned int n; void main(void) { P2(n, 16); P2(-7, 10); for (;;) { } }
XX
prueba ternario_const <<'XX'
#define C(e) (sizeof(char[1 + (e)]) - 1u)
#define ISF(x) (__builtin_types_compatible_p(__typeof__((x) + 0), float) || __builtin_types_compatible_p(__typeof__((x) + 0), double))
#define P(x) (C(ISF(x)) ? p_f2(x) : p_u10(x))
unsigned int n; void main(void) { p_s("hola "); P(n); for (;;) { } }
XX
prueba if_const <<'XX'
unsigned int n; float f; void main(void) { p_s("hola "); if (sizeof(char[1]) - 1u) p_f2(f); else p_u10(n); for (;;) { } }
XX
prueba typeof_sizeof <<'XX'
unsigned int n; void main(void) { p_u10(sizeof(__typeof__((n) + 0))); p_u10(__builtin_types_compatible_p(int, long)); for (;;) { } }
XX
cat *.log | grep -i 'error' | sort | uniq -c | head -20
ls > lista.txt
