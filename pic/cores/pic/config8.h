/*
 * config8.h - Bits de configuración de los PIC de 8 bits. Lo incluye main.c
 * (así siempre entra en el programa, aunque el núcleo sea una biblioteca).
 *
 * El menú Herramientas > Reloj define F_CPU y una de estas macros:
 *   PIC_OSC_XT      cristal hasta 4 MHz
 *   PIC_OSC_HS      cristal de más de 4 MHz
 *   PIC_OSC_HSPLL   cristal de 10 MHz x4 (PIC18F2x20)
 *   PIC_OSC_INT     oscilador interno
 *   PIC_OSC_USB48   PIC18F2455/2550: cristal + PLL de 96 MHz, CPU a 48 MHz
 *                   (PIC_OSC_PLLDIV = MHz del cristal / 4)
 *
 * Comunes: perro guardián apagado, MCLR como reset, protección de código
 * apagada. LVP (programación en bajo voltaje):
 *   - PIC clásicos (16F87x, 16F88, 18F2xxx): apagada, para no perder el pin
 *     PGM (RB3/RB5). El PICkit 3 programa en alto voltaje sin problema.
 *   - PIC nuevos (16F1947, 16F15385, 16F18877, 18F57Q84): encendida, porque
 *     las placas Curiosity Nano solo programan en bajo voltaje y en estos
 *     chips LVP no ocupa ningún pin extra.
 */
#if defined(PIC_8BIT)

#if !defined(PIC_K_PIC18)
#pragma config WDTE = OFF
#endif
#if !defined(PIC_K_PIC18) && !defined(PIC_K_PIC18Q) && !defined(PIC_K_ENHNEW)
#pragma config PWRTE = ON
#endif

/* ============================================================ PIC16F84/84A */
#if defined(_16F84) || defined(_16F84A)
  #if defined(PIC_OSC_HS)
    #pragma config FOSC = HS
  #else
    #pragma config FOSC = XT
  #endif
  #pragma config CP = OFF

/* ================================================= PIC16F870..877 y 87xA */
#elif defined(_16F870) || defined(_16F871) || defined(_16F872) || defined(_16F873) || defined(_16F874) || \
      defined(_16F876) || defined(_16F877) || defined(_16F873A) || defined(_16F874A) || defined(_16F876A) || defined(_16F877A)
  #if defined(PIC_OSC_HS)
    #pragma config FOSC = HS
  #else
    #pragma config FOSC = XT
  #endif
  #pragma config BOREN = ON
  #pragma config LVP = OFF
  #pragma config CPD = OFF
  #pragma config CP = OFF

/* ================================================================ PIC16F88 */
#elif defined(_16F88)
  #if defined(PIC_OSC_HS)
    #pragma config FOSC = HS
  #elif defined(PIC_OSC_XT)
    #pragma config FOSC = XT
  #else
    #pragma config FOSC = INTOSCIO
  #endif
  #pragma config MCLRE = ON
  #pragma config BOREN = ON
  #pragma config LVP = OFF
  #pragma config CPD = OFF
  #pragma config CP = OFF
  #pragma config FCMEN = OFF
  #pragma config IESO = OFF

/* =============================================================== PIC16F690 */
#elif defined(_16F690)
  #if defined(PIC_OSC_HS)
    #pragma config FOSC = HS
  #elif defined(PIC_OSC_XT)
    #pragma config FOSC = XT
  #else
    #pragma config FOSC = INTRCIO
  #endif
  #pragma config MCLRE = ON
  #pragma config BOREN = ON
  #pragma config CPD = OFF
  #pragma config CP = OFF
  #pragma config FCMEN = OFF
  #pragma config IESO = OFF

/* ============================================================== PIC16F1947 */
#elif defined(_16F1947)
  #if defined(PIC_OSC_HS)
    #pragma config FOSC = HS
  #elif defined(PIC_OSC_XT)
    #pragma config FOSC = XT
  #else
    #pragma config FOSC = INTOSC
  #endif
  #pragma config MCLRE = ON
  #pragma config BOREN = ON
  #pragma config CLKOUTEN = OFF
  #pragma config IESO = OFF
  #pragma config FCMEN = OFF
  #pragma config PLLEN = OFF          /* el PLL x4 lo enciende el núcleo (SPLLEN) */
  #pragma config STVREN = ON
  #pragma config LVP = ON
  #pragma config CPD = OFF
  #pragma config CP = OFF

/* ================================================= PIC16F15385 / PIC16F18877 */
#elif defined(PIC_K_ENHNEW)
  #if defined(PIC_OSC_HS)
    #pragma config FEXTOSC = HS
    #pragma config RSTOSC = EXT1X
  #elif defined(PIC_OSC_XT)
    #pragma config FEXTOSC = XT
    #pragma config RSTOSC = EXT1X
  #else
    #pragma config FEXTOSC = OFF
    #pragma config RSTOSC = HFINT32   /* 32 MHz; el núcleo divide con OSCCON1 */
  #endif
  #pragma config CLKOUTEN = OFF
  #pragma config CSWEN = ON
  #pragma config FCMEN = OFF
  #pragma config MCLRE = ON
  #pragma config PWRTE = OFF
  #pragma config BOREN = ON
  #pragma config PPS1WAY = OFF        /* los pines PPS se pueden cambiar más de una vez */
  #pragma config STVREN = ON
  #pragma config LVP = ON
  #pragma config CP = OFF

/* ===================================================== PIC18F2420/2520/2525 */
#elif defined(_18F2420) || defined(_18F2520) || defined(_18F2525) || defined(_18F2620) || \
      defined(_18F4420) || defined(_18F4520) || defined(_18F4525) || defined(_18F4620)
  #if defined(PIC_OSC_HSPLL)
    #pragma config OSC = HSPLL
  #elif defined(PIC_OSC_HS)
    #pragma config OSC = HS
  #elif defined(PIC_OSC_XT)
    #pragma config OSC = XT
  #else
    #pragma config OSC = INTIO67
  #endif
  #pragma config WDT = OFF
  #pragma config FCMEN = OFF
  #pragma config IESO = OFF
  #pragma config PWRT = ON
  #pragma config BOREN = SBORDIS
  #pragma config PBADEN = OFF         /* PORTB digital al arrancar */
  #pragma config MCLRE = ON
  #pragma config LVP = OFF
  #pragma config XINST = OFF

/* ========================================================= PIC18F2455/2550 */
#elif defined(_18F2455) || defined(_18F2550) || defined(_18F4455) || defined(_18F4550)
  #if defined(PIC_OSC_USB48)
    #if PIC_OSC_PLLDIV == 5
      #pragma config PLLDIV = 5       /* cristal de 20 MHz */
    #elif PIC_OSC_PLLDIV == 2
      #pragma config PLLDIV = 2       /* cristal de 8 MHz */
    #else
      #pragma config PLLDIV = 1       /* cristal de 4 MHz */
    #endif
    #pragma config CPUDIV = OSC1_PLL2 /* 96 MHz / 2 = 48 MHz */
    #pragma config USBDIV = 2
    #if PIC_OSC_PLLDIV == 1
      #pragma config FOSC = XTPLL_XT
    #else
      #pragma config FOSC = HSPLL_HS
    #endif
  #elif defined(PIC_OSC_HS)
    #pragma config CPUDIV = OSC1_PLL2
    #pragma config FOSC = HS
  #elif defined(PIC_OSC_XT)
    #pragma config CPUDIV = OSC1_PLL2
    #pragma config FOSC = XT_XT
  #else
    #pragma config FOSC = INTOSCIO_EC
  #endif
  #pragma config WDT = OFF
  #pragma config FCMEN = OFF
  #pragma config IESO = OFF
  #pragma config PWRT = ON
  #pragma config BOR = ON
  #pragma config VREGEN = OFF
  #pragma config PBADEN = OFF
  #pragma config MCLRE = ON
  #pragma config LVP = OFF
  #pragma config XINST = OFF

/* ============================================================= PIC18F57Q84 */
#elif defined(PIC_K_PIC18Q)
  #pragma config FEXTOSC = OFF
  #pragma config RSTOSC = HFINTOSC_64MHZ   /* 64 MHz; el núcleo divide con OSCCON1 */
  #pragma config CLKOUTEN = OFF
  #pragma config PR1WAY = OFF
  #pragma config CSWEN = ON
  #pragma config JTAGEN = OFF
  #pragma config FCMEN = OFF
  #pragma config MCLRE = EXTMCLR
  #pragma config PWRTS = PWRT_64
  #pragma config MVECEN = OFF              /* un solo vector de interrupción */
  #pragma config BOREN = SBORDIS
  #pragma config PPS1WAY = OFF
  #pragma config STVREN = ON
  #pragma config LVP = ON
  #pragma config XINST = OFF

#else
  #error "Chip de 8 bits sin bits de configuración en config8.h"
#endif

#endif /* PIC_8BIT */
