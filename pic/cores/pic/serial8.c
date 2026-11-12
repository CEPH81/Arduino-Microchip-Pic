/*
 * serial8.c - Serial por el USART/EUSART de los PIC de 8 bits.
 *
 *  - PIC16F88/690/87x, 16F1947, 16F15385, 16F18877, PIC18F2xxx:
 *    TXSTA / RCSTA / SPBRG (con BRG16 donde existe).
 *  - PIC18F57Q84: UART1 nuevo (U1CON0, U1BRG, U1TXB, U1RXB).
 *  - En los chips con PPS se conectan PIN_SERIAL_TX y PIN_SERIAL_RX.
 *
 * Transmisión y recepción por consulta (sin interrupciones ni búfer en
 * RAM): el hardware guarda 2 bytes recibidos; lea Serial.available() con
 * frecuencia. Si se pierde un byte (OERR) el receptor se reinicia solo.
 */
#include "Arduino.h"

#if defined(PIC_8BIT) && defined(PIC_HAS_SERIAL)

#if defined(PIC_K_PIC18Q)
/* ----------------------------------------------------- PIC18F57Q84: UART1 */
void Serial_begin(unsigned long baud)
{
  U1CON1 = 0;
  U1CON0 = 0xB0;                 /* BRGS = 1 (alta velocidad), TXEN, RXEN, 8 bits asíncrono */
  U1CON2 = 0;
  U1BRG = (uint16_t)((F_CPU / 4UL + baud / 2UL) / baud - 1UL);
  /* PPS: RF0 = U1TX (0x20), U1RX = RF1 (puerto F = 5: 0b101_001) */
  RF0PPS = 0x20;
  U1RXPPS = 0x29;
  TRISFbits.TRISF0 = 0;
  TRISFbits.TRISF1 = 1;
  U1CON1bits.ON = 1;
}

void Serial_end(void) { U1CON1bits.ON = 0; RF0PPS = 0; }

void Serial_write(uint8_t c)
{
  while (U1FIFObits.TXBF) { }
  U1TXB = c;
}

void Serial_flush(void) { while (!U1ERRIRbits.TXMTIF) { } }

uint8_t Serial_available(void)
{
  if (U1ERRIRbits.RXFOIF) U1ERRIRbits.RXFOIF = 0;      /* desborde: se limpia */
  return U1FIFObits.RXBE ? 0u : 1u;
}

int Serial_read(void)
{
  if (U1FIFObits.RXBE) return -1;
  return U1RXB;
}

int Serial_peek(void) { return -1; }

#else
/* ------------------------------------------------------ USART / EUSART */
#if defined(_16F15385)
  #define TX_IF  PIR3bits.TX1IF
  #define RC_IF  PIR3bits.RC1IF
#elif defined(PIC_K_ENHNEW)
  #define TX_IF  PIR3bits.TXIF
  #define RC_IF  PIR3bits.RCIF
#else
  #define TX_IF  PIR1bits.TXIF
  #define RC_IF  PIR1bits.RCIF
#endif

#if defined(BAUDCON)
  #define BAUD_REG BAUDCON
#elif defined(BAUDCTL)
  #define BAUD_REG BAUDCTL
#elif defined(BAUD1CON)
  #define BAUD_REG BAUD1CON
#endif

void Serial_begin(unsigned long baud)
{
#if defined(BAUD_REG) && defined(SPBRGH)
  /* generador de 16 bits: baud = Fosc / (4 * (n + 1)) */
  uint16_t n = (uint16_t)((F_CPU / 4UL + baud / 2UL) / baud - 1UL);
  BAUD_REG = 0x08;               /* BRG16 */
  SPBRGH = (uint8_t)(n >> 8);
  SPBRG = (uint8_t)n;
#else
  /* generador de 8 bits con BRGH = 1: baud = Fosc / (16 * (n + 1)) */
  SPBRG = (uint8_t)((F_CPU / 16UL + baud / 2UL) / baud - 1UL);
#endif
#if defined(_16F15385)
  RC6PPS = 0x0F;                 /* RC6 = TX1 */
  RX1DTPPS = 0x17;               /* RX1 = RC7 */
#elif defined(_16F18877)
  RC6PPS = 0x10;                 /* RC6 = TX */
  RXPPS = 0x17;                  /* RX = RC7 */
#endif
  /* El USART toma sus pines; TRIS como pide cada datasheet */
#if defined(_16F88)
  TRISBbits.TRISB5 = 1; TRISBbits.TRISB2 = 1;
#elif defined(_16F690)
  TRISBbits.TRISB7 = 1; TRISBbits.TRISB5 = 1;
#elif defined(PIC_K_ENHNEW)
  TRISCbits.TRISC6 = 0; TRISCbits.TRISC7 = 1;
#else
  TRISCbits.TRISC6 = 1; TRISCbits.TRISC7 = 1;
#endif
  TXSTA = 0x24;                  /* TXEN, BRGH, asíncrono, 8 bits */
  RCSTA = 0x90;                  /* SPEN, CREN */
}

void Serial_end(void)
{
  RCSTA = 0;
  TXSTA = 0;
}

void Serial_write(uint8_t c)
{
  while (!TX_IF) { }
  TXREG = c;
}

void Serial_flush(void) { while (!TXSTAbits.TRMT) { } }

uint8_t Serial_available(void)
{
  if (RCSTAbits.OERR) {          /* se perdió un byte: reiniciar el receptor */
    RCSTAbits.CREN = 0;
    RCSTAbits.CREN = 1;
  }
  return RC_IF ? 1u : 0u;
}

int Serial_read(void)
{
  if (!Serial_available()) return -1;
  return RCREG;
}

int Serial_peek(void) { return -1; }
#endif

#endif
