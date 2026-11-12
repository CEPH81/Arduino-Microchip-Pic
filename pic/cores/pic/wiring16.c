/*
 * wiring16.c - PIC24 / dsPIC33 (XC16, XC-DSC): reloj, millis(), pines y Serial.
 *
 *  F_CPU es FCY (frecuencia de instrucciones = reloj de los periféricos).
 *  millis()  Timer1 con interrupción cada 1 ms (preescalador 1:8).
 *  Serial    UART1 por consulta, en PIN_SERIAL_TX / PIN_SERIAL_RX (PPS).
 *  Pines     número = puerto * 16 + bit (PIN_RB15 = 31).
 */
#include "Arduino.h"

#if defined(PIC_16BIT)

#define FCY F_CPU
#include <libpic30.h>

typedef volatile uint16_t *reg_t;

#define NPORTS 7u
static const reg_t r_tris[NPORTS] = {
#if defined(TRISA)
  &TRISA,
#else
  0,
#endif
  &TRISB,
#if defined(TRISC)
  &TRISC,
#else
  0,
#endif
#if defined(TRISD)
  &TRISD,
#else
  0,
#endif
#if defined(TRISE)
  &TRISE,
#else
  0,
#endif
#if defined(TRISF)
  &TRISF,
#else
  0,
#endif
#if defined(TRISG)
  &TRISG,
#else
  0,
#endif
};
/* PORT, LAT y CNPU están a distancia fija de TRIS en estas familias:
 * TRISx, PORTx, LATx (+2, +4). Se calcula en lugar de repetir las tablas. */
#define R_PORT(t)  ((reg_t)((volatile uint8_t *)(t) + 2))
#define R_LAT(t)   ((reg_t)((volatile uint8_t *)(t) + 4))

/* ----------------------------------------------------------------- tiempo */
static volatile unsigned long t1_millis;

void __attribute__((interrupt, no_auto_psv)) _T1Interrupt(void)
{
  IFS0bits.T1IF = 0;
  t1_millis++;
}

void interrupts(void)   { SRbits.IPL = 0; }
void noInterrupts(void) { SRbits.IPL = 7; }

unsigned long millis(void)
{
  unsigned long m;
  IEC0bits.T1IE = 0;
  m = t1_millis;
  IEC0bits.T1IE = 1;
  return m;
}

unsigned long micros(void)
{
  unsigned long m;
  uint16_t t;
  IEC0bits.T1IE = 0;
  m = t1_millis;
  t = TMR1;
  if (IFS0bits.T1IF && t < (PR1 >> 1)) m++;      /* venció y la interrupción aún no corrió */
  IEC0bits.T1IE = 1;
  return m * 1000UL + ((unsigned long)t * 8000UL) / (F_CPU / 1000UL);
}

void delay(unsigned long ms)
{
  unsigned long ini = millis();
  while (millis() - ini < ms) { }
}

void delayMicroseconds(unsigned int us)
{
  unsigned long c = ((unsigned long)us * (F_CPU / 1000UL)) / 1000UL;
  if (c > 16UL) __delay32(c - 12UL);
}

/* ------------------------------------------------------------------ reloj */
static void clock_init(void)
{
#if defined(__PIC24F__)
  CLKDIVbits.RCDIV = 0;                    /* FRC sin dividir: 8 MHz */
#elif defined(PIC_OSC_PLL) && defined(PIC_K_33CK)
  /* FRC 8 MHz -> VCO 800 MHz -> /2 = 400 MHz -> FOSC 200 MHz, FCY 100 MHz */
  CLKDIVbits.PLLPRE = 1;
  PLLFBDbits.PLLFBDIV = 100;
  PLLDIVbits.POST1DIV = 2;
  PLLDIVbits.POST2DIV = 1;
  __builtin_write_OSCCONH(0x01);           /* NOSC = FRC con PLL */
  __builtin_write_OSCCONL(OSCCON | 0x01);  /* OSWEN */
  while (OSCCONbits.OSWEN) { }
  while (!OSCCONbits.LOCK) { }
#elif defined(PIC_OSC_PLL)
  /* dsPIC33EP: FRC 7.37 MHz x76 / (2 x 2) = 140 MHz, FCY = 70 MHz */
  PLLFBD = 74;                             /* M = 76 */
  CLKDIVbits.PLLPRE = 0;                   /* N1 = 2 */
  CLKDIVbits.PLLPOST = 0;                  /* N2 = 2 */
  __builtin_write_OSCCONH(0x01);
  __builtin_write_OSCCONL(OSCCON | 0x01);
  while (OSCCONbits.OSWEN) { }
  while (!OSCCONbits.LOCK) { }
#endif
}

void init(void)
{
  clock_init();

  /* todos los pines digitales */
#if defined(AD1PCFG)
  AD1PCFG = 0xFFFF;
#endif
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
#if defined(ANSELG)
  ANSELG = 0;
#endif

  /* Timer1: FCY / 8, 1 ms */
  T1CON = 0;
  TMR1 = 0;
  PR1 = (uint16_t)(F_CPU / 8UL / 1000UL - 1UL);
  IPC0bits.T1IP = 4;
  IFS0bits.T1IF = 0;
  IEC0bits.T1IE = 1;
  T1CON = 0x8010;                          /* encendido, 1:8 */
}

/* ------------------------------------------------------------------ pines */
#if defined(__PIC24F__)
/* PIC24FJxxGA002: las pull-ups van por número de CN (0xFF = no tiene) */
static const uint8_t cn_a[5]  = { 2, 3, 30, 29, 0 };
static const uint8_t cn_b[16] = { 4, 5, 6, 7, 1, 27, 24, 23, 22, 21, 16, 15, 14, 13, 12, 11 };
#endif

static void pullup(uint8_t port, uint8_t b, uint8_t on)
{
#if defined(__PIC24F__)
  uint8_t cn = 0xFF;
  if (port == 0u && b < 5u) cn = cn_a[b];
  else if (port == 1u) cn = cn_b[b];
  if (cn == 0xFF) return;
  if (cn < 16u) { if (on) CNPU1 |= (1u << cn); else CNPU1 &= ~(1u << cn); }
  else          { if (on) CNPU2 |= (1u << (cn - 16u)); else CNPU2 &= ~(1u << (cn - 16u)); }
#else
  static const reg_t r_cnpu[NPORTS] = {
  #if defined(CNPUA)
    &CNPUA,
  #else
    0,
  #endif
    &CNPUB,
  #if defined(CNPUC)
    &CNPUC,
  #else
    0,
  #endif
  #if defined(CNPUD)
    &CNPUD,
  #else
    0,
  #endif
  #if defined(CNPUE)
    &CNPUE,
  #else
    0,
  #endif
  #if defined(CNPUF)
    &CNPUF,
  #else
    0,
  #endif
  #if defined(CNPUG)
    &CNPUG,
  #else
    0,
  #endif
  };
  reg_t r = r_cnpu[port];
  if (!r) return;
  if (on) *r |= (uint16_t)(1u << b); else *r &= (uint16_t)~(1u << b);
#endif
}

void pinMode(uint8_t pin, uint8_t mode)
{
  uint8_t port = pin >> 4, b = pin & 15u;
  reg_t t;
  if (port >= NPORTS) return;
  t = r_tris[port];
  if (!t) return;
  if (mode == OUTPUT) {
    *t &= (uint16_t)~(1u << b);
  } else {
    *t |= (uint16_t)(1u << b);
    pullup(port, b, mode == INPUT_PULLUP);
  }
}

void digitalWrite(uint8_t pin, uint8_t val)
{
  uint8_t port = pin >> 4;
  uint16_t m = (uint16_t)(1u << (pin & 15u));
  reg_t t;
  if (port >= NPORTS) return;
  t = r_tris[port];
  if (!t) return;
  if (val) *R_LAT(t) |= m; else *R_LAT(t) &= (uint16_t)~m;
}

uint8_t digitalRead(uint8_t pin)
{
  uint8_t port = pin >> 4;
  reg_t t;
  if (port >= NPORTS) return LOW;
  t = r_tris[port];
  if (!t) return LOW;
  return (*R_PORT(t) & (1u << (pin & 15u))) ? HIGH : LOW;
}

void digitalToggle(uint8_t pin)
{
  uint8_t port = pin >> 4;
  reg_t t;
  if (port >= NPORTS) return;
  t = r_tris[port];
  if (t) *R_LAT(t) ^= (uint16_t)(1u << (pin & 15u));
}

/* ----------------------------------------------------------------- Serial */
static void serial_pins(void)
{
#if defined(PIC_K_33CK)
  __builtin_write_RPCON(0x0000);           /* desbloquear PPS */
  _RP58R = 1;                              /* RC10 = U1TX */
  _U1RXR = 59;                             /* U1RX = RC11 */
  __builtin_write_RPCON(0x0800);
#else
  __builtin_write_OSCCONL(OSCCON & ~0x40); /* IOLOCK = 0 */
  #if defined(__PIC24F__)
  _RP7R = 3;                               /* RB7 = U1TX */
  _U1RXR = 8;                              /* U1RX = RB8 */
  #elif defined(__dsPIC33EP32GS502__)
  _RP43R = 1;                              /* RB11 = U1TX */
  _U1RXR = 44;                             /* U1RX = RB12 */
  #else
  _RP54R = 1;                              /* RC6 = U1TX */
  _U1RXR = 55;                             /* U1RX = RC7 */
  #endif
  __builtin_write_OSCCONL(OSCCON | 0x40);
#endif
}

void Serial_begin(unsigned long baud)
{
  serial_pins();
  U1BRG = (uint16_t)((F_CPU / 4UL + baud / 2UL) / baud - 1UL);
#if defined(PIC_K_33CK)
  U1MODE = 0;
  U1MODEH = 0;
  U1MODEbits.BRGH = 1;
  U1MODEbits.UARTEN = 1;
  U1MODEbits.UTXEN = 1;
  U1MODEbits.URXEN = 1;
#else
  U1MODE = 0x8008;                         /* UARTEN, BRGH */
  U1STA = 0x0400;                          /* UTXEN */
#endif
}

void Serial_end(void) { U1MODE = 0; }

#if defined(PIC_K_33CK)
void Serial_write(uint8_t c) { while (U1STAHbits.UTXBF) { } U1TXREG = c; }
void Serial_flush(void)      { while (!U1STAbits.TRMT) { } }
uint8_t Serial_available(void)
{
  if (U1STAbits.OERR) U1STAbits.OERR = 0;
  return U1STAHbits.URXBE ? 0u : 1u;
}
#else
void Serial_write(uint8_t c) { while (U1STAbits.UTXBF) { } U1TXREG = c; }
void Serial_flush(void)      { while (!U1STAbits.TRMT) { } }
uint8_t Serial_available(void)
{
  if (U1STAbits.OERR) U1STAbits.OERR = 0;
  return U1STAbits.URXDA ? 1u : 0u;
}
#endif

int Serial_read(void)
{
  if (!Serial_available()) return -1;
  return (int)(U1RXREG & 0xFF);
}

int Serial_peek(void) { return -1; }

#endif /* PIC_16BIT */
