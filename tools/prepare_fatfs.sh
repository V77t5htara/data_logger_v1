#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEST="$PROJECT_ROOT/lib/fatfs/src"
mkdir -p "$DEST"

SRC="$(find "$HOME/.platformio/packages" -type f -path '*/FatFs/src/ff.c' -print -quit || true)"
if [[ -z "$SRC" ]]; then
  echo "FatFs source not found in ~/.platformio/packages."
  echo "Run: pio run once with framework = stm32cube, then run this script again."
  exit 1
fi

FATFS_DIR="$(dirname "$SRC")"

echo "Using FatFs from: $FATFS_DIR"

for f in ff.c ff.h diskio.h; do
  cp "$FATFS_DIR/$f" "$DEST/$f"
done

if [[ -f "$FATFS_DIR/ffconf.h" ]]; then
  cp "$FATFS_DIR/ffconf.h" "$DEST/ffconf.h"
elif [[ -f "$FATFS_DIR/ffconf_template.h" ]]; then
  cp "$FATFS_DIR/ffconf_template.h" "$DEST/ffconf.h"
else
  echo "No ffconf.h or ffconf_template.h found."
  exit 1
fi

echo "FatFs files copied."
echo "Revision:"
grep -m1 -E '#define[[:space:]]+FF_DEFINED' "$DEST/ff.h" || true
echo "Configuration:"
grep -m1 -E '#define[[:space:]]+FFCONF_DEF' "$DEST/ffconf.h" || true
