#!/usr/bin/env python3
"""
make_release.py - Empaqueta el núcleo y actualiza el índice del Gestor de placas.

  python3 tools/make_release.py            -> versión de pic/platform.txt
  python3 tools/make_release.py 0.2.0      -> fija esa versión en platform.txt

Compila picbuild para Windows, Linux y macOS (hace falta Go), genera
dist/pic-<versión>.zip y añade (o reemplaza) esa versión en
package_ceph81_pic_index.json. Lo ejecuta .github/workflows/publicar.yml.

El paquete no trae compiladores: XC8, XC16/XC-DSC, XC32 y MPLAB IPE se
instalan aparte desde microchip.com (su licencia no permite redistribuirlos).
"""
import hashlib, json, os, re, subprocess, sys, zipfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
CORE = os.path.join(ROOT, "pic")
INDEX = os.path.join(ROOT, "package_ceph81_pic_index.json")
REPO = "https://github.com/CEPH81/Arduino-Microchip-Pic"
SKIP = {"platform.local.txt", "boards.local.txt"}
# (GOOS, GOARCH, nombre en pic/tools)
BINARIOS = [("linux", "amd64", "picbuild"), ("windows", "386", "picbuild.exe"), ("darwin", "amd64", "picbuild_mac")]
EJECUTABLES = {"picbuild", "picbuild_mac"}


def platform_version(new=None):
    p = os.path.join(CORE, "platform.txt")
    s = open(p, encoding="utf-8").read()
    if new:
        s = re.sub(r"^version=.*$", "version=" + new, s, flags=re.M)
        open(p, "w", encoding="utf-8", newline="\n").write(s)
    return re.search(r"^version=(.*)$", s, re.M).group(1).strip()


def board_names():
    names = []
    for line in open(os.path.join(CORE, "boards.txt"), encoding="utf-8"):
        m = re.match(r"^[\w]+\.name=(.*)$", line.strip())
        if m:
            names.append({"name": m.group(1)})
    return names


def build_tools():
    src = os.path.join(ROOT, "tools", "picbuild")
    os.makedirs(os.path.join(CORE, "tools"), exist_ok=True)
    for goos, goarch, name in BINARIOS:
        env = dict(os.environ, GOOS=goos, GOARCH=goarch, CGO_ENABLED="0")
        subprocess.check_call(["go", "build", "-trimpath", "-ldflags=-s -w -buildid=", "-o",
                               os.path.join(CORE, "tools", name), "."], cwd=src, env=env)


def build_zip(version):
    os.makedirs(os.path.join(ROOT, "dist"), exist_ok=True)
    name = "pic-%s.zip" % version
    path = os.path.join(ROOT, "dist", name)
    with zipfile.ZipFile(path, "w", zipfile.ZIP_DEFLATED) as z:
        for base, dirs, files in os.walk(CORE):
            dirs.sort()
            for f in sorted(files):
                if f in SKIP or f.endswith(".pyc"):
                    continue
                full = os.path.join(base, f)
                arc = os.path.join("pic-" + version, os.path.relpath(full, CORE))
                info = zipfile.ZipInfo(arc.replace(os.sep, "/"), date_time=(2026, 1, 1, 0, 0, 0))
                info.compress_type = zipfile.ZIP_DEFLATED
                info.external_attr = (0o755 if f in EJECUTABLES or f.endswith(".sh") else 0o644) << 16
                z.writestr(info, open(full, "rb").read())
    data = open(path, "rb").read()
    return name, path, "SHA-256:" + hashlib.sha256(data).hexdigest(), str(len(data))


def main():
    if len(sys.argv) > 1 and not re.fullmatch(r"\d+\.\d+\.\d+", sys.argv[1]):
        sys.exit("uso: make_release.py [X.Y.Z]")
    version = platform_version(sys.argv[1] if len(sys.argv) > 1 else None)
    build_tools()
    name, path, checksum, size = build_zip(version)

    if os.path.exists(INDEX):
        idx = json.load(open(INDEX, encoding="utf-8"))
    else:
        idx = {"packages": [{
            "name": "CEPH81", "maintainer": "CEPH81 (MikrocontrollerCephs)",
            "websiteURL": REPO, "email": "", "help": {"online": REPO + "/issues"},
            "platforms": [], "tools": []}]}
    pkg = idx["packages"][0]
    pkg["platforms"] = [p for p in pkg["platforms"] if p["version"] != version]
    pkg["platforms"].append({
        "name": "Microchip PIC (CEPH81)",
        "architecture": "pic",
        "version": version,
        "category": "Contributed",
        "help": {"online": REPO},
        "url": "%s/releases/download/v%s/%s" % (REPO, version, name),
        "archiveFileName": name,
        "checksum": checksum,
        "size": size,
        "boards": board_names(),
        "toolsDependencies": [],
    })
    pkg["platforms"].sort(key=lambda p: [int(x) for x in re.findall(r"\d+", p["version"])])
    with open(INDEX, "w", encoding="utf-8", newline="\n") as f:
        json.dump(idx, f, indent=2, ensure_ascii=False)
        f.write("\n")
    print("%s  %s  %s bytes" % (path, checksum, size))


if __name__ == "__main__":
    main()
