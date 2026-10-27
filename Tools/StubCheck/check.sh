#!/usr/bin/env bash
# Стенд проверки «Тени Парижа»: синтаксис C++ против макета UE 4.27 без самого движка.
# Использование:
#   Tools/StubCheck/check.sh                 — все .cpp по отдельности + unity-сборка + линтер + AST-проверка
#   Tools/StubCheck/check.sh Foo.cpp Bar.cpp — только указанные файлы (+ линтер по ним)
#   Tools/StubCheck/check.sh --unity         — только unity-сборка
#   Tools/StubCheck/check.sh --lint          — только линтер (+ проверка конфигов)
#   Tools/StubCheck/check.sh --selftest      — самопроверка стенда на эталонных примерах
# Переменные: CXX (по умолчанию clang++), SRC (каталог модуля), JOBS.
set -u
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"
SRC="$(cd "${SRC:-$ROOT/Source/TeniParizha}" && pwd)"
CXX="${CXX:-clang++}"
JOBS="${JOBS:-$(nproc 2>/dev/null || echo 4)}"
OUT="${TP_OUT:-${TMPDIR:-/tmp}/tp_stubcheck_$$}"
mkdir -p "$OUT/gen"
trap 'rm -rf "$OUT"' EXIT

MODE=all; FILES=()
for a in "$@"; do
	case "$a" in
		--unity) MODE=unity ;;
		--lint) MODE=lint ;;
		--selftest) exec "$HERE/selftest/run.sh" ;;
		-h|--help) sed -n '2,9p' "$0"; exit 0 ;;
		*) FILES+=("$(cd "$(dirname "$a")" && pwd)/$(basename "$a")") ;;
	esac
done
[ ${#FILES[@]} -gt 0 ] && MODE=files

mapfile -t HEADERS < <(find "$SRC" -name '*.h' | sort)
mapfile -t ALLCPP < <(find "$SRC" -name '*.cpp' | sort)
python3 "$HERE/gen_generated.py" "$OUT/gen" "${HEADERS[@]}" || { echo "ОШИБКА: генерация .generated.h"; exit 2; }

INC=(-I"$HERE/ue" -I"$OUT/gen")
for d in Public Private Classes; do [ -d "$SRC/$d" ] && INC+=(-I"$SRC/$d"); done
INC+=(-I"$SRC")
# Флаги: C++14 (стандарт по умолчанию в UE 4.27), затенение имён — ошибка (в UE 4.27 с V2 это ошибка C4456–C4459).
FLAGS=(-std=c++14 -fshort-wchar -fsyntax-only -fno-color-diagnostics -ferror-limit=40 -DTENIPARIZHA_API= -DUE_BUILD_SHIPPING=0
	-Wall -Wno-unused-variable -Wno-unused-private-field -Wno-unused-function -Wno-unused-but-set-variable -Wno-unused-lambda-capture
	-Wno-missing-braces -Wno-null-dereference -Wno-inconsistent-missing-override -Wno-unknown-pragmas -Wno-unused-const-variable
	-Wno-deprecated-declarations -Wno-overloaded-virtual -Wno-invalid-offsetof -Wno-unused-value -Wno-delete-non-abstract-non-virtual-dtor
	-Werror=shadow -Werror=return-type -Werror=uninitialized -Werror=sometimes-uninitialized -Werror=dangling-else
	-Wshadow-field-in-constructor -Werror=shadow-field-in-constructor -Werror=sizeof-pointer-memaccess -Werror=dynamic-class-memaccess)

FAIL=0
compile_one() { # $1 — файл, $2 — WITH_EDITOR
	local f="$1" log
	log="$("$CXX" "${FLAGS[@]}" -DWITH_EDITOR="$2" "${INC[@]}" "$f" 2>&1)"
	local rc=$?
	if [ $rc -ne 0 ] || echo "$log" | grep -q 'warning:'; then
		echo "---- ${f#$ROOT/} (WITH_EDITOR=$2)"
		echo "$log" | grep -E 'error:|warning:|note: candidate|static_assert|required from|in instantiation' | sed "s#$ROOT/##g; s#$HERE/ue/#[макет UE] #g" | head -60
	fi
	return $rc
}

run_files() {
	local n=0 bad=0
	for f in "$@"; do
		n=$((n+1))
		compile_one "$f" 1 || bad=$((bad+1))
	done
	echo "== Отдельные файлы: $((n-bad))/$n без ошибок"
	[ $bad -eq 0 ]
}

run_unity() {
	local u="$OUT/Module.TeniParizha.cpp"
	: > "$u"
	for f in "${ALLCPP[@]}"; do echo "#include \"$f\"" >> "$u"; done
	if compile_one "$u" 0; then echo "== Unity-сборка (${#ALLCPP[@]} файлов, WITH_EDITOR=0): без ошибок"; return 0; fi
	echo "== Unity-сборка: ОШИБКИ"; return 1
}

run_ast() {
	python3 -c 'import clang.cindex' 2>/dev/null || pip install -q libclang==18.1.1 --break-system-packages >/dev/null 2>&1 || pip install -q libclang==18.1.1 >/dev/null 2>&1
	if ! python3 -c 'import clang.cindex' 2>/dev/null; then echo "== AST-проверка пропущена: нет python-модуля libclang"; return 0; fi
	python3 "$HERE/tp_ast.py" --root "$ROOT" --src "$SRC" -- "${FLAGS[@]}" -DWITH_EDITOR=1 "${INC[@]}" -- "$@"
}

run_lint() { python3 "$HERE/lint.py" --root "$ROOT" --src "$SRC" --stub "$HERE/ue" "$@"; }

case $MODE in
	all)
		run_files "${ALLCPP[@]}" || FAIL=1
		run_unity || FAIL=1
		run_ast "${ALLCPP[@]}" || FAIL=1
		run_lint || FAIL=1 ;;
	files)
		run_files "${FILES[@]}" || FAIL=1
		run_ast "${FILES[@]}" || FAIL=1
		run_lint "${FILES[@]}" || FAIL=1 ;;
	unity) run_unity || FAIL=1 ;;
	lint) run_lint || FAIL=1 ;;
esac
if [ $FAIL -eq 0 ]; then echo "ИТОГ: чисто"; else echo "ИТОГ: есть ошибки"; fi
exit $FAIL
