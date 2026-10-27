#!/usr/bin/env bash
# Самопроверка стенда: эталон должен проходить чисто, плохие примеры — давать все ожидаемые сообщения.
HERE="$(cd "$(dirname "$0")" && pwd)"
CHECK="$HERE/../check.sh"
FAIL=0
out="$(SRC="$HERE/good" "$CHECK" 2>&1)"
if echo "$out" | grep -q 'ИТОГ: чисто'; then echo "[ок] эталон проходит чисто"; else echo "[СБОЙ] эталон не прошёл:"; echo "$out" | tail -30; FAIL=1; fi
out="$(SRC="$HERE/bad" "$CHECK" 2>&1)"
total=0; miss=0
while IFS= read -r exp; do
	total=$((total+1))
	if ! grep -qF -- "$exp" <<<"$out"; then echo "[СБОЙ] не найдено: $exp"; miss=$((miss+1)); fi
done < <(grep -rhoP '//\s*ОЖИДАЕТСЯ:\s*\K.*' "$HERE/bad" | sed 's/[[:space:]]*$//')
while IFS= read -r exp; do
	total=$((total+1))
	if grep -qF -- "$exp" <<<"$out"; then echo "[СБОЙ] ложное срабатывание: $exp"; miss=$((miss+1)); fi
done < <(grep -rhoP '//\s*НЕ_ОЖИДАЕТСЯ:\s*\K.*' "$HERE/bad" | sed 's/[[:space:]]*$//')
echo "[$([ $miss -eq 0 ] && echo ок || echo СБОЙ)] плохие примеры: поймано $((total-miss)) из $total"
[ $miss -eq 0 ] || FAIL=1
[ "${VERBOSE:-0}" = 1 ] && echo "$out"
exit $FAIL
