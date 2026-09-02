#!/bin/sh

set -u

# === Цвета ===
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
RESET='\033[0m'

# === Подготовка ===
TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

A="$TMP_DIR/a.txt"
B="$TMP_DIR/b.txt"
PATTERNS="$TMP_DIR/patterns.txt"
EMPTY="$TMP_DIR/empty.txt"
MISSING="$TMP_DIR/missing.txt"

printf 'Alpha\nbeta\nALPHA beta\n\n123 alpha 456\nno newline' > "$A"
printf 'beta\nzzz\nAlpha\n' > "$B"
printf 'alpha\nbeta\n' > "$PATTERNS"
: > "$EMPTY"

PASS=0
FAIL=0
SKIP=0

# === Функция проверки ===
check() {
  ./s21_grep "$@" > "$TMP_DIR/ours" 2>/dev/null
  OUR_STATUS=$?

  busybox grep "$@" > "$TMP_DIR/ref" 2>/dev/null
  REF_STATUS=$?

  if [ "$REF_STATUS" -ge 128 ]; then
    SKIP=$((SKIP + 1))
    printf "${YELLOW}SKIP${RESET}  grep %s\n" "$*"
  elif cmp -s "$TMP_DIR/ours" "$TMP_DIR/ref" && [ "$OUR_STATUS" -eq "$REF_STATUS" ]; then
    PASS=$((PASS + 1))
    printf "${GREEN}PASS${RESET}  grep %s\n" "$*"
  else
    FAIL=$((FAIL + 1))
    printf "${RED}FAIL${RESET}  grep %s\n" "$*"
  fi
}

printf "\n${BOLD}${CYAN}=== s21_grep tests ===${RESET}\n\n"

# === Базовые тесты ===
printf "${BOLD}Basic tests:${RESET}\n"
check alpha "$A"
check alpha "$A" "$B"
printf "\n"

# === Одиночные флаги ===
printf "${BOLD}Single flags:${RESET}\n"
for flag in i v c l n h s o; do
  check "-$flag" alpha "$A"
  check "-$flag" alpha "$A" "$B"
done
printf "\n"

# === Пары флагов ===
printf "${BOLD}Flag combinations:${RESET}\n"
for pair in iv ic il in ih is io \
            vc vl vn vh vs \
            cl cn ch cs co \
            ln lh ls lo \
            nh ns no \
            hs ho so; do
  check "-$pair" alpha "$A" "$B"
done
printf "\n"

# === Специальный случай -vo ===
printf "${BOLD}Special cases:${RESET}\n"
./s21_grep -vo alpha "$A" > "$TMP_DIR/ours" 2>/dev/null
OUR_STATUS=$?
if [ ! -s "$TMP_DIR/ours" ] && [ "$OUR_STATUS" -eq 0 ]; then
  PASS=$((PASS + 1))
  printf "${GREEN}PASS${RESET}  grep -vo\n"
else
  FAIL=$((FAIL + 1))
  printf "${RED}FAIL${RESET}  grep -vo\n"
fi

# === Флаги -e / -f ===
check -e alpha "$A" "$B"
check -ealpha "$A" "$B"
check -i -e alpha "$A" "$B"
check -f "$PATTERNS" "$A" "$B"
check -if "$PATTERNS" "$A" "$B"
check -e alpha -f "$PATTERNS" "$A" "$B"
check -f "$EMPTY" "$A"
check -v -f "$EMPTY" "$A"
check "$A" -e alpha "$B"
check -on -e alpha -e beta "$A" "$B"
check -cl alpha "$A" "$B"
check -s alpha "$MISSING" "$A"

# === Итог ===
printf "\n${BOLD}────────────────────────────${RESET}\n"
printf "Result:  "
printf "${GREEN}%d passed${RESET}  " "$PASS"
printf "${RED}%d failed${RESET}  " "$FAIL"
printf "${YELLOW}%d skipped${RESET}\n" "$SKIP"
printf "${BOLD}────────────────────────────${RESET}\n\n"

[ "$FAIL" -eq 0 ]