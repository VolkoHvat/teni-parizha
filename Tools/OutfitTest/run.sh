#!/bin/bash
# Тени Парижа — тесты построителя одежды и превью в PNG (рендер Lutèce без окна, EGL/Mesa).
# Запуск: Tools/OutfitTest/run.sh [каталог_вывода]   (по умолчанию Tools/OutfitTest/out)
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"; ROOT="$HERE/../.."; L="$ROOT/legacy/lutece"
OUT="${1:-$HERE/out}"; mkdir -p "$OUT"
g++ -std=c++14 -O1 -Wall -Wextra -Wshadow -Werror -I"$HERE/shim" -I"$ROOT/Source/TeniParizha/Public" "$HERE/outfit_test.cpp" -o "$OUT/outfit_test"
"$OUT/outfit_test"
"$OUT/outfit_test" --dump "$OUT"
gcc -O1 -DLU_HEADLESS -I"$L" "$HERE/render_outfit.c" "$L"/core/lu_core.c "$L"/gfx/lu_gfx.c "$L"/gfx/lu_gl.c "$L"/gfx/lu_mesh.c \
    "$L"/gfx/lu_font_data.c "$L"/gfx/lu_texgen.c "$L"/platform/lu_platform_headless.c -ldl -lm -o "$OUT/render_outfit" 2>&1 | grep -v -i warning || true
shot() {   # сцена, имя картинки, камера (ex ey ez tx ty tz fovy) [w h]
    "$OUT/render_outfit" "$OUT/$1.bin" "$OUT/$1.txt" "$OUT/$2.ppm" "${@:3}"
    python3 -c "
import sys, os; from PIL import Image; p = sys.argv[1]; Image.open(p + '.ppm').save(p + '.png'); os.remove(p + '.ppm')" "$OUT/$2"
}
shot lineup outfit_front 1.6 1.3 7.8 0 1.0 0 .52
shot lineup outfit_back -1.6 1.5 -7.8 0 1.0 0 .52
shot lineup outfit_side 9 1.2 0.4 0 1.0 0 .5
shot lineup outfit_heads 1.3 1.75 3.2 0 1.55 0 .5 2000 900
shot poses_hero outfit_poses_hero 1.6 1.4 12.5 0 .9 0 .42 2400 1000
shot side_hero outfit_side_hero 9 1.0 0 0 .9 0 .6 2400 1000
shot side_guard outfit_side_guard 9 1.0 0 0 .9 0 .6 2400 1000
shot poses_guard outfit_poses_guard 1.6 1.4 12.5 0 .9 0 .42 2400 1000
echo "превью: $OUT/outfit_*.png"
