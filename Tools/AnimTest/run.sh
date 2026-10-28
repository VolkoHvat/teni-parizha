#!/bin/bash
# Тени Парижа — тесты математики поз и лист поз в PNG (рендер Lutèce без окна, EGL/Mesa).
# Запуск: Tools/AnimTest/run.sh [каталог_вывода]   (по умолчанию Tools/AnimTest/out)
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"; ROOT="$HERE/../.."; L="$ROOT/legacy/lutece"
OUT="${1:-$HERE/out}"; mkdir -p "$OUT"
g++ -std=c++14 -O1 -Wall -Wextra -Wshadow -Werror -I"$HERE/shim" -I"$ROOT/Source/TeniParizha/Public" "$HERE/anim_test.cpp" -o "$OUT/anim_test"
"$OUT/anim_test"
"$OUT/anim_test" --dump "$OUT/poses.txt"
gcc -O1 -DLU_HEADLESS -I"$L" "$HERE/render_sheet.c" "$L"/core/lu_core.c "$L"/gfx/lu_gfx.c "$L"/gfx/lu_gl.c "$L"/gfx/lu_mesh.c \
    "$L"/gfx/lu_font_data.c "$L"/gfx/lu_texgen.c "$L"/platform/lu_platform_headless.c -ldl -lm -o "$OUT/render_sheet" 2>&1 | grep -v -i warning || true
"$OUT/render_sheet" "$OUT/poses.txt" "$OUT/poses_side.ppm" side
"$OUT/render_sheet" "$OUT/poses.txt" "$OUT/poses_34.ppm" 34
for f in "$OUT"/poses_side "$OUT"/poses_34; do python3 -c "
import sys; from PIL import Image; Image.open(sys.argv[1]+'.ppm').save(sys.argv[1]+'.png')" "$f" && rm -f "$f.ppm"; done
echo "лист поз: $OUT/poses_side.png, $OUT/poses_34.png"
