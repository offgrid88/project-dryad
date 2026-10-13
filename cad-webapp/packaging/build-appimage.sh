#!/usr/bin/env bash
# Build a single-file AppImage that serves the CAD generator and opens it.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${OUT:-$ROOT/dist}"
NODE_VERSION="${NODE_VERSION:-22.14.0}"
WORKDIR="$(mktemp -d)"
APPDIR="$WORKDIR/DryadCAD.AppDir"
cleanup() { rm -rf "$WORKDIR"; }
trap cleanup EXIT

mkdir -p "$APPDIR/usr/bin" "$APPDIR/usr/lib/dryad-cad"
if [[ ! -d "$ROOT/node_modules/three" ]]; then
  npm install --prefix "$ROOT"
fi

cp "$ROOT/server.js" "$APPDIR/usr/lib/dryad-cad/server.js"
cp -a "$ROOT/public" "$ROOT/src" "$APPDIR/usr/lib/dryad-cad/"
mkdir -p "$APPDIR/usr/lib/dryad-cad/node_modules"
cp -a "$ROOT/node_modules/three" \
      "$ROOT/node_modules/three-mesh-bvh" \
      "$ROOT/node_modules/three-bvh-csg" \
      "$APPDIR/usr/lib/dryad-cad/node_modules/"

curl -fsSL "https://nodejs.org/dist/v${NODE_VERSION}/node-v${NODE_VERSION}-linux-x64.tar.xz" \
  | tar -xJ -C "$WORKDIR" "node-v${NODE_VERSION}-linux-x64/bin/node"
install -m 755 "$WORKDIR/node-v${NODE_VERSION}-linux-x64/bin/node" "$APPDIR/usr/bin/node"

if command -v rsvg-convert >/dev/null 2>&1 && [[ -f "$ROOT/../logo.svg" ]]; then
  rsvg-convert -w 256 -h 256 "$ROOT/../logo.svg" -o "$APPDIR/dryad-cad.png"
else
  # 64×64 solid green PNG, so the image still has an icon without librsvg.
  python3 - "$APPDIR/dryad-cad.png" << 'PY'
import struct, sys, zlib
w = h = 64
raw = b"".join(b"\x00" + bytes([46, 125, 50, 255]) * w for _ in range(h))
def chunk(tag, data):
    return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)
png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0)) + chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b"")
open(sys.argv[1], "wb").write(png)
PY
fi
cp "$APPDIR/dryad-cad.png" "$APPDIR/.DirIcon"

cat > "$APPDIR/dryad-cad.desktop" << 'EOF'
[Desktop Entry]
Type=Application
Name=Dryad CAD
Comment=Parametric enclosure generator for Project Dryad
Exec=dryad-cad
Icon=dryad-cad
Categories=Graphics;Engineering;
Terminal=false
EOF

cat > "$APPDIR/AppRun" << 'EOF'
#!/bin/sh
HERE="$(dirname "$(readlink -f "$0")")"
export PORT="${PORT:-8080}"
"$HERE/usr/bin/node" "$HERE/usr/lib/dryad-cad/server.js" &
pid=$!
cleanup() { kill "$pid" 2>/dev/null || true; }
trap cleanup EXIT INT TERM
i=0
while [ "$i" -lt 50 ]; do
  if "$HERE/usr/bin/node" -e 'fetch("http://127.0.0.1:"+process.env.PORT+"/").then(()=>process.exit(0)).catch(()=>process.exit(1))'; then
    break
  fi
  i=$((i + 1))
  sleep 0.1
done
if command -v xdg-open >/dev/null 2>&1; then
  xdg-open "http://127.0.0.1:${PORT}/" >/dev/null 2>&1 || true
fi
wait "$pid"
EOF
chmod 755 "$APPDIR/AppRun"

mkdir -p "$OUT" "$WORKDIR/tools"
TOOL="$WORKDIR/tools/appimagetool"
curl -fsSL -o "$TOOL" https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage
chmod +x "$TOOL"
ARCH=x86_64 APPIMAGE_EXTRACT_AND_RUN=1 "$TOOL" "$APPDIR" "$OUT/DryadCAD-x86_64.AppImage"
chmod +x "$OUT/DryadCAD-x86_64.AppImage"
echo "Wrote $OUT/DryadCAD-x86_64.AppImage"
