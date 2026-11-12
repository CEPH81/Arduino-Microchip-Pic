package main

// Carga del .hex: ipecmd (MPLAB IPE) con PICkit 3/4/5, ICD, SNAP o PKOB, o
// copia a la unidad CURIOSITY de las placas Curiosity Nano.

import (
	"fmt"
	"io"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"strings"
)

// ipecmd de la instalación de MPLAB X / IPE más nueva (o la indicada)
func ipecmd(o *opciones) string {
	if p := prop(o.ipe); p != "" {
		return p
	}
	if p := os.Getenv("IPECMD_PATH"); p != "" {
		return p
	}
	nombres := []string{"ipecmd.sh", "ipecmd.jar"}
	if runtime.GOOS == "windows" {
		nombres = []string{"ipecmd.exe", "ipecmd.jar"}
	}
	for _, x := range versiones("MPLABX") {
		for _, sub := range []string{"mplab_platform/mplab_ipe", "mplab_ipe"} {
			for _, n := range nombres {
				p := filepath.Join(x, filepath.FromSlash(sub), n)
				if existe(p) {
					return p
				}
			}
		}
	}
	// macOS: /Applications/microchip/mplabx/vX.XX/mplab_ipe.app/...
	m, _ := filepath.Glob("/Applications/microchip/mplabx/v*/mplab_platform/mplab_ipe/ipecmd.sh")
	if len(m) > 0 {
		return m[len(m)-1]
	}
	return ""
}

// unidades con nombre CURIOSITY (traen el archivo KIT-INFO.HTM)
func unidadesCuriosity() []string {
	var cand []string
	switch runtime.GOOS {
	case "windows":
		for c := 'D'; c <= 'Z'; c++ {
			cand = append(cand, string(c)+`:\`)
		}
	case "darwin":
		cand, _ = filepath.Glob("/Volumes/CURIOSITY*")
	default:
		u := os.Getenv("USER")
		for _, p := range []string{"/media/" + u + "/CURIOSITY*", "/run/media/" + u + "/CURIOSITY*", "/media/CURIOSITY*", "/mnt/CURIOSITY*"} {
			m, _ := filepath.Glob(p)
			cand = append(cand, m...)
		}
	}
	var r []string
	for _, d := range cand {
		if existe(filepath.Join(d, "KIT-INFO.HTM")) || existe(filepath.Join(d, "KIT-INFO.htm")) ||
			(existe(filepath.Join(d, "STATUS.TXT")) && existe(filepath.Join(d, "PUBKEY.TXT"))) {
			r = append(r, d)
		}
	}
	return r
}

func copiar(src, dst string) error {
	in, err := os.Open(src)
	if err != nil {
		return err
	}
	defer in.Close()
	out, err := os.Create(dst)
	if err != nil {
		return err
	}
	if _, err := io.Copy(out, in); err != nil {
		out.Close()
		return err
	}
	if err := out.Sync(); err != nil {
		out.Close()
		return err
	}
	return out.Close()
}

// código de herramienta de ipecmd (-TP...)
var herramientasIPE = map[string]string{
	"PK3": "PK3", "PK4": "PK4", "PK5": "PK5", "ICD3": "ICD3", "ICD4": "ICD4", "ICD5": "ICD5",
	"SNAP": "SNAP", "PKOB": "PKOB", "PKOB4": "PKOB4", "ICE4": "ICE4", "RICE": "RICE",
}

func cmdUpload(o *opciones) {
	if len(o.resto) < 1 {
		fallo("upload: falta el archivo .hex")
	}
	hex := o.resto[0]
	if !existe(hex) {
		fallo("no existe %s (compile primero)", hex)
	}
	tool := strings.ToUpper(prop(o.tool))
	if tool == "" {
		tool = "PK3"
	}

	if tool == "CURIOSITY" {
		u := unidadesCuriosity()
		if len(u) == 0 {
			fallo("no encuentro la unidad CURIOSITY.\n  Conecte la placa Curiosity por su USB de depuración y espere a que aparezca la unidad;\n  o elija otro programador en Herramientas > Programador.")
		}
		dst := filepath.Join(u[0], "sketch.hex")
		fmt.Printf("Copiando el programa a %s ...\n", u[0])
		if err := copiar(hex, dst); err != nil {
			fallo("no se pudo copiar a %s: %v", u[0], err)
		}
		fmt.Println("Listo: la placa se programa sola al terminar la copia (vea STATUS.TXT en la unidad si no arranca).")
		return
	}

	cod, ok := herramientasIPE[tool]
	if !ok {
		fallo("programador desconocido: %s", tool)
	}
	ipe := ipecmd(o)
	if ipe == "" {
		instalarIPE(o)
	}
	// ipecmd deja sus registros (MPLABXLog.xml...) en la carpeta de trabajo
	tmp, err := os.MkdirTemp("", "picbuild-ipe")
	if err != nil {
		fallo("%v", err)
	}
	defer os.RemoveAll(tmp)

	args := []string{"-TP" + cod, "-P" + o.mcu, "-F" + hex, "-M", "-OL"}
	if p := prop(o.power); p != "" && p != "0" && strings.ToLower(p) != "no" {
		args = append(args, "-W"+p) // el programador alimenta la placa
	}
	prog := ipe
	if strings.HasSuffix(strings.ToLower(ipe), ".jar") {
		java := "java"
		// Java que trae MPLAB X
		raiz := filepath.Dir(filepath.Dir(filepath.Dir(ipe)))
		for _, pat := range []string{"sys/java/*/bin/java*", "sys/java/*/jre/bin/java*"} {
			if m, _ := filepath.Glob(filepath.Join(raiz, filepath.FromSlash(pat))); len(m) > 0 {
				java = m[0]
				break
			}
		}
		args = append([]string{"-jar", ipe}, args...)
		prog = java
	}
	fmt.Printf("Programando PIC%s con %s (ipecmd)...\n", o.mcu, tool)
	if o.verb {
		fmt.Fprintln(os.Stderr, "picbuild>", prog, strings.Join(args, " "))
	}
	c := exec.Command(prog, args...)
	c.Dir = tmp
	c.Stdout = os.Stdout
	c.Stderr = os.Stderr
	if err := c.Run(); err != nil {
		fmt.Fprintln(os.Stderr, "picbuild: ipecmd terminó con error. Revise: programador conectado, placa alimentada (o active\n  \"Alimentar desde el programador\"), chip correcto en el menú, y que MPLAB X/IPE no esté usando el programador.")
		salirSi(err)
	}
	fmt.Println("Listo.")
}
