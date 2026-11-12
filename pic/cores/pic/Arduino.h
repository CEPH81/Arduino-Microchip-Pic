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

/*
 * Serial.print(x) / Serial.print(x, base): la macro elige según el tipo.
 *   texto (char*)        -> Serial_print_s
 *   char                 -> Serial_write (imprime la letra)
 *   float / double       -> Serial_print_f (2 decimales, o los que se pidan)
 *   enteros sin signo    -> Serial_print_u
 *   el resto de enteros  -> Serial_print_i
 */
#define PIC_T(x)        __typeof__((x) + 0)
#define PIC_IS(x, t)    __builtin_types_compatible_p(PIC_T(x), t)
#define PIC_IS_STR(x)   (PIC_IS(x, char *) || PIC_IS(x, const char *) || PIC_IS(x, unsigned char *) || PIC_IS(x, const unsigned char *))
#define PIC_IS_FLT(x)   (PIC_IS(x, float) || PIC_IS(x, double) || PIC_IS(x, long double))
#define PIC_IS_UNS(x)   (PIC_IS(x, unsigned int) || PIC_IS(x, unsigned long) || PIC_IS(x, unsigned long long) || \
                         __builtin_types_compatible_p(__typeof__(x), unsigned char) || \
                         __builtin_types_compatible_p(__typeof__(x), unsigned short))
#define PIC_IS_CHAR(x)  __builtin_types_compatible_p(__typeof__(x), char)
/* valores "seguros" para las ramas que no se usan (deben compilar igual) */
#define PIC_STR(x)      __builtin_choose_expr(PIC_IS_STR(x), (x), "")
#define PIC_NUM(x)      __builtin_choose_expr(PIC_IS_STR(x), 0, (x))

#define PIC_PRINT1(x) \
  __builtin_choose_expr(PIC_IS_STR(x),  Serial_print_s((const char *)PIC_STR(x)), \
  __builtin_choose_expr(PIC_IS_CHAR(x), Serial_write((uint8_t)PIC_NUM(x)), \
  __builtin_choose_expr(PIC_IS_FLT(x),  Serial_print_f(PIC_NUM(x), 2), \
  __builtin_choose_expr(PIC_IS_UNS(x),  Serial_print_u((unsigned long)PIC_NUM(x), 10), \
                                        Serial_print_i((long)PIC_NUM(x), 10)))))
#define PIC_PRINT2(x, b) \
  __builtin_choose_expr(PIC_IS_FLT(x),  Serial_print_f(PIC_NUM(x), (uint8_t)(b)), \
  __builtin_choose_expr(PIC_IS_UNS(x),  Serial_print_u((unsigned long)PIC_NUM(x), (uint8_t)(b)), \
                                        Serial_print_i((long)PIC_NUM(x), (uint8_t)(b))))
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
