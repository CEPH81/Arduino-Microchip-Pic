/*
 * 02_HolaSerial - Envía un texto y un contador por Serial cada segundo.
 *
 * Monitor Serie a 9600 baudios, con un conversor USB-serie en los pines
 * PIN_SERIAL_TX / PIN_SERIAL_RX del chip (RC6/RC7 en casi todos los PIC16
 * y PIC18; ver la tabla del README). Las Curiosity Nano ya traen el
 * conversor en su USB.
 * No sirve en los PIC sin USART (16F84, 16F84A, 16F872).
 */
unsigned int cuenta = 0;

void setup() {
  Serial.begin(9600);
}

void loop() {
  Serial.print("Hola desde el PIC, cuenta = ");
  Serial.println(cuenta);
  cuenta++;
  delay(1000);
}
