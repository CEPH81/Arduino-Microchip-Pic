/*
 * 03_EcoSerial - Devuelve cada carácter que llega por Serial y muestra su
 * código en hexadecimal. Monitor Serie a 9600 baudios.
 */
void setup() {
  Serial.begin(9600);
  Serial.println("Escriba algo:");
}

void loop() {
  if (Serial.available()) {
    int c = Serial.read();
    Serial.write(c);
    Serial.print(" = 0x");
    Serial.println(c, HEX);
  }
}
