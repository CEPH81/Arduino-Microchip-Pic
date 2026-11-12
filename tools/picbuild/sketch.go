package main

// Paso del sketch (.ino) a C.
//
//  1. Sintaxis de objetos: Serial.print(x) -> Serial__print(x) (macros del núcleo).
//  2. Serial.print('A') imprime la letra; Serial.println() vacío.
//  3. Números con decimales: XC8 no sabe elegir la función según el tipo sin
//     arrastrar todo el código de punto flotante (1700 palabras), así que las
//     llamadas que imprimen un float se marcan aquí: Serial__printF(x).
//     Se reconoce un float por: un literal con punto (3.14), un cast
//     (float)/(double), una variable o función declarada float/double en el
//     sketch, o una función matemática (sin, sqrt...). Si se escapa alguno,
//     el núcleo da un error al compilar que pide escribir (float)x.

import (
	"bytes"
	"regexp"
)

var (
	reObjeto  = regexp.MustCompile(`\b(Serial[0-9]?|Wire|SPI|EEPROM)\s*\.\s*([A-Za-z_]\w*)\s*\(`)
	reVacio   = regexp.MustCompile(`\b(Serial[0-9]?__println)\(\s*\)`)
	reChar    = regexp.MustCompile(`\b(Serial[0-9]?__(?:print|println))\(\s*'((?:\\.|[^\\'])+)'\s*\)`)
	rePrint   = regexp.MustCompile(`\bSerial[0-9]?__(?:print|println)\(`)
	reDeclF   = regexp.MustCompile(`\b(?:float|double)\b([^;{}()]*(?:\([^;{}]*)?)`)
	reIdent   = regexp.MustCompile(`[A-Za-z_]\w*`)
	reLitF    = regexp.MustCompile(`(?:^|[^\w.])(?:\d+\.\d*(?:[eE][-+]?\d+)?|\.\d+(?:[eE][-+]?\d+)?|\d+[eE][-+]?\d+)[fFlL]?`)
	reCastF   = regexp.MustCompile(`\(\s*(?:float|double)\s*\)`)
	mateFloat = map[string]bool{"sin": true, "cos": true, "tan": true, "asin": true, "acos": true, "atan": true, "atan2": true,
		"sqrt": true, "pow": true, "exp": true, "log": true, "log10": true, "fabs": true, "floor": true, "ceil": true, "fmod": true,
		"sinf": true, "cosf": true, "sqrtf": true, "powf": true, "round": true}
	noNombres = map[string]bool{"float": true, "double": true, "const": true, "static": true, "volatile": true, "unsigned": true,
		"int": true, "long": true, "char": true, "void": true, "return": true, "sizeof": true}
)

// Copia de src con el contenido de cadenas, caracteres y comentarios en
// blanco (mismo largo): sirve para buscar sin tropezar con ellos.
func enBlanco(src []byte) []byte {
	b := append([]byte(nil), src...)
	n := len(b)
	for i := 0; i < n; {
		c := b[i]
		switch {
		case c == '"' || c == '\'':
			j := i + 1
			for j < n && b[j] != c && b[j] != '\n' {
				if b[j] == '\\' && j+1 < n {
					b[j] = ' '
					j++
				}
				b[j] = ' '
				j++
			}
			i = j + 1
		case c == '/' && i+1 < n && b[i+1] == '/':
			for i < n && b[i] != '\n' {
				b[i] = ' '
				i++
			}
		case c == '/' && i+1 < n && b[i+1] == '*':
			j := bytes.Index(b[i+2:], []byte("*/"))
			fin := n
			if j >= 0 {
				fin = i + 2 + j + 2
			}
			for ; i < fin; i++ {
				if b[i] != '\n' {
					b[i] = ' '
				}
			}
		default:
			i++
		}
	}
	return b
}

// Aplica f a los tramos de código (no a cadenas ni comentarios)
func soloCodigo(src []byte, f func([]byte) []byte) []byte {
	m := enBlanco(src)
	var out bytes.Buffer
	ini := 0
	for i := 0; i <= len(src); i++ {
		// un tramo de código termina donde la máscara difiere del original
		if i == len(src) || m[i] != src[i] {
			out.Write(f(src[ini:i]))
			j := i
			for j < len(src) && m[j] != src[j] {
				j++
			}
			out.Write(src[i:j])
			ini = j
			i = j
			if i == len(src) && ini == len(src) {
				break
			}
		}
	}
	return out.Bytes()
}

// nombres declarados como float/double (variables y funciones) en todo el sketch
func nombresFloat(m []byte) map[string]bool {
	r := map[string]bool{}
	for _, d := range reDeclF.FindAllSubmatch(m, -1) {
		// "float a = 1.5, b[3], *c" o "float leer(int x"
		decl := d[1]
		prof := 0
		ini := 0
		tomar := func(seg []byte) {
			// el nombre es el primer identificador del declarador
			for _, id := range reIdent.FindAll(seg, -1) {
				if !noNombres[string(id)] {
					r[string(id)] = true
					return
				}
			}
		}
		for i := 0; i <= len(decl); i++ {
			if i == len(decl) {
				tomar(decl[ini:i])
				break
			}
			switch decl[i] {
			case '(', '[':
				if prof == 0 {
					tomar(decl[ini:i]) // función o arreglo: el nombre va antes
					ini = len(decl)    // lo que sigue son parámetros o tamaños
				}
				prof++
			case ')', ']':
				prof--
			case ',':
				if prof == 0 {
					if ini < i {
						tomar(decl[ini:i])
					}
					ini = i + 1
				}
			case '=':
				if prof == 0 && ini < i {
					tomar(decl[ini:i])
					// saltar el valor inicial hasta la coma de nivel 0
					j := i + 1
					p := 0
					for j < len(decl) && !(decl[j] == ',' && p == 0) {
						if decl[j] == '(' || decl[j] == '[' || decl[j] == '{' {
							p++
						} else if decl[j] == ')' || decl[j] == ']' || decl[j] == '}' {
							p--
						}
						j++
					}
					i = j - 1
					ini = j + 1
					if ini > len(decl) {
						ini = len(decl)
					}
				}
			}
		}
	}
	return r
}

func esFloat(expr []byte, nombres map[string]bool) bool {
	if reLitF.Match(expr) || reCastF.Match(expr) {
		return true
	}
	for _, id := range reIdent.FindAll(expr, -1) {
		s := string(id)
		if nombres[s] || mateFloat[s] {
			return true
		}
	}
	return false
}

// Marca con F las llamadas Serial__print/println cuyo primer argumento es float
func marcarFloat(src []byte) []byte {
	m := enBlanco(src)
	nombres := nombresFloat(m)
	var out bytes.Buffer
	pos := 0
	for _, loc := range rePrint.FindAllIndex(m, -1) {
		abre := loc[1] - 1
		// primer argumento: hasta la coma o el paréntesis de cierre de nivel 0
		prof, fin := 0, -1
		for i := abre + 1; i < len(m); i++ {
			c := m[i]
			if c == '(' || c == '[' {
				prof++
			} else if c == ')' || c == ']' {
				if prof == 0 {
					fin = i
					break
				}
				prof--
			} else if c == ',' && prof == 0 {
				fin = i
				break
			}
		}
		if fin < 0 || !esFloat(m[abre+1:fin], nombres) {
			continue
		}
		out.Write(src[pos:abre])
		out.WriteByte('F')
		pos = abre
	}
	out.Write(src[pos:])
	return out.Bytes()
}

// Convierte la sintaxis de objetos de Arduino a las macros en C del núcleo.
func convertir(src []byte) []byte {
	b := soloCodigo(src, func(c []byte) []byte { return reObjeto.ReplaceAll(c, []byte("${1}__${2}(")) })
	// Serial.print('A') imprime la letra, como en Arduino (en C 'A' es un int)
	b = reChar.ReplaceAll(b, []byte(`${1}("${2}")`))
	b = reVacio.ReplaceAll(b, []byte("${1}0()"))
	return marcarFloat(b)
}
