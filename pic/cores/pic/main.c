/*
 * main.c - Punto de entrada: init() prepara reloj, pines y millis();
 * después corre el sketch.
 */
#include "Arduino.h"
#include "config8.h"
#include "config16.h"
#include "config32.h"

void init(void);

#if defined(PIC_8BIT)
void main(void)
#else
int main(void)
#endif
{
  init();
  setup();
  for (;;) {
    loop();
  }
}
