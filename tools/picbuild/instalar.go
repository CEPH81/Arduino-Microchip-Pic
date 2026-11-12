package main

// Instalación asistida de los compiladores de Microchip.
//
// Los compiladores XC son gratis, pero su licencia solo permite instalarlos
// y usarlos, no redistribuirlos: no pueden venir dentro del paquete del
// Gestor de placas. Cuando falta el que pide el chip elegido, picbuild
// descarga el instalador oficial desde microchip.com a la carpeta de
// Descargas del usuario y lo abre; el usuario acepta ahí la licencia de
// Microchip. Al terminar basta con volver a compilar.
//
// Se desactiva con  picbuild.autoinstalar=no  en platform.local.txt.

import (
	"fmt"
	"io"
	"net/http"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"strings"
	"time"
)

const baseMicrochip = "https://ww1.microchip.com/downloads/aemDocuments/documents/DEV/ProductDocuments/SoftwareTools/"
const paginaMPLABX = "https://www.microchip.com/en-us/tools-resources/archives/mplab-ecosystem"

type instalador struct {
	nombre  string            // "MPLAB XC8 v2.50"
	archivo map[string]string // por sistema: windows, linux, darwin
	mb      int               // tamaño aproximado de la descarga
}

// Versiones con las que se compila el núcleo en cada cambio (GitHub Actions)
var instaladores = map[string]instalador{
	"xc8": {"MPLAB XC8 v2.50", map[string]string{
		"windows": "xc8-v2.50-full-install-windows-x64-installer.exe",
		"linux":   "xc8-v2.50-full-install-linux-x64-installer.run",
		"darwin":  "xc8-v2.50-full-install-macos-x64-installer.dmg"}, 85},
	"xc16": {"MPLAB XC16 v2.10", map[string]string{
		"windows": "xc16-v2.10-full-install-windows64-installer.exe",
		"linux":   "xc16-v2.10-full-install-linux64-installer.run",
		"darwin":  "xc16-v2.10-full-install-osx64-installer.dmg"}, 125},
	"xc32": {"MPLAB XC32 v5.00", map[string]string{
		"windows": "xc32-v5.00-full-install-windows-x64-installer.exe",
		"linux":   "xc32-v5.00-full-install-linux-x64-installer.run"}, 1100},
}

func carpetaDescargas() string {
	if h, err := os.UserHomeDir(); err == nil {
		for _, n := range []string{"Downloads", "Descargas"} {
			if d := filepath.Join(h, n); existe(d) {
				return d
			}
		}
	}
	return os.TempDir()
}

// Descarga url a dst (continúa una descarga a medias). Muestra el avance.
func descargar(url, dst string) error {
	parte := dst + ".part"
	var desde int64
	if st, err := os.Stat(parte); err == nil {
		desde = st.Size()
	}
	req, err := http.NewRequest("GET", url, nil)
	if err != nil {
		return err
	}
	req.Header.Set("User-Agent", "picbuild (Arduino-Microchip-Pic)")
	if desde > 0 {
		req.Header.Set("Range", fmt.Sprintf("bytes=%d-", desde))
	}
	resp, err := (&http.Client{}).Do(req)
	if err != nil {
		return err
	}
	defer resp.Body.Close()
	modo := os.O_CREATE | os.O_WRONLY
	switch resp.StatusCode {
	case http.StatusPartialContent:
		modo |= os.O_APPEND
	case http.StatusOK:
		modo |= os.O_TRUNC
		desde = 0
	default:
		return fmt.Errorf("el servidor de Microchip respondió %s", resp.Status)
	}
	total := desde + resp.ContentLength
	f, err := os.OpenFile(parte, modo, 0644)
	if err != nil {
		return err
	}
	buf := make([]byte, 256*1024)
	hecho, aviso := desde, time.Now()
	for {
		n, er := resp.Body.Read(buf)
		if n > 0 {
			if _, ew := f.Write(buf[:n]); ew != nil {
				f.Close()
				return ew
			}
			hecho += int64(n)
			if time.Since(aviso) > 3*time.Second {
				aviso = time.Now()
				if resp.ContentLength > 0 {
					fmt.Fprintf(os.Stderr, "  %d de %d MB (%d%%)\n", hecho>>20, total>>20, hecho*100/total)
				} else {
					fmt.Fprintf(os.Stderr, "  %d MB\n", hecho>>20)
				}
			}
		}
		if er == io.EOF {
			break
		}
		if er != nil {
			f.Close()
			return fmt.Errorf("se cortó la descarga (%v); vuelva a compilar para continuarla", er)
		}
	}
	if err := f.Close(); err != nil {
		return err
	}
	if resp.ContentLength > 0 && hecho != total {
		return fmt.Errorf("descarga incompleta (%d de %d bytes); vuelva a compilar para continuarla", hecho, total)
	}
	return os.Rename(parte, dst)
}

// Abre un archivo o una página con el programa del sistema, sin esperar
func abrir(ruta string) error {
	var c *exec.Cmd
	switch runtime.GOOS {
	case "windows":
		c = exec.Command("cmd", "/c", "start", "", ruta)
	case "darwin":
		c = exec.Command("open", ruta)
	default:
		c = exec.Command("xdg-open", ruta)
	}
	return c.Start()
}

// Abre el instalador descargado. En Linux hace falta ser administrador.
func lanzar(ruta string) (string, error) {
	switch runtime.GOOS {
	case "linux":
		os.Chmod(ruta, 0755)
		if p, err := exec.LookPath("pkexec"); err == nil && (os.Getenv("DISPLAY") != "" || os.Getenv("WAYLAND_DISPLAY") != "") {
			c := exec.Command(p, "env", "DISPLAY="+os.Getenv("DISPLAY"), "XAUTHORITY="+os.Getenv("XAUTHORITY"), ruta)
			return "", c.Start()
		}
		return "Ábralo desde una terminal:\n    sudo \"" + ruta + "\"", nil
	default:
		return "", abrir(ruta)
	}
}

func autoinstalar(o *opciones) bool {
	v := strings.ToLower(prop(o.auto))
	return v != "no" && v != "0" && v != "false" && os.Getenv("PICBUILD_AUTOINSTALAR") != "no"
}

// Falta el compilador de 'tc': lo descarga de Microchip, abre el instalador
// y termina con las instrucciones. 'motivo' explica qué falta.
func instalarCompilador(o *opciones, tc, motivo, manual string) {
	ins, ok := instaladores[tc]
	arch := ins.archivo[runtime.GOOS]
	if !ok || arch == "" || !autoinstalar(o) {
		fallo("%s\n  %s", motivo, manual)
	}
	dst := filepath.Join(carpetaDescargas(), arch)
	marca := dst + ".abierto"
	if st, err := os.Stat(marca); err == nil && time.Since(st.ModTime()) < 20*time.Minute && existe(dst) {
		fallo("%s\n  El instalador de %s ya se abrió hace %d min. Termine la instalación (opciones por\n  defecto, licencia \"Free\") y vuelva a compilar. Si lo cerró, ábralo de nuevo:\n    %s",
			motivo, ins.nombre, int(time.Since(st.ModTime()).Minutes()), dst)
	}
	if st, err := os.Stat(dst + ".part"); err == nil && time.Since(st.ModTime()) < 15*time.Second {
		fallo("%s\n  Ya hay una descarga de %s en curso; espere a que termine.", motivo, ins.nombre)
	}
	fmt.Fprintf(os.Stderr, "picbuild: %s\n", motivo)
	if !existe(dst) {
		fmt.Fprintf(os.Stderr, "  Descargando %s desde microchip.com (unos %d MB) a:\n    %s\n", ins.nombre, ins.mb, dst)
		base := baseMicrochip
		if b := os.Getenv("PICBUILD_DESCARGAS"); b != "" { // otro servidor (pruebas)
			base = b
		}
		if err := descargar(base+arch, dst); err != nil {
			fallo("no se pudo descargar %s: %v\n  %s", ins.nombre, err, manual)
		}
	}
	nota, err := lanzar(dst)
	if err != nil {
		fallo("no se pudo abrir el instalador (%v). Ábralo a mano:\n    %s", err, dst)
	}
	os.WriteFile(marca, []byte(time.Now().Format(time.RFC3339)+"\n"), 0644)
	msg := "Se abrió el instalador oficial de " + ins.nombre + "."
	if nota != "" {
		msg = "El instalador oficial de " + ins.nombre + " está en:\n    " + dst + "\n  " + nota
	}
	fallo("falta instalar el compilador (solo la primera vez).\n  %s\n  Complete la instalación con las opciones por defecto (tipo de licencia: \"Free\"; desde julio de 2026\n  incluye todas las optimizaciones) y vuelva a pulsar Verificar o Subir.", msg)
}

// Falta ipecmd: abre la página de descargas de MPLAB X
func instalarIPE(o *opciones) {
	base := "no encuentro ipecmd (MPLAB IPE), que es el programa de Microchip que maneja el PICkit.\n" +
		"  Instale MPLAB X IDE con MPLAB IPE. Para el PICkit 3 use la v6.20 o anterior (las nuevas ya no lo traen).\n" +
		"  Si lo tiene en otra carpeta, indíquela en platform.local.txt:\n" +
		"    picbuild.ipe=C:/Program Files/Microchip/MPLABX/v6.20/mplab_platform/mplab_ipe/ipecmd.exe"
	if autoinstalar(o) && abrir(paginaMPLABX) == nil {
		fallo("%s\n  Se abrió en el navegador la página de descargas de Microchip:\n    %s", base, paginaMPLABX)
	}
	fallo("%s\n  Descargas: %s", base, paginaMPLABX)
}
