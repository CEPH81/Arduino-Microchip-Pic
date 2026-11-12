#!/bin/bash
# Compila todos los ejemplos en todas las placas (runner de GitHub Actions).
# Uso: tools/ci_compilar.sh [placas...]   (sin argumentos: todas)
R=$(cd "$(dirname "$0")/.." && pwd)
H=$HOME/Arduino/hardware/CEPH81
mkdir -p "$H" && rm -rf "$H/pic" && ln -s "$R/pic" "$H/pic"
( cd "$R/tools/picbuild" && go build -o "$R/pic/tools/picbuild" . ) || exit 1
"$R/pic/tools/picbuild" donde
PLACAS=${*:-$(grep -o '^[a-z0-9]*\.name=' "$R/pic/boards.txt" | cut -d. -f1)}
SIN_SERIAL="pic16f84 pic16f84a pic16f872"
ok=0; mal=0; L=/tmp/pic-logs; mkdir -p $L
for b in $PLACAS; do
  for e in "$R"/pic/libraries/*/examples/*/; do
    n=$(basename "$e")
    case " $SIN_SERIAL " in *" $b "*) grep -q 'Serial' "$e/$n.ino" && continue ;; esac
    if arduino-cli compile --fqbn CEPH81:pic:$b --build-path /tmp/b/$b/$n "$e" > $L/$b-$n.log 2>&1; then
      ok=$((ok+1)); echo "OK   $b $n $(grep -o '[0-9]* bytes' $L/$b-$n.log | head -2 | tr '\n' ' ')"
    else
      mal=$((mal+1)); echo "FAIL $b $n"; echo "::group::$b $n"; tail -${LINEAS:-25} $L/$b-$n.log; echo "::endgroup::"
    fi
  done
done
echo "Resultado: $ok bien, $mal con error"
[ $mal -eq 0 ]
