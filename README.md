# Arduino-Microchip-Pic

Núcleo Arduino para microcontroladores **Microchip PIC de 8, 16 y 32 bits**. El sketch se escribe en el Arduino IDE, se compila con los compiladores oficiales **MPLAB XC8, XC16 y XC32** (gratis) y se graba con el **PICkit 3** u otro programador de Microchip desde el botón **Subir**.

> **Estado (versión 0.1.1):** base del núcleo: pines digitales, tiempos y `Serial`. Los 6 ejemplos compilan en los 23 chips con los compiladores reales de Microchip (128 compilaciones, 0 errores). **Todavía no se ha probado en ningún PIC real**, ni la grabación con el PICkit 3, ni el paquete en Windows.

## Contenido

1. [Chips soportados](#chips-soportados)
2. [Instalación](#instalación)
3. [Uso: menú Herramientas](#uso-menú-herramientas)
4. [Conexión del PICkit 3](#conexión-del-pickit-3)
5. [Primer programa](#primer-programa)
6. [Pines, LED y Serial de cada chip](#pines-led-y-serial-de-cada-chip)
7. [Relojes disponibles](#relojes-disponibles)
8. [Funciones disponibles](#funciones-disponibles)
9. [Diferencias con un Arduino normal](#diferencias-con-un-arduino-normal)
10. [Ejemplos](#ejemplos)
11. [Bits de configuración](#bits-de-configuración)
12. [Problemas frecuentes](#problemas-frecuentes)
13. [Por qué los compiladores no vienen en el paquete](#por-qué-los-compiladores-no-vienen-en-el-paquete)
14. [Cómo está hecho](#cómo-está-hecho)
15. [Pendiente](#pendiente)
16. [Créditos](#créditos)

## Chips soportados

| Familia | Chips | Compilador |
|---|---|---|
| PIC16 clásicos | 16F84, 16F84A, 16F88, 16F690, 16F870, 16F872, 16F874, 16F874A, 16F877, 16F877A | XC8 |
| PIC16 mejorados | 16F1947, 16F15385, 16F18877 | XC8 |
| PIC18 | 18F2420, 18F2520, 18F2455, 18F2550, 18F57Q84 (Curiosity Nano) | XC8 |
| PIC24 / dsPIC33 | PIC24FJ16GA002, dsPIC33EP32GS502, dsPIC33EP64MC206, dsPIC33CK64MC105 (Curiosity Nano) | XC16 o XC-DSC |
| PIC32 | PIC32MM0256GPM064 (PIC32MM USB Curiosity) | XC32 |

Todavía **no** están: PIC32CM-LS00 Curiosity (es ARM Cortex-M23) y AVR32SD32 Curiosity Nano (es AVR).

## Instalación

### 1. El núcleo

Arduino IDE › Archivo › Preferencias › *URLs adicionales del Gestor de placas*:

```
https://raw.githubusercontent.com/CEPH81/Arduino-Microchip-Pic/main/package_ceph81_pic_index.json
```

Después: Herramientas › Placa › Gestor de placas › buscar **Microchip PIC (CEPH81)** › Instalar. El paquete pesa unos 7 MB.

### 2. El compilador (se descarga solo la primera vez)

La primera vez que se compila para una familia, si falta su compilador el núcleo **descarga el instalador oficial desde microchip.com** a la carpeta de Descargas y lo abre:

1. Pulse **Verificar**. En la consola aparece el avance de la descarga.
2. Se abre el instalador de Microchip. Instale con las opciones por defecto; en el tipo de licencia elija **Free**.
3. Vuelva a pulsar **Verificar**.

| Familia | Compilador que se descarga | Descarga | Instalado |
|---|---|---|---|
| 8 bits (PIC16, PIC18) | MPLAB XC8 v2.50 | 85 MB | 2,2 GB |
| 16 bits (PIC24, dsPIC33) | MPLAB XC16 v2.10 | 125 MB | 1,9 GB |
| 32 bits (PIC32MM) | MPLAB XC32 v5.00 | 1,1 GB | 7 GB |

Detalles:

- Si ya está instalado XC8, XC16, XC-DSC o XC32, **se usa ese y no se descarga nada**. En XC8 sirve la v2.50 (trae los datos de todos los chips) o una v3.x si MPLAB X tiene instalado el paquete del chip (DFP).
- Si la descarga se corta, al volver a compilar continúa donde quedó.
- En Linux el instalador pide permisos de administrador; si no se puede abrir solo, la consola muestra la orden `sudo` para ejecutarlo.
- En macOS se descargan XC8 y XC16; XC32 hay que bajarlo a mano de microchip.com.
- Para desactivar la descarga automática: `picbuild.autoinstalar=no` en `platform.local.txt` (ver abajo).

### 3. MPLAB X IDE con MPLAB IPE (para grabar con el PICkit)

El PICkit se maneja con `ipecmd`, que viene con MPLAB X IDE / MPLAB IPE. **Para el PICkit 3 use MPLAB X v6.20 o anterior**: las versiones nuevas ya no lo incluyen. Si falta, al pulsar **Subir** se abre la [página de versiones anteriores de Microchip](https://www.microchip.com/en-us/tools-resources/archives/mplab-ecosystem) para descargarlo (aprox. 1 GB). Las placas Curiosity Nano no lo necesitan.

### Carpetas en otro sitio

El núcleo busca los compiladores e IPE en su carpeta normal (`C:\Program Files\Microchip`, `/opt/microchip`, `/Applications/microchip`) y elige la versión más nueva que sirva para el chip. Si están en otro sitio, cree el archivo `platform.local.txt` junto al `platform.txt` del paquete:

```
picbuild.xc8=D:/Microchip/xc8/v2.50
picbuild.xc16=D:/Microchip/xc16/v2.10
picbuild.xc32=D:/Microchip/xc32/v5.00
picbuild.ipe=D:/Microchip/MPLABX/v6.20/mplab_platform/mplab_ipe/ipecmd.exe
picbuild.autoinstalar=no
```

El paquete queda en `%LOCALAPPDATA%\Arduino15\packages\CEPH81\hardware\pic\<versión>\` (Windows) o `~/.arduino15/packages/CEPH81/hardware/pic/<versión>/` (Linux).

## Uso: menú Herramientas

- **Placa** › Microchip PIC (CEPH81) › el chip.
- **Reloj**: cristal u oscilador interno y su frecuencia. Con esto el núcleo pone los bits de configuración del oscilador y calcula tiempos y baudios. **Debe coincidir con el circuito** (si elige "Cristal 20 MHz" tiene que haber un cristal de 20 MHz).
- **Programador**: PICkit 3 (por defecto en los chips sueltos), PICkit 4, PICkit 5, MPLAB SNAP, ICD 3, ICD 4, PKOB, PKOB4, o *Unidad CURIOSITY*.
- **Alimentar desde el programador**: *No* (la placa tiene su fuente), *5.0 V* o *3.3 V*. El PICkit 3 puede alimentar un chip suelto con poco consumo.

**Verificar** compila; **Subir** compila y graba.

## Conexión del PICkit 3

Conector ICSP del PICkit 3 (el pin 1 es el de la flecha):

| PICkit 3 | Señal | En el PIC |
|---|---|---|
| 1 | MCLR / VPP | MCLR (con resistencia de 10 kΩ a VDD) |
| 2 | VDD | VDD |
| 3 | GND | VSS |
| 4 | PGD (datos) | PGD / ICSPDAT |
| 5 | PGC (reloj) | PGC / ICSPCLK |
| 6 | PGM / LVP | sin conectar |

Pines PGD / PGC más usados: RB7 / RB6 en PIC16F84/88/87x, PIC16F1947 y PIC18F2xxx; RA0 / RA1 en PIC16F690. En PIC24 y dsPIC33 use el par PGED1 / PGEC1. Ante la duda, mire el datasheet del chip.

No ponga condensadores ni cargas fuertes en PGD y PGC. Si el circuito consume más de unos 30 mA, aliméntelo con su propia fuente y deje *Alimentar desde el programador* en *No*.

**Placas Curiosity Nano** (PIC18F57Q84, dsPIC33CK64MC105): no usan el PICkit. Elija *Unidad CURIOSITY*: al conectar la placa aparece una unidad USB llamada CURIOSITY y el núcleo copia ahí el `.hex`.

## Primer programa

```cpp
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
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

`LED_BUILTIN` es el LED de la placa (Curiosity) o, en un chip suelto, el pin que propone el núcleo (tabla siguiente): conecte ahí un LED con una resistencia de 330 Ω a GND.

## Pines, LED y Serial de cada chip

Los pines se llaman como en el datasheet: `PIN_RA0`, `PIN_RB5`, `PIN_RC6`… Por dentro son un número: puerto × 8 + bit en los PIC de 8 bits, y puerto × 16 + bit en los de 16 y 32 bits.

| Chip | Flash | RAM | `LED_BUILTIN` | Serial TX / RX | Programador por defecto |
|---|---|---|---|---|---|
| PIC16F84, 16F84A | 1 K palabras | 68 | RB0 | no tiene | PICkit 3 |
| PIC16F88 | 4 K | 368 | RB0 | RB5 / RB2 | PICkit 3 |
| PIC16F690 | 4 K | 256 | RC0 | RB7 / RB5 | PICkit 3 |
| PIC16F870 | 2 K | 128 | RB0 | RC6 / RC7 | PICkit 3 |
| PIC16F872 | 2 K | 128 | RB0 | no tiene | PICkit 3 |
| PIC16F874, 874A | 4 K | 192 | RB0 | RC6 / RC7 | PICkit 3 |
| PIC16F877, 877A | 8 K | 368 | RB0 | RC6 / RC7 | PICkit 3 |
| PIC16F1947 | 16 K | 1024 | RB0 | RC6 / RC7 | PICkit 3 |
| PIC16F15385 | 16 K | 2048 | RB0 | RC6 / RC7 (PPS) | PICkit 3 |
| PIC16F18877 | 32 K | 4096 | RB0 | RC6 / RC7 (PPS) | PICkit 3 |
| PIC18F2420 | 16 KB | 768 | RB0 | RC6 / RC7 | PICkit 3 |
| PIC18F2520 | 32 KB | 1536 | RB0 | RC6 / RC7 | PICkit 3 |
| PIC18F2455 | 24 KB | 2048 | RB0 | RC6 / RC7 | PICkit 3 |
| PIC18F2550 | 32 KB | 2048 | RB0 | RC6 / RC7 | PICkit 3 |
| PIC18F57Q84 (Curiosity Nano) | 128 KB | 12,8 KB | RF3 (enciende en LOW) | RF0 / RF1 | Unidad CURIOSITY |
| PIC24FJ16GA002 | 16 KB | 4 KB | RB15 | RB7 / RB8 (RP7, RP8) | PICkit 3 |
| dsPIC33EP32GS502 | 32 KB | 4 KB | RB15 | RB11 / RB12 (RP43, RP44) | PICkit 3 |
| dsPIC33EP64MC206 | 64 KB | 8 KB | RB15 | RC6 / RC7 (RP54, RP55) | PICkit 3 |
| dsPIC33CK64MC105 (Curiosity Nano) | 64 KB | 8 KB | RD10 (enciende en LOW) | RC10 / RC11 | Unidad CURIOSITY |
| PIC32MM0256GPM064 (USB Curiosity) | 256 KB | 32 KB | RD3 | RC12 / RA6 | PICkit 3 |

- En los PIC16 la flash se mide en **palabras** de 14 bits; el IDE las muestra como "bytes".
- `LED_BUILTIN_ON` vale `HIGH` o `LOW` según cómo encienda el LED de la placa.
- `PIN_BOTON` existe en las placas Curiosity: RB4 (18F57Q84), RD13 (33CK64MC105), RB9 (PIC32MM).
- En las Curiosity Nano el `Serial` sale por el USB de la placa (puerto COM virtual).
- Para ver el `Serial` de un chip suelto hace falta un conversor USB-serie (TX del PIC a RX del conversor, RX a TX y GND común).

## Relojes disponibles

| Chips | Opciones del menú Reloj |
|---|---|
| 16F84 | Cristal 4 MHz (XT), 8 y 10 MHz (HS) |
| 16F84A, 16F87x | Cristal 4 MHz (XT), 8, 10, 16 y 20 MHz (HS) |
| 16F88, 16F690 | Interno 8 y 4 MHz; cristal 4, 8, 16 y 20 MHz |
| 16F1947 | Interno 16, 32 (8 MHz × 4), 8 y 4 MHz; cristal 16 y 20 MHz |
| 16F15385, 16F18877 | Interno 32, 16, 8 y 4 MHz; cristal 16 y 20 MHz |
| 18F2420, 18F2520 | Interno 8, 32 (× 4) y 4 MHz; cristal 4, 8, 16 y 20 MHz; cristal 10 MHz × 4 = 40 MHz |
| 18F2455, 18F2550 | Interno 8 y 4 MHz; cristal de 20, 8 o 4 MHz + PLL = 48 MHz; cristal 4, 8 y 20 MHz |
| 18F57Q84 | Interno 64, 32, 16 y 8 MHz |
| PIC24FJ16GA002 | Interno 8 MHz × 4 (FCY 16 MHz); interno 8 MHz (FCY 4 MHz) |
| dsPIC33EP | Interno 7,37 MHz (FCY 3,685 MHz); interno + PLL (FCY 70 MHz) |
| dsPIC33CK64MC105 | Interno 8 MHz (FCY 4 MHz); interno + PLL (FCY 100 MHz) |
| PIC32MM | Interno 8 MHz; interno 8 MHz × 3 = 24 MHz |

La primera opción de cada fila es la que queda por defecto. En los PIC de 16 bits, `F_CPU` es FCY (la frecuencia de instrucciones).

## Funciones disponibles

| Función | Estado | Notas |
|---|---|---|
| `pinMode(pin, INPUT / OUTPUT / INPUT_PULLUP)` | ✓ | En PIC16/PIC18 clásicos solo el PORTB tiene pull-ups internas |
| `digitalWrite`, `digitalRead`, `digitalToggle` | ✓ | |
| `millis`, `micros`, `delay`, `delayMicroseconds` | ✓ | 8 bits: Timer0. 16 y 32 bits: Timer1 |
| `Serial.begin`, `print`, `println`, `write`, `read`, `available`, `flush`, `end` | ✓ | Menos 16F84, 84A y 872 (no tienen USART) |
| `interrupts`, `noInterrupts` | ✓ | |
| `map`, `min`, `max`, `constrain`, `bitRead`, `bitSet`, `lowByte`… | ✓ | |
| `analogRead`, `analogWrite` (PWM), `tone` | siguiente fase | |
| `attachInterrupt`, I2C (`Wire`), `SPI`, `EEPROM` | siguiente fase | |

Qué temporizador queda ocupado: **Timer0** en los PIC de 8 bits y **Timer1** en los de 16 y 32 bits (los usa `millis()`). El resto están libres.

`Serial.print()` acepta:

```cpp
Serial.print("texto");
Serial.print('A');            // la letra
Serial.print(1234);           // enteros con o sin signo, de 8, 16 y 32 bits
Serial.print(255, HEX);       // DEC, HEX, OCT, BIN
Serial.print(3.14159);        // float: 2 decimales
Serial.print(3.14159, 4);     // float: 4 decimales
Serial.println();             // salto de línea
```

## Diferencias con un Arduino normal

XC8 y XC16 solo compilan **C**, no C++. El sketch se escribe igual que en Arduino, y la herramienta `picbuild` del paquete lo convierte a C antes de compilar (`Serial.print(x)` pasa a ser una macro del núcleo). Consecuencias:

- **No hay** clases, `String`, `new`, sobrecarga de funciones ni referencias (`&`) de C++. Las bibliotecas de Arduino escritas en C++ no sirven.
- Las variables se declaran al estilo C99 (se pueden declarar en medio del código y dentro del `for`).
- **`float` en 8 bits cuesta memoria**: imprimir un número con decimales agrega unas 2000 palabras. En un PIC16F870 (2 K) no cabe.
- `picbuild` reconoce un `float` en `Serial.print()` por: un número con punto (`3.14`), un cast `(float)`, una variable o función declarada `float`/`double` en el sketch, o una función matemática (`sqrt`, `sin`…). Si se le escapa uno, el compilador dice *array size is negative* en esa línea: escriba `Serial.print((float)x)`.
- En los PIC de 8 bits el `Serial` **no usa interrupciones ni búfer**: el chip guarda 2 bytes recibidos. Lea `Serial.available()` con frecuencia o perderá datos a velocidades altas.
- `Serial.peek()` no está disponible (devuelve −1).
- En PIC16/PIC18 **no ponga `delay()` largos dentro de funciones llamadas desde muchos sitios**: la pila de llamadas es de 8 niveles (PIC16 clásicos) o 16 (mejorados) y 31 (PIC18).
- `delayMicroseconds()` es aproximado en 8 bits (pasos de 10 µs, o 20 µs por debajo de 8 MHz). Para tiempos exactos con una constante use `__delay_us(n)` de XC8.

## Ejemplos

Archivo › Ejemplos › PIC:

| Ejemplo | Qué hace |
|---|---|
| 01_Parpadeo | LED cada medio segundo |
| 02_HolaSerial | Mensaje por `Serial` cada segundo (9600 baudios) |
| 03_EcoSerial | Devuelve cada carácter recibido y su código en hexadecimal |
| 04_Boton | LED encendido mientras se pulsa un botón (pull-up interna; RB4 o el botón de la placa) |
| 05_SinDelay | Parpadeo con `millis()`, sin detener el programa |
| 06_Numeros | Formatos de `Serial.print`: decimal, hexadecimal, binario, negativos, `float`, `millis` |

## Bits de configuración

El núcleo los pone solo, según el chip y el menú **Reloj**. No hace falta escribir `#pragma config`.

- **Oscilador**: el del menú Reloj (XT, HS, interno, PLL).
- **Perro guardián (WDT)**: apagado.
- **MCLR**: funciona como reset (hace falta la resistencia de 10 kΩ a VDD).
- **Brown-out reset**: encendido. **Protección de código**: apagada.
- **LVP** (programación en bajo voltaje):
  - PIC clásicos (16F87x, 16F88, 16F690, 18F2xxx): **apagada**, para no perder el pin PGM. El PICkit 3 programa en alto voltaje sin problema.
  - PIC nuevos (16F1947, 16F15385, 16F18877, 18F57Q84): **encendida**, porque las Curiosity Nano solo programan en bajo voltaje y en estos chips no ocupa ningún pin extra.
- **PIC24 / dsPIC33 / PIC32MM**: arrancan con el oscilador interno; si el menú pide PLL, el núcleo lo enciende al arrancar. JTAG apagado (libera pines) y PPS reconfigurable.
- **18F57Q84**: un solo vector de interrupción (`MVECEN = OFF`).
- **Pines analógicos**: al arrancar el núcleo los deja todos como digitales (ANSEL, ADCON1) y apaga los comparadores.

Los valores exactos están en `pic/cores/pic/config8.h`, `config16.h` y `config32.h`.

## Problemas frecuentes

| Mensaje o síntoma | Causa y solución |
|---|---|
| *no encuentro el compilador MPLAB XC8* (o XC16, XC32) | No está instalado o está en otra carpeta. Deje que se descargue, o indique la carpeta en `platform.local.txt` |
| *El instalador ya se abrió hace N min* | Termine la instalación y vuelva a compilar. Si lo cerró, ábralo desde Descargas |
| *el XC8 instalado no trae el PIC…* | Tiene XC8 v3.x sin el paquete del chip. Instale XC8 v2.50 (se descarga solo) o el DFP desde MPLAB X › Tools › Packs |
| *no encuentro ipecmd* | Falta MPLAB X / IPE, o está en otra carpeta (`picbuild.ipe=`) |
| *ipecmd terminó con error* | Revise: PICkit conectado, placa alimentada (o active *Alimentar desde el programador*), chip correcto en el menú, cables PGD/PGC/MCLR, y que MPLAB X o IPE no estén usando el PICkit a la vez |
| El PICkit 3 no aparece en IPE | MPLAB X más nuevo que v6.20: instale la v6.20 o una anterior |
| *no encuentro la unidad CURIOSITY* | Conecte la Curiosity Nano por su USB de depuración y espere a que aparezca la unidad |
| *array size is negative* en un `Serial.print` | Es un `float` que no se reconoció: `Serial.print((float)x)` |
| El programa no cabe | En 8 bits evite `float` y textos largos; los PIC16F84/870/872 tienen 1–2 K palabras |
| El LED parpadea a otra velocidad o el `Serial` muestra basura | El menú **Reloj** no coincide con el cristal u oscilador real |
| No arranca después de grabar | Falta la resistencia de MCLR a VDD, o el menú Reloj pide un cristal que no está |

Para ver las órdenes exactas que se ejecutan: Archivo › Preferencias › *Mostrar salida detallada* durante compilación y carga.

## Por qué los compiladores no vienen en el paquete

Desde el 8 de julio de 2026 Microchip ofrece XC8, XC16, XC-DSC y XC32 **gratis y con todas las optimizaciones** (antes eran de pago). Pero gratis no es lo mismo que redistribuible: la licencia que trae cada compilador (`docs/MPLAB_XC8_C_Compiler_License`) solo concede el derecho a *instalarlo y usarlo*, y prohíbe expresamente su "distribución no autorizada". Además pesan entre 2 y 7 GB instalados.

Por eso el paquete del Gestor de placas no los contiene. En su lugar, el núcleo descarga el instalador oficial **desde el sitio de Microchip, en el equipo de cada usuario**, que es quien acepta la licencia. El resultado práctico es casi el mismo: se instala solo la primera vez.

Con MPLAB X / IPE pasa igual, y además Microchip no permite la descarga directa de ese instalador: por eso el núcleo abre la página de descargas en el navegador.

Lo único de Microchip que sí va dentro del paquete son los datos del PIC32MM (`pic/dfp/`: header, arranque y script de enlace), cuya licencia sí permite redistribuirlos.

## Cómo está hecho

```
pic/
  platform.txt            recetas del IDE: todas llaman a picbuild
  boards.txt              los chips y sus menús (generado por tools/gen_boards.py)
  cores/pic/
    Arduino.h             API de Arduino en C y macros de Serial.print
    pic_chip.h            clasifica el chip (familia, LAT, USART) y sus pines por defecto
    pic_pins.h            nombres PIN_Rxn
    main.c                init(), setup() y loop()
    config8.h / config16.h / config32.h    bits de configuración por chip
    wiring8.c             8 bits: reloj, Timer0, millis, delay
    wiring_digital8.c     8 bits: pinMode, digitalWrite, digitalRead
    serial8.c             8 bits: USART, EUSART y UART del 18F57Q84
    wiring16.c            PIC24 / dsPIC33: reloj, PLL, Timer1, pines, UART1
    wiring32.c            PIC32MM: reloj, PLL, Timer1, pines, UART1
    print.c               números y texto por Serial, sin printf
  dfp/                    datos del PIC32MM para XC32 (de Microchip)
  libraries/PIC/          ejemplos
tools/
  picbuild/               puente entre el IDE y XC8 / XC16 / XC32 / ipecmd (Go)
  gen_boards.py           genera boards.txt
  make_release.py         compila picbuild, empaqueta y actualiza el índice
  instalar_xc.sh          instala los XC en GitHub Actions
  ci_compilar.sh          compila todos los ejemplos en todos los chips
  sonda.sh                consultas a los compiladores instalados en GitHub Actions
.github/workflows/        compilar.yml, publicar.yml, sonda.yml
package_ceph81_pic_index.json    índice para el Gestor de placas
```

### picbuild

El Arduino IDE está pensado para GCC y C++. `picbuild` es un programa pequeño (un ejecutable para Windows, otro para Linux y otro para macOS) que traduce cada paso del IDE:

| Paso del IDE | Qué hace picbuild |
|---|---|
| Buscar herramientas | Localiza XC8, XC16 / XC-DSC, XC32 e `ipecmd`; si falta el compilador lo descarga de Microchip |
| Convertir el sketch | `Serial.print(x)` → `Serial__print(x)`; `Serial.print('A')` imprime la letra; marca los `float` (`Serial__printF`) |
| Compilar | XC8: `xc8-cc -c` (código intermedio `.p1`). XC16 y XC32: `gcc -c`. El sketch se compila como C |
| Archivar y enlazar | `xc8-ar` / `xc16-ar` / `xc32-ar`; genera `.elf` y `.hex` |
| Tamaño | Lee el resumen de memoria del compilador para que el IDE muestre flash y RAM |
| Subir | `ipecmd -TP<programador> -P<chip> -F<hex> -M -OL [-W<voltios>]`, o copia el `.hex` a la unidad CURIOSITY |

`picbuild donde` muestra qué compiladores, MPLAB X e `ipecmd` encontró.

### Cómo se elige el tipo en Serial.print

XC8 no tiene `_Generic` y no descarta el código que no se usa de un `? :`. La macro del núcleo averigua el tipo del argumento al compilar (con `__builtin_types_compatible_p` dentro de un `sizeof`, que es lo que XC8 sí acepta) y llama a una sola función con un número que dice qué tipo llegó. Los `float` van por otra función, para que el código de punto flotante solo entre en la flash cuando el sketch imprime decimales: `02_HolaSerial` ocupa unas 900 palabras en un PIC16F88, frente a 3300 si se incluyera siempre. En XC16 y XC32 (que son GCC) la macro elige sola.

### Pruebas automáticas

Desde el equipo de desarrollo no se pueden descargar los compiladores, así que todo se comprueba en GitHub Actions: `compilar.yml` instala XC8 v2.50 y v3.10, XC16 v2.10 y XC32 v5.00 (quedan en caché) y compila los 6 ejemplos en los 23 chips en cada cambio. `publicar.yml` empaqueta y publica una versión nueva cuando cambia `version=` en `pic/platform.txt`.

## Pendiente

- Probar en hardware: cada familia, los bits de configuración, los baudios y la grabación con el PICkit 3.
- Probar el paquete y la descarga del compilador en Windows.
- `analogRead`, PWM (`analogWrite`), `tone`, `attachInterrupt`, I2C, SPI, EEPROM.
- `Serial` con interrupciones y búfer en los chips con RAM suficiente.
- PIC32CM-LS00 Curiosity (ARM) y AVR32SD32 Curiosity Nano (AVR).
- Más PIC18: confirmar qué otros modelos de la serie 18F2xxx hacen falta.

## Créditos

- `pic/dfp/` (header, arranque y script de enlace del PIC32MM) es de Microchip y conserva su licencia tipo BSD (ver la cabecera de cada archivo).
- MPLAB, XC8, XC16, XC32, PICkit y Curiosity son marcas de Microchip Technology Inc. Este proyecto no está afiliado a Microchip.
- Autor: CEPH81 (MikrocontrollerCephs).
