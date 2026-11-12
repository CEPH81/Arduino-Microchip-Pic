/*
 * 04_Boton - El LED se enciende mientras el botón está pulsado.
 *
 * Botón entre el pin y GND (se usa la resistencia pull-up interna). En las
 * placas Curiosity se usa su botón. En los PIC16/PIC18 clásicos solo el
 * PORTB tiene pull-ups internas: por eso el pin propuesto es RB4.
 */
#ifdef PIN_BOTON
  #define BOTON PIN_BOTON
#else
  #define BOTON PIN_RB4
#endif

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BOTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(BOTON) == LOW) {
    digitalWrite(LED_BUILTIN, LED_BUILTIN_ON);
  } else {
    digitalWrite(LED_BUILTIN, !LED_BUILTIN_ON);
  }
}
