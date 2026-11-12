/*
 * wiring_digital8.c - pinMode / digitalWrite / digitalRead para XC8.
 *
 * Número de pin = puerto * 8 + bit (PIN_RA0 = 0, PIN_RB0 = 8...). Los
 * registros se buscan en tablas constantes (quedan en la flash).
 * En los PIC sin registros LAT (PIC16F84/88/690/87x) las salidas se
 * escriben en PORT.
 */
#include "Arduino.h"

#if defined(PIC_8BIT)

typedef volatile unsigned char *reg_t;

#if defined(TRISJ)
  #error "Puerto J no soportado todavía"
#endif

/* Tablas de registros por puerto (0 = el chip no lo tiene) */
#if defined(TRISA)
  #define R_TRISA &TRISA
#else
  #define R_TRISA 0
#endif
#if defined(TRISB)
  #define R_TRISB &TRISB
#else
  #define R_TRISB 0
#endif
#if defined(TRISC)
  #define R_TRISC &TRISC
#else
  #define R_TRISC 0
#endif
#if defined(TRISD)
  #define R_TRISD &TRISD
#else
  #define R_TRISD 0
#endif
#if defined(TRISE)
  #define R_TRISE &TRISE
#else
  #define R_TRISE 0
#endif
#if defined(TRISF)
  #define R_TRISF &TRISF
#else
  #define R_TRISF 0
#endif
#if defined(TRISG)
  #define R_TRISG &TRISG
#else
  #define R_TRISG 0
#endif
#if defined(PORTA)
  #define R_PORTA &PORTA
#else
  #define R_PORTA 0
#endif
#if defined(PORTB)
  #define R_PORTB &PORTB
#else
  #define R_PORTB 0
#endif
#if defined(PORTC)
  #define R_PORTC &PORTC
#else
  #define R_PORTC 0
#endif
#if defined(PORTD)
  #define R_PORTD &PORTD
#else
  #define R_PORTD 0
#endif
#if defined(PORTE)
  #define R_PORTE &PORTE
#else
  #define R_PORTE 0
#endif
#if defined(PORTF)
  #define R_PORTF &PORTF
#else
  #define R_PORTF 0
#endif
#if defined(PORTG)
  #define R_PORTG &PORTG
#else
  #define R_PORTG 0
#endif
#if defined(LATA)
  #define R_LATA &LATA
#else
  #define R_LATA 0
#endif
#if defined(LATB)
  #define R_LATB &LATB
#else
  #define R_LATB 0
#endif
#if defined(LATC)
  #define R_LATC &LATC
#else
  #define R_LATC 0
#endif
#if defined(LATD)
  #define R_LATD &LATD
#else
  #define R_LATD 0
#endif
#if defined(LATE)
  #define R_LATE &LATE
#else
  #define R_LATE 0
#endif
#if defined(LATF)
  #define R_LATF &LATF
#else
  #define R_LATF 0
#endif
#if defined(LATG)
  #define R_LATG &LATG
#else
  #define R_LATG 0
#endif
#define PORTS(x) { R_##x##A, R_##x##B, R_##x##C, R_##x##D, R_##x##E, R_##x##F, R_##x##G }
#define NPORTS 7u

static const reg_t r_tris[NPORTS] = PORTS(TRIS);
static const reg_t r_port[NPORTS] = PORTS(PORT);
#if defined(PIC_HAS_LAT)
static const reg_t r_lat[NPORTS]  = PORTS(LAT);
#else
  #define r_lat r_port
#endif

static const uint8_t bitmask[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };

/* Pull-ups: en los PIC nuevos hay un registro WPU por puerto; en los
 * clásicos solo el PORTB entero (bit nRBPU). */
static void pullup(uint8_t port, uint8_t mask, uint8_t on)
{
#if defined(WPUA) && defined(WPUB) && defined(WPUC)
  static const reg_t r_wpu[NPORTS] = {
    &WPUA, &WPUB, &WPUC,
  #if defined(WPUD)
    &WPUD,
  #else
    0,
  #endif
  #if defined(WPUE)
    &WPUE,
  #else
    0,
  #endif
  #if defined(WPUF)
    &WPUF,
  #else
    0,
  #endif
    0 };
  reg_t w = r_wpu[port];
  if (w) { if (on) *w |= mask; else *w &= (uint8_t)~mask; }
#elif defined(PIC_K_PIC18)
  if (port == 1u && on) INTCON2bits.nRBPU = 0;        /* pull-ups de PORTB (todas) */
#elif defined(WPUB)
  if (port == 1u) { if (on) { WPUB |= mask; OPTION_REG &= 0x7Fu; } else WPUB &= (uint8_t)~mask; }
#else
  if (port == 1u && on) OPTION_REG &= 0x7Fu;         /* nRBPU = 0: pull-ups de PORTB (todas) */
#endif
  (void)port; (void)mask; (void)on;
}

void pinMode(uint8_t pin, uint8_t mode)
{
  uint8_t port = pin >> 3, m = bitmask[pin & 7u];
  reg_t t;
  if (port >= NPORTS) return;
  t = r_tris[port];
  if (!t) return;
  if (mode == OUTPUT) {
    *t &= (uint8_t)~m;
  } else {
    *t |= m;
    pullup(port, m, mode == INPUT_PULLUP);
  }
}

void digitalWrite(uint8_t pin, uint8_t val)
{
  uint8_t port = pin >> 3, m = bitmask[pin & 7u];
  reg_t l;
  if (port >= NPORTS) return;
  l = r_lat[port];
  if (!l) return;
  if (val) *l |= m; else *l &= (uint8_t)~m;
}

uint8_t digitalRead(uint8_t pin)
{
  uint8_t port = pin >> 3;
  reg_t p;
  if (port >= NPORTS) return LOW;
  p = r_port[port];
  if (!p) return LOW;
  return (*p & bitmask[pin & 7u]) ? HIGH : LOW;
}

void digitalToggle(uint8_t pin)
{
  uint8_t port = pin >> 3;
  reg_t l;
  if (port >= NPORTS) return;
  l = r_lat[port];
  if (l) *l ^= bitmask[pin & 7u];
}

#endif /* PIC_8BIT */
