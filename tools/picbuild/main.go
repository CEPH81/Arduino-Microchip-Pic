// picbuild - puente entre el Arduino IDE y los compiladores de Microchip.
//
// El IDE compila con recetas pensadas para GCC y C++. Los compiladores XC8
// y XC16 solo aceptan C, guardan su código intermedio en archivos propios
// (.p1) y se instalan aparte (no vienen en el paquete). picbuild:
//
//   - busca el compilador instalado (XC8, XC16/XC-DSC, XC32) y, si hace
//     falta, el paquete de dispositivo (DFP) de MPLAB X;
//
//   - convierte el sketch (.ino) a C: "Serial.print(x)" pasa a
//     "Serial__print(x)", que es una macro del núcleo;
//
//   - traduce cada paso del IDE (preprocesar, compilar, archivar, enlazar,
//     medir tamaño, subir) a las órdenes del compilador que toque;
//
//   - graba con ipecmd (MPLAB IPE: PICkit 3/4/5, ICD, SNAP) o copiando el
//     .hex a la unidad CURIOSITY de las placas Curiosity Nano.
//
//     picbuild <orden> -tc xc8|xc16|xc32|arm|avr -mcu CHIP [opciones] ...
//
// Órdenes: pre, cc, ar, link, size, upload, donde (muestra lo que encontró).
package main

import (
	"bytes"
	"fmt"
	"io"
	"os"
	"os/exec"
	"path/filepath"
	"regexp"
	"runtime"
	"sort"
	"strconv"
	"strings"
)

type opciones struct {
	tc    string // xc8, xc16, xc32, arm, avr
	mcu   string // 16F877A, 33EP64MC206, 32MM0256GPM064...
	xc    string // ruta del compilador XC (opcional)
	gcc   string // ruta del GCC de Arduino (arm, avr)
	dfp   string // DFP incluido en el paquete (xc32, avr)
	ipe   string // ruta de ipecmd (opcional)
	tool  string // PK3, PK4, PK5, ICD3, ICD4, SNAP, CURIOSITY
	power string // "", "3.3", "5.0"...
	verb  bool
	resto []string
}

func fallo(f string, a ...interface{}) {
	fmt.Fprintf(os.Stderr, "picbuild: "+f+"\n", a...)
	os.Exit(1)
}

// valor de una propiedad del IDE: vacío si no se resolvió ("{...}")
func prop(s string) string {
	s = strings.TrimSpace(s)
	if s == "" || strings.Contains(s, "{") {
		return ""
	}
	return s
}

func existe(p string) bool { _, err := os.Stat(p); return err == nil }

func exe(p string) string {
	if runtime.GOOS == "windows" && !strings.HasSuffix(strings.ToLower(p), ".exe") {
		return p + ".exe"
	}
	return p
}

// ---------------------------------------------------------------- búsqueda

// carpetas donde Microchip instala sus herramientas
func raices() []string {
	var r []string
	switch runtime.GOOS {
	case "windows":
		for _, v := range []string{"ProgramFiles", "ProgramFiles(x86)", "ProgramW6432"} {
			if d := os.Getenv(v); d != "" {
				r = append(r, filepath.Join(d, "Microchip"))
			}
		}
		r = append(r, `C:\Program Files\Microchip`, `C:\Program Files (x86)\Microchip`)
	case "darwin":
		r = append(r, "/Applications/microchip")
	default:
		r = append(r, "/opt/microchip")
	}
	return r
}

// "v2.50" -> clave para ordenar versiones
func claveVersion(dir string) string {
	nums := regexp.MustCompile(`\d+`).FindAllString(filepath.Base(dir), -1)
	k := ""
	for _, n := range nums {
		k += fmt.Sprintf("%06s.", n)
	}
	return k
}

// versiones instaladas de un producto (xc8, xc16, xc-dsc, xc32, MPLABX), la más nueva primero
func versiones(producto string) []string {
	visto := map[string]bool{}
	var v []string
	for _, r := range raices() {
		m, _ := filepath.Glob(filepath.Join(r, producto, "v*"))
		for _, d := range m {
			if !visto[d] {
				visto[d] = true
				v = append(v, d)
			}
		}
	}
	sort.Slice(v, func(i, j int) bool { return claveVersion(v[i]) > claveVersion(v[j]) })
	return v
}

type herramientas struct {
	raiz  string // carpeta del compilador
	cc    string // xc8-cc, xc16-gcc, xc32-gcc, arm-none-eabi-gcc...
	ar    string
	hex   string // bin2hex / objcopy
	size  string
	extra []string // -mdfp=...
}

// packs de MPLAB X (del IDE y los descargados por el usuario)
func carpetasPacks() []string {
	var p []string
	for _, x := range versiones("MPLABX") {
		p = append(p, filepath.Join(x, "packs", "Microchip"))
	}
	if h, err := os.UserHomeDir(); err == nil {
		p = append(p, filepath.Join(h, ".mchp_packs", "Microchip"))
	}
	return p
}

// DFP de XC8 que trae el header de 'mcu' (el más nuevo)
func dfpXC8(mcu string) string {
	h := "pic" + strings.ToLower(mcu) + ".h"
	var c []string
	for _, base := range carpetasPacks() {
		m, _ := filepath.Glob(filepath.Join(base, "*_DFP", "*", "xc8", "pic", "include", "proc", h))
		c = append(c, m...)
	}
	if len(c) == 0 {
		return ""
	}
	sort.Slice(c, func(i, j int) bool {
		return claveVersion(filepath.Dir(filepath.Dir(filepath.Dir(filepath.Dir(filepath.Dir(c[i])))))) >
			claveVersion(filepath.Dir(filepath.Dir(filepath.Dir(filepath.Dir(filepath.Dir(c[j]))))))
	})
	return filepath.Dir(filepath.Dir(filepath.Dir(filepath.Dir(c[0])))) // .../xc8
}

func buscar(o *opciones) herramientas {
	var h herramientas
	bin := func(raiz, n string) string { return exe(filepath.Join(raiz, "bin", n)) }
	switch o.tc {
	case "xc8":
		cand := versiones("xc8")
		if p := prop(o.xc); p != "" {
			cand = []string{p}
		} else if p := os.Getenv("XC8_PATH"); p != "" {
			cand = []string{p}
		}
		hdr := "pic" + strings.ToLower(o.mcu) + ".h"
		// 1.º un compilador que traiga el chip; 2.º el más nuevo con un DFP de MPLAB X
		for _, c := range cand {
			if existe(bin(c, "xc8-cc")) && existe(filepath.Join(c, "pic", "include", "proc", hdr)) {
				h.raiz = c
				break
			}
		}
		if h.raiz == "" {
			for _, c := range cand {
				if existe(bin(c, "xc8-cc")) {
					if d := dfpXC8(o.mcu); d != "" {
						h.raiz = c
						h.extra = []string{"-mdfp=" + d}
					}
					break
				}
			}
		}
		if h.raiz == "" {
			if len(cand) == 0 {
				fallo("no encuentro el compilador MPLAB XC8.\n  Instálelo desde https://www.microchip.com/xc8 (es gratis) o indique su carpeta en\n  platform.local.txt:  picbuild.xc8=C:/Program Files/Microchip/xc8/v2.50")
			}
			fallo("el XC8 instalado (%s) no trae el PIC%s y no hay un paquete de dispositivo (DFP) en MPLAB X.\n  Instale XC8 v2.50 o abra MPLAB X > Tools > Packs e instale el DFP de este chip.", cand[0], o.mcu)
		}
		h.cc, h.ar = bin(h.raiz, "xc8-cc"), bin(h.raiz, "xc8-ar")
	case "xc16":
		type par struct{ prod, pre string }
		var cand []par
		if p := prop(o.xc); p != "" {
			cand = []par{{p, ""}}
		} else if p := os.Getenv("XC16_PATH"); p != "" {
			cand = []par{{p, ""}}
		} else {
			for _, d := range versiones("xc16") {
				cand = append(cand, par{d, "xc16"})
			}
			for _, d := range versiones("xc-dsc") {
				cand = append(cand, par{d, "xc-dsc"})
			}
		}
		for _, c := range cand {
			for _, pre := range []string{"xc16", "xc-dsc"} {
				if existe(bin(c.prod, pre+"-gcc")) {
					h.raiz = c.prod
					h.cc, h.ar, h.hex = bin(c.prod, pre+"-gcc"), bin(c.prod, pre+"-ar"), bin(c.prod, pre+"-bin2hex")
					h.size = bin(c.prod, pre+"-size")
					break
				}
			}
			if h.raiz != "" {
				break
			}
		}
		if h.raiz == "" {
			fallo("no encuentro el compilador MPLAB XC16 ni XC-DSC.\n  Instálelo desde https://www.microchip.com/xc16 (es gratis) o indique su carpeta en\n  platform.local.txt:  picbuild.xc16=C:/Program Files/Microchip/xc16/v2.10")
		}
	case "xc32":
		cand := versiones("xc32")
		if p := prop(o.xc); p != "" {
			cand = []string{p}
		} else if p := os.Getenv("XC32_PATH"); p != "" {
			cand = []string{p}
		}
		for _, c := range cand {
			if existe(bin(c, "xc32-gcc")) {
				h.raiz = c
				break
			}
		}
		if h.raiz == "" {
			fallo("no encuentro el compilador MPLAB XC32.\n  Instálelo desde https://www.microchip.com/xc32 (es gratis) o indique su carpeta en\n  platform.local.txt:  picbuild.xc32=C:/Program Files/Microchip/xc32/v4.45")
		}
		h.cc, h.ar, h.hex, h.size = bin(h.raiz, "xc32-gcc"), bin(h.raiz, "xc32-ar"), bin(h.raiz, "xc32-bin2hex"), bin(h.raiz, "xc32-size")
		// los XC32 anteriores a v4.x traen los chips; los nuevos necesitan un DFP
		propio := filepath.Join(h.raiz, "pic32mx", "include", "proc", "p"+strings.ToLower(o.mcu)+".h")
		if d := prop(o.dfp); d != "" && !existe(propio) {
			h.extra = []string{"-mdfp=" + d}
		}
	case "arm", "avr":
		pre := map[string]string{"arm": "arm-none-eabi-", "avr": "avr-"}[o.tc]
		g := prop(o.gcc)
		if g == "" || !existe(bin(g, pre+"gcc")) {
			fallo("no encuentro %sgcc (lo instala el Gestor de placas junto con este paquete): %s", pre, o.gcc)
		}
		h.raiz = g
		h.cc, h.ar, h.hex, h.size = bin(g, pre+"gcc"), bin(g, pre+"ar"), bin(g, pre+"objcopy"), bin(g, pre+"size")
	default:
		fallo("familia desconocida: -tc %q", o.tc)
	}
	return h
}

// ---------------------------------------------------------------- ejecutar

func correr(o *opciones, dir string, salida io.Writer, prog string, args ...string) error {
	if o.verb {
		fmt.Fprintln(os.Stderr, "picbuild>", prog, strings.Join(args, " "))
	}
	c := exec.Command(prog, args...)
	c.Dir = dir
	c.Stdout = salida
	c.Stderr = os.Stderr
	return c.Run()
}

func salirSi(err error) {
	if err != nil {
		if e, ok := err.(*exec.ExitError); ok {
			os.Exit(e.ExitCode())
		}
		fallo("%v", err)
	}
}

// ---------------------------------------------------------------- sketch

var (
	reObjeto = regexp.MustCompile(`\b(Serial[0-9]?|Wire|SPI|EEPROM)\s*\.\s*([A-Za-z_]\w*)\s*\(`)
	reVacio  = regexp.MustCompile(`\b(Serial[0-9]?__println)\(\s*\)`)
	reChar   = regexp.MustCompile(`\b(Serial[0-9]?__(?:print|println))\(\s*'((?:\\.|[^\\'])+)'\s*\)`)
)

// Convierte la sintaxis de objetos de Arduino a las macros en C del núcleo.
// Solo toca código: respeta cadenas, caracteres y comentarios.
func convertir(src []byte) []byte {
	var out bytes.Buffer
	code := func(b []byte) {
		b = reObjeto.ReplaceAll(b, []byte("${1}__${2}("))
		out.Write(b)
	}
	i, ini := 0, 0
	n := len(src)
	for i < n {
		c := src[i]
		switch {
		case c == '"' || c == '\'':
			code(src[ini:i])
			j := i + 1
			for j < n && src[j] != c && src[j] != '\n' {
				if src[j] == '\\' {
					j++
				}
				j++
			}
			if j < n {
				j++
			}
			out.Write(src[i:j])
			i, ini = j, j
		case c == '/' && i+1 < n && src[i+1] == '/':
			code(src[ini:i])
			j := i
			for j < n && src[j] != '\n' {
				j++
			}
			out.Write(src[i:j])
			i, ini = j, j
		case c == '/' && i+1 < n && src[i+1] == '*':
			code(src[ini:i])
			j := bytes.Index(src[i+2:], []byte("*/"))
			if j < 0 {
				j = n
			} else {
				j += i + 4
			}
			out.Write(src[i:j])
			i, ini = j, j
		default:
			i++
		}
	}
	code(src[ini:])
	// Serial.print('A') imprime la letra, como en Arduino (en C 'A' es un int)
	b := reChar.ReplaceAll(out.Bytes(), []byte(`${1}("${2}")`))
	// Serial.println() sin argumentos
	return reVacio.ReplaceAll(b, []byte("${1}0()"))
}

func esSketch(src string) bool { return strings.HasSuffix(strings.ToLower(src), ".ino.cpp") }

// Deja en 'dst' (un .c) la fuente lista para un compilador de C
func prepararC(src, dst string) {
	b, err := os.ReadFile(src)
	if err != nil {
		fallo("%v", err)
	}
	if esSketch(src) {
		b = convertir(b)
	}
	cab := fmt.Sprintf("#line 1 %s\n", strconv.Quote(filepath.ToSlash(src)))
	if err := os.WriteFile(dst, append([]byte(cab), b...), 0644); err != nil {
		fallo("%v", err)
	}
}

// ---------------------------------------------------------------- órdenes

// flags comunes de XC8 para compilar y enlazar
func baseXC8(o *opciones, h herramientas) []string {
	a := []string{"-mcpu=" + o.mcu}
	return append(a, h.extra...)
}

// separa los flags propios de GCC que XC8 no entiende
func flagsXC8(resto []string) []string {
	var f []string
	for _, a := range resto {
		switch {
		case strings.HasPrefix(a, "-I"), strings.HasPrefix(a, "-D"), strings.HasPrefix(a, "-U"),
			strings.HasPrefix(a, "-O"), strings.HasPrefix(a, "-m"), strings.HasPrefix(a, "-std="),
			strings.HasPrefix(a, "-W"), strings.HasPrefix(a, "-f"), a == "-w":
			f = append(f, a)
		}
	}
	return f
}

func cmdPre(o *opciones) {
	if len(o.resto) < 2 {
		fallo("pre: faltan argumentos")
	}
	src, dst, flags := o.resto[0], o.resto[1], o.resto[2:]
	h := buscar(o)
	tmp, err := os.MkdirTemp("", "picbuild")
	if err != nil {
		fallo("%v", err)
	}
	defer os.RemoveAll(tmp)
	c := filepath.Join(tmp, "pre.c")
	prepararC(src, c)
	var buf bytes.Buffer
	var args []string
	switch o.tc {
	case "xc8":
		args = append(baseXC8(o, h), "-E")
		args = append(args, flagsXC8(flags)...)
		args = append(args, c)
	default:
		args = append(append([]string{}, h.extra...), "-E", "-x", "c")
		args = append(args, flags...)
		args = append(args, c)
	}
	err = correr(o, tmp, &buf, h.cc, args...)
	if dst != "" && dst != "nul" && dst != "NUL" && dst != os.DevNull {
		os.WriteFile(dst, buf.Bytes(), 0644)
	}
	salirSi(err)
}

func cmdCC(o *opciones) {
	if len(o.resto) < 2 {
		fallo("cc: faltan argumentos")
	}
	src, obj, flags := o.resto[0], o.resto[1], o.resto[2:]
	h := buscar(o)
	ext := strings.ToLower(filepath.Ext(src))
	switch o.tc {
	case "xc8":
		if ext == ".s" {
			// el núcleo no usa ensamblador en 8 bits: objeto vacío
			vacio := obj + ".c"
			os.WriteFile(vacio, []byte("/* vacio */\n"), 0644)
			src, ext = vacio, ".c"
		}
		c := obj + ".c"
		if filepath.Clean(src) != filepath.Clean(c) {
			prepararC(src, c)
		}
		p1 := strings.TrimSuffix(c, ".c") + ".p1"
		args := append(baseXC8(o, h), "-c")
		args = append(args, flagsXC8(flags)...)
		args = append(args, "-o", p1, c)
		salirSi(correr(o, filepath.Dir(obj), os.Stdout, h.cc, args...))
		// el IDE espera el objeto con su nombre (.o): es el p-code de XC8
		os.Remove(obj)
		if err := os.Rename(p1, obj); err != nil {
			fallo("%v", err)
		}
		os.Remove(strings.TrimSuffix(c, ".c") + ".d")
	default:
		args := append([]string{}, h.extra...)
		args = append(args, "-c")
		fuente := src
		if ext == ".cpp" || ext == ".ino" {
			// el sketch y las bibliotecas se compilan como C
			c := obj + ".c"
			prepararC(src, c)
			fuente = c
		}
		args = append(args, flags...)
		args = append(args, fuente, "-o", obj)
		salirSi(correr(o, filepath.Dir(obj), os.Stdout, h.cc, args...))
	}
}

func cmdAr(o *opciones) {
	if len(o.resto) < 2 {
		fallo("ar: faltan argumentos")
	}
	arch, obj := o.resto[0], o.resto[1]
	h := buscar(o)
	if o.tc != "xc8" {
		salirSi(correr(o, "", os.Stdout, h.ar, "rcs", arch, obj))
		return
	}
	// xc8-ar necesita miembros .p1
	tmp, err := os.MkdirTemp("", "picbuild")
	if err != nil {
		fallo("%v", err)
	}
	defer os.RemoveAll(tmp)
	p1 := filepath.Join(tmp, strings.TrimSuffix(filepath.Base(obj), ".o")+".p1")
	b, err := os.ReadFile(obj)
	if err != nil {
		fallo("%v", err)
	}
	os.WriteFile(p1, b, 0644)
	salirSi(correr(o, tmp, os.Stdout, h.ar, "-r", arch, p1))
}

var (
	reProg = regexp.MustCompile(`(?i)Program space\s+used\s+[0-9A-F]+h\s*\(\s*(\d+)\)\s+of\s+[0-9A-F]+h\s+(words|bytes)`)
	reData = regexp.MustCompile(`(?i)Data space\s+used\s+[0-9A-F]+h\s*\(\s*(\d+)\)`)
)

func cmdLink(o *opciones) {
	// link BASE [flags...] -- objetos... archivo.a
	sep := -1
	for i, a := range o.resto {
		if a == "--" {
			sep = i
			break
		}
	}
	if sep < 1 {
		fallo("link: faltan argumentos")
	}
	base, flags, objs := o.resto[0], o.resto[1:sep], o.resto[sep+1:]
	h := buscar(o)
	dir := filepath.Dir(base)
	switch o.tc {
	case "xc8":
		tmp := filepath.Join(dir, "p1")
		os.RemoveAll(tmp)
		os.MkdirAll(tmp, 0755)
		args := append(baseXC8(o, h), flagsXC8(flags)...)
		args = append(args, "-msummary=+mem,-psect,-class,-hex,-file", "-Wl,-Map="+base+".map", "-o", base+".elf")
		for i, f := range objs {
			if strings.HasSuffix(strings.ToLower(f), ".a") {
				if existe(f) {
					args = append(args, f)
				}
				continue
			}
			b, err := os.ReadFile(f)
			if err != nil {
				fallo("%v", err)
			}
			p1 := filepath.Join(tmp, fmt.Sprintf("%03d_%s.p1", i, strings.TrimSuffix(filepath.Base(f), ".o")))
			os.WriteFile(p1, b, 0644)
			args = append(args, p1)
		}
		var buf bytes.Buffer
		err := correr(o, dir, io.MultiWriter(os.Stdout, &buf), h.cc, args...)
		salirSi(err)
		prog, data := "0", "0"
		if m := reProg.FindStringSubmatch(buf.String()); m != nil {
			prog = m[1]
		}
		if m := reData.FindStringSubmatch(buf.String()); m != nil {
			data = m[1]
		}
		os.WriteFile(base+".size", []byte("FLASH "+prog+"\nRAM "+data+"\n"), 0644)
	case "xc16":
		args := append([]string{}, flags...)
		args = append(args, "-o", base+".elf")
		args = append(args, objs...)
		args = append(args, "-Wl,-Map="+base+".map,--report-mem")
		var buf bytes.Buffer
		err := correr(o, dir, &buf, h.cc, args...)
		if err != nil || o.verb {
			os.Stdout.Write(buf.Bytes())
		}
		salirSi(err)
		os.WriteFile(base+".mem", buf.Bytes(), 0644)
		salirSi(correr(o, dir, os.Stdout, h.hex, base+".elf", "-a", "-omf=elf"))
	case "xc32":
		args := append([]string{}, h.extra...)
		args = append(args, flags...)
		args = append(args, "-o", base+".elf")
		args = append(args, objs...)
		args = append(args, "-Wl,-Map="+base+".map")
		salirSi(correr(o, dir, os.Stdout, h.cc, args...))
		salirSi(correr(o, dir, os.Stdout, h.hex, base+".elf"))
	default: // arm, avr
		args := append([]string{}, flags...)
		args = append(args, "-o", base+".elf")
		args = append(args, objs...)
		args = append(args, "-Wl,-Map="+base+".map")
		salirSi(correr(o, dir, os.Stdout, h.cc, args...))
		if o.tc == "avr" {
			salirSi(correr(o, dir, os.Stdout, h.hex, "-O", "ihex", "-R", ".eeprom", "-R", ".fuse", "-R", ".lock", "-R", ".signature", base+".elf", base+".hex"))
		} else {
			salirSi(correr(o, dir, os.Stdout, h.hex, "-O", "ihex", base+".elf", base+".hex"))
		}
	}
}

func cmdSize(o *opciones) {
	if len(o.resto) < 1 {
		fallo("size: faltan argumentos")
	}
	base := o.resto[0]
	if o.tc == "xc8" {
		b, _ := os.ReadFile(base + ".size")
		os.Stdout.Write(b)
		return
	}
	h := buscar(o)
	if o.tc == "xc16" {
		// informe de memoria del enlazador (--report-mem)
		b, _ := os.ReadFile(base + ".mem")
		prog, data := "0", "0"
		if m := regexp.MustCompile(`(?m)Total program memory used \(bytes\):\s+0x[0-9a-fA-F]+\s+\((\d+)\)`).FindSubmatch(b); m != nil {
			prog = string(m[1])
		}
		if m := regexp.MustCompile(`(?m)Total data memory used \(bytes\):\s+0x[0-9a-fA-F]+\s+\((\d+)\)`).FindSubmatch(b); m != nil {
			data = string(m[1])
		}
		fmt.Printf("FLASH %s\nRAM %s\n", prog, data)
		return
	}
	var buf bytes.Buffer
	salirSi(correr(o, "", &buf, h.size, "-A", base+".elf"))
	var flash, ram int
	for _, l := range strings.Split(buf.String(), "\n") {
		f := strings.Fields(l)
		if len(f) < 3 {
			continue
		}
		n, err := strconv.Atoi(f[1])
		if err != nil {
			continue
		}
		s := f[0]
		switch {
		case s == ".data" || strings.HasPrefix(s, ".sdata") || s == ".relocate":
			flash += n
			ram += n
		case s == ".bss" || strings.HasPrefix(s, ".sbss") || s == ".noinit":
			ram += n
		case s == ".text" || strings.HasPrefix(s, ".rodata") || strings.HasPrefix(s, ".reset") ||
			strings.HasPrefix(s, ".vector") || strings.HasPrefix(s, ".app_excpt") || strings.HasPrefix(s, ".bev_excpt") ||
			s == ".startup" || s == ".init" || s == ".fini" || s == ".ctors" || s == ".dtors" || s == ".dinit" ||
			s == ".ARM.exidx" || strings.HasPrefix(s, ".config"):
			flash += n
		}
	}
	fmt.Printf("FLASH %d\nRAM %d\n", flash, ram)
}

func cmdDonde(o *opciones) {
	for _, t := range []string{"xc8", "xc16", "xc32"} {
		v := versiones(t)
		if t == "xc16" {
			v = append(v, versiones("xc-dsc")...)
		}
		fmt.Printf("%-5s %v\n", t, v)
	}
	fmt.Println("MPLABX", versiones("MPLABX"))
	fmt.Println("ipecmd", ipecmd(o))
	fmt.Println("CURIOSITY", unidadesCuriosity())
}

func main() {
	if len(os.Args) < 2 {
		fmt.Fprintln(os.Stderr, "uso: picbuild pre|cc|ar|link|size|upload|donde -tc xc8|xc16|xc32|arm|avr -mcu CHIP ...")
		os.Exit(2)
	}
	o := &opciones{}
	a := os.Args[2:]
	i := 0
	for i < len(a) {
		toma := func() string {
			if i+1 >= len(a) {
				fallo("falta el valor de %s", a[i])
			}
			i += 2
			return a[i-1]
		}
		// forma -clave=valor (el valor puede venir vacío)
		if k, v, ok := strings.Cut(a[i], "="); ok && (k == "-xc" || k == "-dfp" || k == "-ipe" || k == "-power" || k == "-gcc") {
			switch k {
			case "-xc":
				o.xc = v
			case "-dfp":
				o.dfp = v
			case "-ipe":
				o.ipe = v
			case "-power":
				o.power = v
			case "-gcc":
				o.gcc = v
			}
			i++
			continue
		}
		switch a[i] {
		case "-tc":
			o.tc = toma()
		case "-mcu":
			o.mcu = toma()
		case "-xc":
			o.xc = toma()
		case "-gcc":
			o.gcc = toma()
		case "-dfp":
			o.dfp = toma()
		case "-ipe":
			o.ipe = toma()
		case "-tool":
			o.tool = toma()
		case "-power":
			o.power = toma()
		case "-v":
			o.verb = true
			i++
		default:
			o.resto = a[i:]
			i = len(a)
		}
	}
	switch os.Args[1] {
	case "pre":
		cmdPre(o)
	case "cc":
		cmdCC(o)
	case "ar":
		cmdAr(o)
	case "link":
		cmdLink(o)
	case "size":
		cmdSize(o)
	case "upload":
		cmdUpload(o)
	case "donde":
		cmdDonde(o)
	case "nada":
	default:
		fallo("orden desconocida: %s", os.Args[1])
	}
}
