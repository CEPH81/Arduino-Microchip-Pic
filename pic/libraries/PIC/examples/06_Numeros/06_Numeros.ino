/*
 * 06_Numeros - Formatos de Serial.print: decimal, hexadecimal, binario,
 * negativos, decimales y tiempo. Monitor Serie a 9600 baudios.
 */
void setup() {
  Serial.begin(9600);
}

void loop() {
  int n = -1234;
  unsigned int u = 51966;
  float f = 3.14159;
  char letra = 'A';

  Serial.print("int: ");        Serial.println(n);
  Serial.print("unsigned: ");   Serial.println(u);
  Serial.print("hex: ");        Serial.println(u, HEX);
  Serial.print("bin: ");        Serial.println(202, BIN);
  Serial.print("float: ");      Serial.println(f);
  Serial.print("4 decimales: ");Serial.println(f, 4);
  Serial.print("char: ");       Serial.println(letra);
  Serial.print("millis: ");     Serial.println(millis());
  Serial.println();
  delay(2000);
}
