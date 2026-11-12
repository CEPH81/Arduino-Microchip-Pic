/*
 * 01_Parpadeo - Enciende y apaga un LED cada medio segundo.
 *
 * LED_BUILTIN es el LED de la placa (Curiosity) o, en un chip suelto, el
 * pin que el núcleo propone: RB0 en los PIC16/PIC18 (RC0 en el 16F690) y
 * RB15 en PIC24/dsPIC33. Conecte ahí un LED con su resistencia de 330 ohm
 * a GND, o cambie PIN_LED por otro pin (PIN_RB1, PIN_RD0...).
 */
#define PIN_LED LED_BUILTIN

void setup() {
  pinMode(PIN_LED, OUTPUT);
}

void loop() {
  digitalWrite(PIN_LED, HIGH);
  delay(500);
  digitalWrite(PIN_LED, LOW);
  delay(500);
}
