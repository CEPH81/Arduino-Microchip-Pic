/*
 * Arduino.h - API de Arduino en C para Microchip PIC de 8, 16 y 32 bits
 * (compiladores MPLAB XC8, XC16 / XC-DSC y XC32).
 *
 * El sketch se escribe como en Arduino (setup, loop, Serial.print...). La
 * herramienta picbuild lo pasa a C antes de compilar: "Serial.print(x)"
 * se vuelve "Serial__print(x)", una macro que elige la función según el
 * tipo de x. Lo que no hay: clases, String, new/delete (es C, no C++).
 */
#ifndef Arduino_h
#define Arduino_h

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifndef F_CPU
  #error "Falta F_CPU (lo define el menú Herramientas > Reloj)"
#endif

/* ----------------------------------------------------------- constantes */
#define HIGH            1
#define LOW             0
#define INPUT           0
#define OUTPUT          1
#define INPUT_PULLUP    2

#define DEC 10
#define HEX 16
#define OCT 8
#define BIN 2

#define LSBFIRST 0
#define MSBFIRST 1

#define PI          3.1415926535897932
#define HALF_PI     1.5707963267948966
#define TWO_PI      6.283185307179586
#define DEG_TO_RAD  0.017453292519943295
#define RAD_TO_DEG  57.29577951308232

typedef uint8_t  boolean;
typedef uint8_t  byte;
typedef uint16_t word;

#include "pic_chip.h"

/* ---------------------------------------------------------------- macros */
#ifndef min
#define min(a, b)               ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a, b)               ((a) > (b) ? (a) : (b))
#endif
#define constrain(x, lo, hi)    ((x) < (lo) ? (lo) : ((x) > (hi) ? (hi) : (x)))
#define sq(x)                   ((x) * (x))
#define lowByte(w)              ((uint8_t)((w) & 0xFF))
#define highByte(w)             ((uint8_t)((w) >> 8))
#define bit(b)                  (1UL << (b))
#define bitRead(v, b)           (((v) >> (b)) & 1)
#define bitSet(v, b)            ((v) |= (1UL << (b)))
#define bitClear(v, b)          ((v) &= ~(1UL << (b)))
#define bitToggle(v, b)         ((v) ^= (1UL << (b)))
#define bitWrite(v, b, x)       ((x) ? bitSet(v, b) : bitClear(v, b))
#define clockCyclesPerMicrosecond()  (F_CPU / 1000000L)

/* ------------------------------------------------------------------ API */
void setup(void);
void loop(void);

void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
uint8_t digitalRead(uint8_t pin);
void digitalToggle(uint8_t pin);

unsigned long millis(void);
unsigned long micros(void);
void delay(unsigned long ms);
void delayMicroseconds(unsigned int us);

long map(long x, long in_min, long in_max, long out_min, long out_max);

void interrupts(void);
void noInterrupts(void);

/* --------------------------------------------------------------- Serial */
#if defined(PIC_HAS_SERIAL)
void    Serial_begin(unsigned long baud);
void    Serial_end(void);
uint8_t Serial_available(void);
int     Serial_read(void);
int     Serial_peek(void);
void    Serial_flush(void);
void    Serial_write(uint8_t c);
void    Serial_print_s(const char *s);
void    Serial_print_i(long v, uint8_t base);
void    Serial_print_u(unsigned long v, uint8_t base);
void    Serial_print_f(double v, uint8_t digits);
void    Serial_println0(void);

void    Serial_printk(uint8_t kind, uint8_t base, ...);
void    Serial_printkf(uint8_t digits, ...);

/*
 * Serial.print(x) / Serial.print(x, base): la macro averigua el tipo de x
 * al compilar y se lo pasa a Serial_printk() como un número:
 *   1 texto (char*)   2 char (imprime la letra)   3 float / double
 *   4 unsigned int    5 int    6 unsigned long    7 long
 * PIC_C() convierte la comprobación de tipos en una constante normal
 * (sizeof de un arreglo), que es lo que entienden los tres compiladores.
 * Los float van por Serial_printkf(): si el programa no imprime ninguno,
 * el código de punto flotante no entra en la flash.
 */
#define PIC_C(e)        (sizeof(char[1 + (e)]) - 1u)
#define PIC_T(x)        __typeof__((x) + 0)
#define PIC_IS(x, t)    __builtin_types_compatible_p(PIC_T(x), t)
#define PIC_IS_STR(x)   (PIC_IS(x, char *) || PIC_IS(x, const char *) || PIC_IS(x, unsigned char *) || PIC_IS(x, const unsigned char *))
#define PIC_IS_FLT(x)   (PIC_IS(x, float) || PIC_IS(x, double) || PIC_IS(x, long double))
#define PIC_IS_UNS(x)   (PIC_IS(x, unsigned int) || PIC_IS(x, unsigned long) || \
                         __builtin_types_compatible_p(__typeof__(x), unsigned char) || \
                         __builtin_types_compatible_p(__typeof__(x), unsigned short))
#define PIC_IS_CHAR(x)  __builtin_types_compatible_p(__typeof__(x), char)
#define PIC_KIND(x)     ((uint8_t)PIC_C(PIC_IS_STR(x) ? 1 : PIC_IS_CHAR(x) ? 2 : \
                         (PIC_IS_UNS(x) ? 4 : 5) + (sizeof((x) + 0) > sizeof(int) ? 2 : 0)))
#define PIC_PRINT_DEF   0xFFu      /* base 10, o 2 decimales */

#define PIC_PRINT2(x, b) \
  (PIC_C(PIC_IS_FLT(x)) ? Serial_printkf((uint8_t)(b), (x)) : Serial_printk(PIC_KIND(x), (uint8_t)(b), (x)))
#define PIC_PRINT1(x)   PIC_PRINT2(x, PIC_PRINT_DEF)
#define PIC_PRINT_SEL(a, b, macro, ...)  macro

#define Serial__begin(baud)     Serial_begin(baud)
#define Serial__end()           Serial_end()
#define Serial__available()     Serial_available()
#define Serial__read()          Serial_read()
#define Serial__peek()          Serial_peek()
#define Serial__flush()         Serial_flush()
#define Serial__write(c)        Serial_write((uint8_t)(c))
#define Serial__print(...)      PIC_PRINT_SEL(__VA_ARGS__, PIC_PRINT2, PIC_PRINT1, 0)(__VA_ARGS__)
#define Serial__println(...)    do { Serial__print(__VA_ARGS__); Serial_println0(); } while (0)
#define Serial__println0()      Serial_println0()
#endif /* PIC_HAS_SERIAL */

#endif /* Arduino_h */
