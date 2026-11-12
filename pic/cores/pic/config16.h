/*
 * config16.h - Bits de configuración de PIC24 / dsPIC33 (lo incluye main.c).
 *
 * Arrancan con el oscilador interno (FRC). Con el menú Reloj en "PLL"
 * (PIC_OSC_PLL) el núcleo enciende el PLL al arrancar (dsPIC33) o se
 * arranca ya con FRCPLL (PIC24FJ). Perro guardián apagado, JTAG apagado
 * (libera pines), PPS reconfigurable (IOL1WAY = OFF).
 */
#if defined(PIC_16BIT)

#if defined(__PIC24FJ16GA002__) || defined(__PIC24FJ32GA002__) || defined(__PIC24FJ48GA002__) || defined(__PIC24FJ64GA002__)
  #pragma config POSCMOD = NONE
  #pragma config IOL1WAY = OFF
  #pragma config OSCIOFNC = ON
  #pragma config FCKSM = CSDCMD
  #if defined(PIC_OSC_PLL)
    #pragma config FNOSC = FRCPLL       /* 8 MHz x4 = 32 MHz, FCY = 16 MHz */
  #else
    #pragma config FNOSC = FRC          /* 8 MHz, FCY = 4 MHz */
  #endif
  #pragma config IESO = OFF
  #pragma config FWDTEN = OFF
  #pragma config ICS = PGx1
  #pragma config GWRP = OFF
  #pragma config GCP = OFF
  #pragma config JTAGEN = OFF

#elif defined(__dsPIC33EP32GS502__)
  #pragma config FNOSC = FRC
  #pragma config IESO = OFF
  #pragma config POSCMD = NONE
  #pragma config OSCIOFNC = ON
  #pragma config IOL1WAY = OFF
  #pragma config FCKSM = CSECMD         /* cambio de reloj permitido (para el PLL) */
  #pragma config WDTEN = OFF
  #pragma config ICS = PGD1
  #pragma config JTAGEN = OFF
  #pragma config GWRP = OFF

#elif defined(__dsPIC33EP64MC206__)
  #pragma config FNOSC = FRC
  #pragma config IESO = OFF
  #pragma config POSCMD = NONE
  #pragma config OSCIOFNC = ON
  #pragma config IOL1WAY = OFF
  #pragma config FCKSM = CSECMD
  #pragma config FWDTEN = OFF
  #pragma config ICS = PGD1
  #pragma config JTAGEN = OFF
  #pragma config GWRP = OFF
  #pragma config GCP = OFF

#elif defined(__dsPIC33CK64MC105__)
  #pragma config FNOSC = FRC
  #pragma config IESO = OFF
  #pragma config POSCMD = NONE
  #pragma config OSCIOFNC = ON
  #pragma config FCKSM = CSECMD
  #pragma config FWDTEN = ON_SW         /* perro guardián por software: apagado */
  #pragma config ICS = PGD1
  #pragma config JTAGEN = OFF
  #pragma config GWRP = OFF

#else
  #error "Chip de 16 bits sin bits de configuración en config16.h"
#endif

#endif /* PIC_16BIT */
