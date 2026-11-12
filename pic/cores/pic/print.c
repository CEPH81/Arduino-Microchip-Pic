/*
 * print.c - Texto y números por Serial, sin printf (ocupa mucho menos).
 */
#include "Arduino.h"

#if defined(PIC_HAS_SERIAL)

void Serial_print_s(const char *s)
{
  while (*s) Serial_write((uint8_t)*s++);
}

void Serial_println0(void)
{
  Serial_write('\r');
  Serial_write('\n');
}

void Serial_print_u(unsigned long v, uint8_t base)
{
  char buf[33];
  uint8_t i = sizeof(buf);
  if (base < 2u) base = 10;
  do {
    uint8_t d = (uint8_t)(v % base);
    v /= base;
    buf[--i] = (char)(d < 10u ? '0' + d : 'A' + d - 10u);
  } while (v);
  while (i < sizeof(buf)) Serial_write((uint8_t)buf[i++]);
}

void Serial_print_i(long v, uint8_t base)
{
  if (base == 10u && v < 0) {
    Serial_write('-');
    Serial_print_u((unsigned long)(-v), 10);
  } else {
    Serial_print_u((unsigned long)v, base);
  }
}

void Serial_print_f(double v, uint8_t digits)
{
  double r = 0.5;
  unsigned long ent;
  uint8_t i;
  if (v < 0.0) { Serial_write('-'); v = -v; }
  for (i = 0; i < digits; i++) r /= 10.0;      /* redondeo */
  v += r;
  ent = (unsigned long)v;
  Serial_print_u(ent, 10);
  if (digits) {
    Serial_write('.');
    v -= (double)ent;
    while (digits--) {
      uint8_t d;
      v *= 10.0;
      d = (uint8_t)v;
      Serial_write((uint8_t)('0' + d));
      v -= d;
    }
  }
}

#endif

long map(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
