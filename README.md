# Arduino-Microchip-Pic

Núcleo Arduino para microcontroladores **Microchip PIC de 8, 16 y 32 bits**. Se escribe el sketch en el Arduino IDE, se compila con los compiladores oficiales **MPLAB XC** (gratis) y se graba con el **PICkit 3** (u otro programador de Microchip) desde el botón **Subir**.

> Versión 0.1: base (pines digitales, tiempos y `Serial`). Compila en los 23 chips de la lista; **todavía sin probar en hardware**.

## Chips

| Familia | Chips | Compilador |
|---|---|---|
| PIC16 clásicos | 16F84, 16F84A, 16F88, 16F690, 16F870, 16F872, 16F874, 16F874A, 16F877, 16F877A | XC8 |
| PIC16 mejorados | 16F1947, 16F15385, 16F18877 | XC8 |
| PIC18 | 18F2420, 18F2520, 18F2455, 18F2550, 18F57Q84 (Curiosity Nano) | XC8 |
| PIC24 / dsPIC33 | 24FJ16GA002, 33EP32GS502, 33EP64MC206, 33CK64MC105 (Curiosity Nano) | XC16 o XC-DSC |
| PIC32 | 32MM0256GPM064 (PIC32MM USB Curiosity) | XC32 |

Pendientes: PIC32CM-LS00 Curiosity (ARM Cortex-M23) y AVR32SD32 Curiosity Nano (AVR).

## Instalación

1. **Compiladores** (se instalan aparte, desde microchip.com; su licencia no permite incluirlos en este paquete). Solo el de la familia que use:
   - 8 bits: [MPLAB XC8](https://www.microchip.com/xc8). Recomendado **v2.50**, que ya trae los datos de todos los chips. Las v3.x necesitan además el paquete del chip (DFP) de MPLAB X.
   - 16 bits: [MPLAB XC16](https://www.microchip.com/xc16) (v2.10) o XC-DSC.
   - 32 bits: [MPLAB XC32](https://www.microchip.com/xc32).
2. **MPLAB X IDE con MPLAB IPE** para grabar (trae `ipecmd`). Para el **PICkit 3 use MPLAB X v6.20 o anterior**: las versiones nuevas ya no lo incluyen.
3. **Arduino IDE** › Archivo › Preferencias › *URLs adicionales del Gestor de placas*:

   ```
   https://raw.githubusercontent.com/CEPH81/Arduino-Microchip-Pic/main/package_ceph81_pic_index.json
   ```

   Después, Herramientas › Placa › Gestor de placas › buscar **Microchip PIC (CEPH81)** › Instalar (unos 3 MB).

El núcleo busca los compiladores e IPE en su carpeta normal (`C:\Program Files\Microchip`, `/opt/microchip`, `/Applications/microchip`). Si están en otro sitio, cree `platform.local.txt` junto a `platform.txt` del paquete:

```
picbuild.xc8=D:/Microchip/xc8/v2.50
picbuild.xc16=D:/Microchip/xc16/v2.10
picbuild.xc32=D:/Microchip/xc32/v5.00
picbuild.ipe=D:/Microchip/MPLABX/v6.20/mplab_platform/mplab_ipe/ipecmd.exe
```

## Uso

Menú **Herramientas**:

- **Placa**: el chip.
- **Reloj**: cristal u oscilador interno y su frecuencia. El núcleo pone solo los bits de configuración (oscilador, perro guardián apagado, MCLR como reset).
- **Programador**: PICkit 3 (por defecto), PICkit 4/5, SNAP, ICD 3/4, PKOB, o *Unidad CURIOSITY* (las placas Curiosity Nano se programan copiando el `.hex` a su unidad USB, sin IPE).
- **Alimentar desde el programador**: No, 5.0 V o 3.3 V (el PICkit 3 puede alimentar un chip suelto).

Conexión del PICkit 3 (ICSP): 1 MCLR/VPP · 2 VDD · 3 GND · 4 PGD · 5 PGC.

```cpp
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);     // RB0 en PIC16/PIC18 sueltos
  Serial.begin(9600);
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
  Serial.println(millis());
}
```

Los pines se nombran como en el datasheet: `PIN_RA0`, `PIN_RB5`, `PIN_RC6`…

## Qué funciona

| Función | Estado |
|---|---|
| `pinMode`, `digitalWrite`, `digitalRead`, `digitalToggle` | ✓ |
| `millis`, `micros`, `delay`, `delayMicroseconds` | ✓ |
| `Serial` (begin, print, println, write, read, available) | ✓ (menos 16F84/84A/872, que no tienen USART) |
| `analogRead`, `analogWrite` (PWM), `tone`, interrupciones externas, I2C, SPI, EEPROM | siguiente fase |

## Diferencias con un Arduino normal

XC8 y XC16 solo compilan **C**, no C++. El sketch se escribe igual (`Serial.print(x)`), y la herramienta `picbuild` lo pasa a C antes de compilar. Por eso:

- No hay clases, `String`, `new` ni bibliotecas de Arduino en C++.
- `Serial.print()` acepta texto, `char`, enteros (con `DEC`, `HEX`, `BIN`) y `float` (con decimales).
- En 8 bits, imprimir un `float` agrega unas 2000 palabras de código. Si el compilador dice *array size is negative* en un `Serial.print`, es un `float` que `picbuild` no reconoció: escriba `Serial.print((float)x)`.
- En PIC16 la memoria se cuenta en **palabras** (el IDE dice "bytes").
- En los PIC de 8 bits `Serial` no usa interrupciones ni búfer: lea `Serial.available()` con frecuencia.

## Cómo está hecho

```
pic/
  platform.txt, boards.txt   recetas del IDE (todas llaman a picbuild) y chips
  cores/pic/                 API de Arduino en C: wiring8/16/32, serial8, print, config de cada familia
  dfp/                       datos del PIC32MM para XC32 (de Microchip)
  libraries/PIC/             ejemplos
tools/
  picbuild/                  puente IDE <-> XC8 / XC16 / XC32 / ipecmd (Go)
  gen_boards.py              genera boards.txt
  make_release.py            empaqueta y actualiza el índice
  instalar_xc.sh, ci_compilar.sh   instalan los XC en GitHub Actions y compilan todo
```

Cada cambio se compila en GitHub Actions con XC8, XC16 y XC32 reales (todos los ejemplos en todos los chips).

## Créditos

`pic/dfp/` (header, arranque y script de enlace del PIC32MM) es de Microchip y conserva su licencia tipo BSD (ver la cabecera de cada archivo). MPLAB, XC8, XC16, XC32 y PICkit son marcas de Microchip Technology Inc.; este proyecto no está afiliado a Microchip.
