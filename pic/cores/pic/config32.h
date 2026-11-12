/*
 * config32.h - Bits de configuración del PIC32MM (lo incluye main.c).
 * Arranca con el FRC de 8 MHz; con el menú Reloj en 24 MHz el núcleo
 * enciende el PLL (8 MHz x3).
 */
#if defined(PIC_32BIT)

#pragma config FNOSC = FRCDIV
#pragma config PLLSRC = FRC
#pragma config SOSCEN = OFF
#pragma config IESO = OFF
#pragma config POSCMOD = OFF
#pragma config OSCIOFNC = OFF
#pragma config SOSCSEL = OFF
#pragma config FCKSM = CSECMD           /* cambio de reloj permitido (para el PLL) */
#pragma config FWDTEN = OFF
#pragma config JTAGEN = OFF
#pragma config ICS = PGx1
#pragma config CP = OFF

#endif /* PIC_32BIT */
