/*
 * pic_chip.h - Clasifica el chip y fija lo que cambia de uno a otro:
 * tamaño de los puertos, estilo de los periféricos, pines del Serial y LED.
 *
 * Estilos de 8 bits (XC8):
 *   PIC_K_CLASSIC   PIC16F84/88/690/87x: sin LAT, Timer0 con OPTION_REG
 *   PIC_K_ENH       PIC16F1947: LAT, ANSELx, Timer0 con OPTION_REG
 *   PIC_K_ENHNEW    PIC16F15385/18877: Timer0 nuevo (T0CON0/1), OSCCON1, PPS
 *   PIC_K_PIC18     PIC18F2420/2520/2525/2455/2550: T0CON, ADCON1
 *   PIC_K_PIC18Q    PIC18F57Q84: Timer0 nuevo, UART nuevo (U1CON0), PPS
 * 16 bits (XC16 / XC-DSC):
 *   PIC_K_PIC24     PIC24FJ16GA002, dsPIC33EP32GS502, dsPIC33EP64MC206
 *   PIC_K_33CK      dsPIC33CK64MC105 (UART y PLL nuevos)
 * 32 bits (XC32):
 *   PIC_K_PIC32MM   PIC32MM0256GPM064
 */
#ifndef PIC_CHIP_H
#define PIC_CHIP_H

#if defined(__XC8)
  #define PIC_8BIT        1
  #define PIC_PORT_BITS   8
  #ifndef _XTAL_FREQ
    #define _XTAL_FREQ    F_CPU          /* para __delay_us() de XC8 */
  #endif
  #if defined(_PIC18)
    #if defined(U1CON0)
      #define PIC_K_PIC18Q  1
    #else
      #define PIC_K_PIC18   1
    #endif
  #elif defined(_PIC14E) || defined(_PIC14EX)
    #if defined(T0CON0)
      #define PIC_K_ENHNEW  1
    #else
      #define PIC_K_ENH     1
    #endif
  #else
    #define PIC_K_CLASSIC   1
  #endif
  /* ¿Tiene registros LAT? (si no, las salidas se escriben en PORT) */
  #if defined(LATB)
    #define PIC_HAS_LAT     1
  #endif
  /* USART / EUSART / UART */
  #if defined(TXSTA) || defined(U1CON0)
    #define PIC_HAS_SERIAL  1
  #endif
#elif defined(__XC16__) || defined(__XC_DSC__) || defined(__dsPIC33E__) || defined(__dsPIC33C__) || defined(__PIC24F__)
  #define PIC_16BIT       1
  #define PIC_PORT_BITS   16
  #define PIC_HAS_LAT     1
  #define PIC_HAS_SERIAL  1
  #if defined(__dsPIC33CK64MC105__) || defined(U1MODEH)
    #define PIC_K_33CK    1
  #else
    #define PIC_K_PIC24   1
  #endif
#elif defined(__XC32) || defined(__PIC32MM__)
  #define PIC_32BIT       1
  #define PIC_PORT_BITS   16
  #define PIC_HAS_LAT     1
  #define PIC_HAS_SERIAL  1
  #define PIC_K_PIC32MM   1
#else
  #error "Compilador o chip no soportado por este núcleo"
#endif

#include "pic_pins.h"

/* ---------------------------------------------------------------------
 * LED y pines del Serial por chip (se pueden cambiar en el sketch: son
 * solo los valores por defecto).
 * ------------------------------------------------------------------- */
#if defined(_18F57Q84)                   /* PIC18F57Q84 Curiosity Nano */
  #define LED_BUILTIN       PIN_RF3
  #define LED_BUILTIN_ON    LOW
  #define PIN_BOTON         PIN_RB4
  #define PIN_SERIAL_TX     PIN_RF0
  #define PIN_SERIAL_RX     PIN_RF1
#elif defined(__dsPIC33CK64MC105__)      /* dsPIC33CK64MC105 Curiosity Nano */
  #define LED_BUILTIN       PIN_RD10
  #define LED_BUILTIN_ON    LOW
  #define PIN_BOTON         PIN_RD13
  #define PIN_SERIAL_TX     PIN_RC10
  #define PIN_SERIAL_RX     PIN_RC11
#elif defined(__32MM0256GPM064__)        /* PIC32MM USB Curiosity */
  #define LED_BUILTIN       PIN_RD3
  #define LED_BUILTIN_ON    HIGH
  #define PIN_BOTON         PIN_RB9
  #define PIN_SERIAL_TX     PIN_RC12
  #define PIN_SERIAL_RX     PIN_RA6
#elif defined(__PIC24FJ16GA002__)
  #define LED_BUILTIN       PIN_RB15
  #define PIN_SERIAL_TX     PIN_RB7      /* RP7 */
  #define PIN_SERIAL_RX     PIN_RB8      /* RP8 */
#elif defined(__dsPIC33EP32GS502__)
  #define LED_BUILTIN       PIN_RB15
  #define PIN_SERIAL_TX     PIN_RB11     /* RP43 */
  #define PIN_SERIAL_RX     PIN_RB12     /* RP44 */
#elif defined(__dsPIC33EP64MC206__)
  #define LED_BUILTIN       PIN_RB15
  #define PIN_SERIAL_TX     PIN_RC6      /* RP54 */
  #define PIN_SERIAL_RX     PIN_RC7      /* RP55 */
#elif defined(_16F84) || defined(_16F84A)
  #define LED_BUILTIN       PIN_RB0
#elif defined(_16F88)
  #define LED_BUILTIN       PIN_RB0
  #define PIN_SERIAL_TX     PIN_RB5
  #define PIN_SERIAL_RX     PIN_RB2
#elif defined(_16F690)
  #define LED_BUILTIN       PIN_RC0
  #define PIN_SERIAL_TX     PIN_RB7
  #define PIN_SERIAL_RX     PIN_RB5
#else                                    /* PIC16F87x, 16F1947, 16F15385, 16F18877, PIC18F2xxx */
  #define LED_BUILTIN       PIN_RB0
  #if defined(PIC_HAS_SERIAL)
    #define PIN_SERIAL_TX   PIN_RC6
    #define PIN_SERIAL_RX   PIN_RC7
  #endif
#endif

#ifndef LED_BUILTIN_ON
  #define LED_BUILTIN_ON    HIGH
#endif

#endif /* PIC_CHIP_H */
