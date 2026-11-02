#!/bin/bash
# Тени Парижа — тесты библиотеки архитектурных элементов и превью фасадов в PNG (рендер Lutèce без окна, EGL/Mesa).
# Запуск: Tools/CityTest/run.sh [каталог_вывода]   (по умолчанию Tools/CityTest/out)
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"; ROOT="$HERE/../.."; L="$ROOT/legacy/lutece"; SRC="$ROOT/Source/TeniParizha/Private"
OUT="${1:-$HERE/out}"; mkdir -p "$OUT"
CXX="g++ -std=c++14 -O1 -Wall -Wextra -Wshadow -Werror -I$HERE/shim -I$SRC"
$CXX "$HERE/elem_test.cpp" "$SRC/TPCityBuilder.cpp" "$SRC/TPCityElements.cpp" -o "$OUT/elem_test"
"$OUT/elem_test"
$CXX "$HERE/dump_scene.cpp" "$SRC/TPCityBuilder.cpp" "$SRC/TPCityElements.cpp" -o "$OUT/dump_scene"
"$OUT/dump_scene" "$OUT/row.bin" row > /dev/null
"$OUT/dump_scene" "$OUT/level.bin" level > /dev/null
gcc -O1 -DLU_HEADLESS -I"$L" "$HERE/render_city.c" "$L"/core/lu_core.c "$L"/gfx/lu_gfx.c "$L"/gfx/lu_gl.c "$L"/gfx/lu_mesh.c "$L"/gfx/lu_font_data.c \
    "$L"/gfx/lu_texgen.c "$L"/world/lu_city.c "$L"/phys/lu_phys.c "$L"/platform/lu_platform_headless.c -ldl -lm -o "$OUT/render_city" 2>&1 | grep -v -i warning || true
for V in street elev win door shop corner roof; do "$OUT/render_city" "$OUT/row.bin" "$OUT/facade_$V.ppm" $V > /dev/null; done
"$OUT/render_city" "$OUT/level.bin" "$OUT/city_level.ppm" level > /dev/null
for f in "$OUT"/*.ppm; do python3 -c "import sys; from PIL import Image; Image.open(sys.argv[1]).save(sys.argv[1][:-4] + '.png')" "$f" && rm -f "$f"; done
echo "превью: $OUT/facade_*.png, $OUT/city_level.png"
