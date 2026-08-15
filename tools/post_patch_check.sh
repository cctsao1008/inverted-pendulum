#!/usr/bin/env bash

set -u

ROOT="$(git rev-parse --show-toplevel 2>/dev/null)" || {
    echo "[ERROR] Not inside a Git repository."
    exit 1
}

cd "$ROOT" || exit 1

HOST_BUILD="build/host"
STM32_BUILD="build/stm32"

echo
echo "============================================================"
echo " POST PATCH CHECK"
echo "============================================================"
echo

echo "=== TIME ==="
date --iso-8601=seconds
echo

echo "=== GIT HEAD ==="
git log -5 --oneline --decorate
echo

echo "=== GIT STATUS ==="
git status --short
if [ -z "$(git status --porcelain)" ]; then
    echo "WORKTREE: clean"
else
    echo "WORKTREE: DIRTY"
fi
echo

echo "=== GIT IDENTIFIERS ==="
echo "HEAD_FULL=$(git rev-parse HEAD)"
echo "HEAD_SHORT=$(git rev-parse --short=8 HEAD)"
echo "BRANCH=$(git branch --show-current)"
echo "ORIGIN_MAIN=$(git rev-parse --short=8 origin/main 2>/dev/null || echo unavailable)"
echo

echo "============================================================"
echo " HOST BUILD"
echo "============================================================"

if cmake --build "$HOST_BUILD"; then
    echo "[PASS] host build"
else
    echo "[FAIL] host build"
    exit 2
fi

echo
echo "=== HOST TESTS ==="

if ctest --test-dir "$HOST_BUILD" --output-on-failure; then
    echo "[PASS] host tests"
else
    echo "[FAIL] host tests"
    exit 3
fi

echo
echo "============================================================"
echo " STM32 CONFIGURE + BUILD"
echo "============================================================"

#
# Reconfigure intentionally:
# FW_GIT_DESCRIBE and build timestamp are generated at CMake configure time.
#
if cmake -S . -B "$STM32_BUILD"; then
    echo "[PASS] STM32 configure"
else
    echo "[FAIL] STM32 configure"
    exit 4
fi

if cmake --build "$STM32_BUILD"; then
    echo "[PASS] STM32 build"
else
    echo "[FAIL] STM32 build"
    exit 5
fi

echo
echo "============================================================"
echo " FIRMWARE ARTIFACTS"
echo "============================================================"

for f in \
    "$STM32_BUILD/inverted-pendulum.elf" \
    "$STM32_BUILD/inverted-pendulum.hex" \
    "$STM32_BUILD/inverted-pendulum.bin" \
    "$STM32_BUILD/inverted-pendulum.map"
do
    if [ -f "$f" ]; then
        ls -lh --time-style=long-iso "$f"
    else
        echo "[MISSING] $f"
    fi
done

echo
echo "=== SHA256 ==="

for f in \
    "$STM32_BUILD/inverted-pendulum.elf" \
    "$STM32_BUILD/inverted-pendulum.hex" \
    "$STM32_BUILD/inverted-pendulum.bin"
do
    if [ -f "$f" ]; then
        sha256sum "$f"
    fi
done

echo
echo "=== ELF SIZE ==="

if command -v arm-none-eabi-size >/dev/null 2>&1 && \
   [ -f "$STM32_BUILD/inverted-pendulum.elf" ]; then
    arm-none-eabi-size "$STM32_BUILD/inverted-pendulum.elf"
fi

echo
echo "============================================================"
echo " FINAL SUMMARY"
echo "============================================================"

echo "HEAD: $(git log -1 --oneline)"
echo "origin/main: $(git rev-parse --short=8 origin/main 2>/dev/null || echo unavailable)"

if [ -z "$(git status --porcelain)" ]; then
    echo "WORKTREE: clean"
else
    echo "WORKTREE: DIRTY"
fi

echo
echo "Next:"
echo "  1. Flash: $STM32_BUILD/inverted-pendulum.hex"
echo "  2. Capture boot/runtime log"
echo "  3. Send this entire output + board log to ChatGPT"
echo
echo "============================================================"
echo " POST PATCH CHECK COMPLETE"
echo "============================================================"

