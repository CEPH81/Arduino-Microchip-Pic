/*
 * wiring32.c - PIC32MM (XC32): reloj, millis(), pines y Serial.
 *
 *  millis()  Timer1 con interrupción cada 1 ms.
 *  micros()  y delayMicroseconds(): contador del núcleo MIPS (SYSCLK / 2).
 *  Serial    UART1 por consulta en RC12 (TX) y RA6 (RX), pines fijos.
 *  Pines     número = puerto * 16 + bit.
 */
#include "Arduino.h"

#if defined(PIC_32BIT)

#include <sys/attribs.h>
#include <cp0defs.h>

typedef volatile uint32_t *reg_t;

#define NPORTS 4u
static const reg_t r_tris[NPORTS] = { &TRISA, &TRISB, &TRISC, &TRISD };
static const reg_t r_port[NPORTS] = { &PORTA, &PORTB, &PORTC, &PORTD };
static const reg_t r_lat[NPORTS]  = { &LATA,  &LATB,  &LATC,  &LATD };
static const reg_t r_cnpu[NPORTS] = { &CNPUA, &CNPUB, &CNPUC, &CNPUD };

static volatile unsigned long t1_millis;

void __ISR(_TIMER_1_VECTOR, IPL2SOFT) pic_t1_isr(void)
{
  IFS0CLR = _IFS0_T1IF_MASK;
  t1_millis++;
}

void interrupts(void)   { __builtin_enable_interrupts(); }
void noInterrupts(void) { __builtin_disable_interrupts(); }

unsigned long millis(void)
{
  unsigned long m;
  do { m = t1_millis; } while (m != t1_millis);
  return m;
}

unsigned long micros(void)
{
  return (unsigned long)(_CP0_GET_COUNT() / (F_CPU / 2000000UL));
}

void delay(unsigned long ms)
{
  unsigned long ini = millis();
  while (millis() - ini < ms) { }
}

void delayMicroseconds(unsigned int us)
{
  uint32_t ini = _CP0_GET_COUNT();
  uint32_t n = (uint32_t)us * (F_CPU / 2000000UL);
  while ((uint32_t)(_CP0_GET_COUNT() - ini) < n) { }
}

static void clock_init(void)
{
#if defined(PIC_OSC_PLL)
  /* FRC 8 MHz x3 = 24 MHz */
  SYSKEY = 0;
  SYSKEY = 0xAA996655;
  SYSKEY = 0x556699AA;
  SPLLCON = (1UL << 7) | (1UL << 16);      /* PLLICLK = FRC, x3, /1 */
  OSCCONbits.NOSC = 1;                     /* PLL del sistema */
  OSCCONbits.OSWEN = 1;
  while (OSCCONbits.OSWEN) { }
  SYSKEY = 0x33333333;
#endif
}

void init(void)
{
  clock_init();
  ANSELA = 0; ANSELB = 0; ANSELC = 0;
#if defined(ANSELD)
  ANSELD = 0;
#endif

  T1CON = 0;
  TMR1 = 0;
  PR1 = F_CPU / 8UL / 1000UL - 1UL;
  IPC4bits.T1IP = 2;
  IFS0CLR = _IFS0_T1IF_MASK;
  IEC0SET = _IEC0_T1IE_MASK;
  T1CON = 0x8010;                          /* encendido, 1:8 */

  INTCONSET = _INTCON_MVEC_MASK;
  __builtin_enable_interrupts();
}

void pinMode(uint8_t pin, uint8_t mode)
{
  uint8_t port = pin >> 4;
  uint32_t m = 1UL << (pin & 15u);
  if (port >= NPORTS) return;
  if (mode == OUTPUT) {
    r_tris[port][1] = m;                   /* TRISxCLR */
  } else {
    r_tris[port][2] = m;                   /* TRISxSET */
    r_cnpu[port][mode == INPUT_PULLUP ? 2 : 1] = m;
  }
}

void digitalWrite(uint8_t pin, uint8_t val)
{
  uint8_t port = pin >> 4;
  if (port >= NPORTS) return;
  r_lat[port][val ? 2 : 1] = 1UL << (pin & 15u);   /* LATxSET / LATxCLR */
}

uint8_t digitalRead(uint8_t pin)
{
  uint8_t port = pin >> 4;
  if (port >= NPORTS) return LOW;
  return (*r_port[port] & (1UL << (pin & 15u))) ? HIGH : LOW;
}

void digitalToggle(uint8_t pin)
{
  uint8_t port = pin >> 4;
  if (port >= NPORTS) return;
  r_lat[port][3] = 1UL << (pin & 15u);             /* LATxINV */
}

/* ----------------------------------------------------------------- Serial */
void Serial_begin(unsigned long baud)
{
  TRISCCLR = 1UL << 12;                    /* RC12 = U1TX */
  TRISASET = 1UL << 6;                     /* RA6 = U1RX */
  U1MODE = 0;
  U1BRG = (F_CPU / 4UL + baud / 2UL) / baud - 1UL;
  U1STA = 0;
  U1MODE = 0x8008;                         /* ON, BRGH */
  U1STASET = _U1STA_UTXEN_MASK | _U1STA_URXEN_MASK;
}

void Serial_end(void) { U1MODE = 0; }

void Serial_write(uint8_t c) { while (U1STAbits.UTXBF) { } U1TXREG = c; }
void Serial_flush(void)      { while (!U1STAbits.TRMT) { } }

uint8_t Serial_available(void)
{
  if (U1STAbits.OERR) U1STACLR = _U1STA_OERR_MASK;
  return U1STAbits.URXDA ? 1u : 0u;
}

int Serial_read(void)
{
  if (!Serial_available()) return -1;
  return (int)(U1RXREG & 0xFF);
}

int Serial_peek(void) { return -1; }

#endif /* PIC_32BIT */
