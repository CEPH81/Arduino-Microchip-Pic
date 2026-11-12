/*
 * wiring8.c - Arranque, reloj y tiempo para los PIC de 8 bits (XC8).
 *
 * millis() usa Timer0 en 8 bits con interrupción por desborde. El
 * preescalador se elige según F_CPU para que desborde cada ~1 ms:
 *
 *   F_CPU      divisor   desborde
 *   <= 4 MHz      4      1024 us (a 4 MHz)
 *   <= 8 MHz      8      1024 us
 *   <= 20 MHz    16      1024 us (16 MHz), 819 us (20 MHz)
 *   <= 40 MHz    32      1024 us (32 MHz)
 *   >  40 MHz    64      1365 us (48 MHz), 1024 us (64 MHz)
 *
 * Como en el Arduino UNO, la parte que no llega al milisegundo se acumula
 * (fracción en microsegundos), así millis() no se atrasa.
 */
#include "Arduino.h"

#if defined(PIC_8BIT)

#if F_CPU <= 4000000L
  #define T0_DIV   4UL
  #define T0_PS    1u          /* bits PS de OPTION_REG / T0CON */
#elif F_CPU <= 8000000L
  #define T0_DIV   8UL
  #define T0_PS    2u
#elif F_CPU <= 20000000L
  #define T0_DIV   16UL
  #define T0_PS    3u
#elif F_CPU <= 40000000L
  #define T0_DIV   32UL
  #define T0_PS    4u
#else
  #define T0_DIV   64UL
  #define T0_PS    5u
#endif

/* microsegundos por desborde (256 cuentas a Fosc/4/T0_DIV) */
#define T0_US        ((256UL * T0_DIV * 4UL * 1000UL) / (F_CPU / 1000UL))
#define T0_MS_INC    ((uint8_t)(T0_US / 1000UL))
#define T0_FRAC_INC  ((uint16_t)(T0_US % 1000UL))

/* Bandera y habilitación de la interrupción de Timer0, y GIE */
#if defined(PIC_K_PIC18Q)
  #define T0_IF     PIR3bits.TMR0IF
  #define T0_IE     PIE3bits.TMR0IE
  #define PIC_GIE   INTCON0bits.GIE
#elif defined(PIC_K_ENHNEW)
  #define T0_IF     PIR0bits.TMR0IF
  #define T0_IE     PIE0bits.TMR0IE
  #define PIC_GIE   INTCONbits.GIE
#elif defined(_16F88)
  #define T0_IF     INTCONbits.TMR0IF
  #define T0_IE     INTCONbits.TMR0IE
  #define PIC_GIE   INTCONbits.GIE
#else
  #define T0_IF     INTCONbits.T0IF
  #define T0_IE     INTCONbits.T0IE
  #define PIC_GIE   INTCONbits.GIE
#endif

#if defined(TMR0L)
  #define T0_COUNT  TMR0L
#else
  #define T0_COUNT  TMR0
#endif

static volatile unsigned long t0_millis;
static volatile uint16_t      t0_frac;        /* 0..999 us */

void __interrupt() pic_isr(void)
{
  if (T0_IE && T0_IF) {
    T0_IF = 0;
    t0_millis += T0_MS_INC;
    t0_frac += T0_FRAC_INC;
    if (t0_frac >= 1000u) {
      t0_frac -= 1000u;
      t0_millis++;
    }
  }
}

void interrupts(void)   { PIC_GIE = 1; }
void noInterrupts(void) { PIC_GIE = 0; }

unsigned long millis(void)
{
  unsigned long m;
  uint8_t g = PIC_GIE;
  PIC_GIE = 0;
  m = t0_millis;
  if (g) PIC_GIE = 1;
  return m;
}

unsigned long micros(void)
{
  unsigned long m;
  uint16_t f;
  uint8_t t, g = PIC_GIE;
  PIC_GIE = 0;
  m = t0_millis;
  f = t0_frac;
  t = T0_COUNT;
  if (T0_IF && t < 128u) {            /* desbordó y la interrupción aún no corrió */
    m += T0_MS_INC;
    f += T0_FRAC_INC;
  }
  if (g) PIC_GIE = 1;
  return m * 1000UL + f + (((unsigned long)t * T0_US) >> 8);
}

void delay(unsigned long ms)
{
  unsigned long ini = millis();
  while (millis() - ini < ms) { }
}

/*
 * Espera aproximada: avanza en tramos de 10 us (20 us por debajo de
 * 8 MHz) y descuenta lo que tarda el propio bucle. Para tiempos exactos
 * con una constante use __delay_us(n) de XC8.
 */
#if F_CPU < 8000000L
  #define DUS_STEP  20u
#else
  #define DUS_STEP  10u
#endif
#define DUS_CYCLES  ((DUS_STEP * (F_CPU / 4000000UL)) - 12UL)

void delayMicroseconds(unsigned int us)
{
  while (us >= DUS_STEP) {
    us -= DUS_STEP;
    _delay(DUS_CYCLES);
  }
}

/* --------------------------------------------------------------- reloj */
static void clock_init(void)
{
#if defined(PIC_K_ENHNEW) || defined(PIC_K_PIC18Q)
  /* HFINTOSC con divisor: PIC_OSC_DIV (0 = /1, 1 = /2, 2 = /4, 3 = /8) */
  #if defined(PIC_OSC_INT)
  OSCCON1 = 0x60u | (PIC_OSC_DIV);        /* NOSC = HFINTOSC, NDIV */
  #endif
#elif defined(PIC_OSC_INT)
  #if defined(_16F88) || defined(_16F690)
  OSCCON = (uint8_t)(PIC_OSC_IRCF << 4);               /* IRCF; la fuente la fija FOSC (interno) */
  #elif defined(PIC_K_ENH)
  OSCCON = (uint8_t)(PIC_OSC_IRCF << 3);               /* 16F1947: IRCF<3:0>, SCS = 00 (según FOSC) */
    #if defined(PIC_OSC_PLL)
  OSCCONbits.SPLLEN = 1;
    #endif
  #elif defined(PIC_K_PIC18)
  OSCCON = (uint8_t)((OSCCON & 0x8Fu) | (PIC_OSC_IRCF << 4));
    #if defined(PIC_OSC_PLL) && defined(_OSCTUNE_PLLEN_POSN)
  OSCTUNEbits.PLLEN = 1;
    #endif
  #endif
#endif
}

/* Todos los pines como digitales (al arrancar, los analógicos mandan) */
static void analog_off(void)
{
#if defined(ANSELA)
  ANSELA = 0;
#endif
#if defined(ANSELB)
  ANSELB = 0;
#endif
#if defined(ANSELC)
  ANSELC = 0;
#endif
#if defined(ANSELD)
  ANSELD = 0;
#endif
#if defined(ANSELE)
  ANSELE = 0;
#endif
#if defined(ANSELF)
  ANSELF = 0;
#endif
#if defined(ANSELG)
  ANSELG = 0;
#endif
#if defined(ANSEL)
  ANSEL = 0;
#endif
#if defined(ANSELH)
  ANSELH = 0;
#endif
#if defined(PIC_K_PIC18)
  ADCON1 = 0x0F;                 /* AN0..AN12 digitales */
#elif defined(PIC_K_CLASSIC) && defined(ADCON1) && !defined(ANSEL)
  ADCON1 = 0x06;                 /* PIC16F87x: PORTA y PORTE digitales */
#endif
#if defined(CMCON)
  CMCON = 0x07;                  /* comparadores apagados */
#endif
}

void init(void)
{
  clock_init();
  analog_off();

  /* Timer0: reloj interno (Fosc/4), 8 bits, preescalador T0_PS */
#if defined(PIC_K_ENHNEW) || defined(PIC_K_PIC18Q)
  T0CON1 = (uint8_t)(0x40u | (T0_PS + 1u));      /* Fosc/4, síncrono, 1:2^(PS+1) */
  TMR0H = 0xFF;                                   /* periodo de 256 cuentas */
  TMR0L = 0;
  T0CON0 = 0x80;                                  /* encendido, 8 bits */
#elif defined(PIC_K_PIC18)
  T0CON = (uint8_t)(0xC0u | T0_PS);               /* encendido, 8 bits, Fosc/4, con preescalador */
  TMR0L = 0;
#else
  OPTION_REG = (uint8_t)((OPTION_REG & 0xC0u) | T0_PS);   /* T0CS = 0, PSA = 0 */
  TMR0 = 0;
#endif
  T0_IF = 0;
  T0_IE = 1;
#if defined(_INTCON_PEIE_POSN)
  INTCONbits.PEIE = 1;
#endif
  PIC_GIE = 1;
}

#endif /* PIC_8BIT */
