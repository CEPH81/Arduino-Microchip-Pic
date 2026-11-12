/*
 * 05_SinDelay - Parpadeo con millis(), sin detener el programa: loop()
 * sigue libre para hacer otras cosas.
 */
unsigned long anterior = 0;
const unsigned long INTERVALO = 250;   // ms

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  unsigned long ahora = millis();
  if (ahora - anterior >= INTERVALO) {
    anterior = ahora;
    digitalToggle(LED_BUILTIN);
  }
}
