#!/usr/bin/env bash
# ==============================================================================
# Md-101 — Debian / Ubuntu (.deb) Package Builder
# Generates dist/md-101_<version>_amd64.deb
# Developed by LU5 (https://lu5.art.br)
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

RAW_VERSION="${1:-1.1.1}"
# Strip leading 'v' if passed from git tag (e.g., v1.1.1 -> 1.1.1)
DEB_VERSION="${RAW_VERSION#v}"
if [[ ! "${DEB_VERSION}" =~ ^[0-9] ]]; then
    DEB_VERSION="1.1.1"
fi

ARCH="amd64"
PKG_NAME="md-101"
BUILD_TYPE="${2:-Release}"

VST3_SRC="${PROJECT_ROOT}/build/Md101_artefacts/${BUILD_TYPE}/VST3/Md-101.vst3"
STANDALONE_SRC="${PROJECT_ROOT}/build/Md101_artefacts/${BUILD_TYPE}/Standalone/Md-101"

if [[ ! -d "${VST3_SRC}" ]]; then
    echo "Error: VST3 bundle not found at ${VST3_SRC}. Run ./build_linux.sh ${BUILD_TYPE} first." >&2
    exit 1
fi

STAGE_DIR="$(mktemp -d)"
trap 'rm -rf "${STAGE_DIR}"' EXIT

DEB_ROOT="${STAGE_DIR}/${PKG_NAME}_${DEB_VERSION}_${ARCH}"
mkdir -p "${DEB_ROOT}/DEBIAN"
mkdir -p "${DEB_ROOT}/usr/lib/vst3"
mkdir -p "${DEB_ROOT}/usr/bin"
mkdir -p "${DEB_ROOT}/usr/share/applications"
mkdir -p "${DEB_ROOT}/usr/share/icons/hicolor/scalable/apps"
mkdir -p "${DEB_ROOT}/usr/share/doc/${PKG_NAME}"

# 1. Copy VST3 bundle
cp -r "${VST3_SRC}" "${DEB_ROOT}/usr/lib/vst3/Md-101.vst3"
chmod -R a+rX "${DEB_ROOT}/usr/lib/vst3/Md-101.vst3"
if [[ -d "${DEB_ROOT}/usr/lib/vst3/Md-101.vst3/Contents/x86_64-linux" ]]; then
    chmod 755 "${DEB_ROOT}/usr/lib/vst3/Md-101.vst3/Contents/x86_64-linux/"*.so 2>/dev/null || true
fi

# 2. Copy Standalone binary (if built)
if [[ -f "${STANDALONE_SRC}" ]]; then
    cp -f "${STANDALONE_SRC}" "${DEB_ROOT}/usr/bin/Md-101"
    chmod 755 "${DEB_ROOT}/usr/bin/Md-101"
    ln -sf "Md-101" "${DEB_ROOT}/usr/bin/md-101"
fi

# 3. Copy Desktop entry & SVG icon
if [[ -f "${PROJECT_ROOT}/installer/linux/md-101.desktop" ]]; then
    cp -f "${PROJECT_ROOT}/installer/linux/md-101.desktop" "${DEB_ROOT}/usr/share/applications/md-101.desktop"
    chmod 644 "${DEB_ROOT}/usr/share/applications/md-101.desktop"
fi

if [[ -f "${PROJECT_ROOT}/assets/md-101.svg" ]]; then
    cp -f "${PROJECT_ROOT}/assets/md-101.svg" "${DEB_ROOT}/usr/share/icons/hicolor/scalable/apps/md-101.svg"
    chmod 644 "${DEB_ROOT}/usr/share/icons/hicolor/scalable/apps/md-101.svg"
fi

# 4. Copy Documentation & License
for f in README.md README-en.md LICENSE; do
    if [[ -f "${PROJECT_ROOT}/${f}" ]]; then
        cp -f "${PROJECT_ROOT}/${f}" "${DEB_ROOT}/usr/share/doc/${PKG_NAME}/"
    fi
done
if [[ -d "${PROJECT_ROOT}/docs" ]]; then
    cp -rf "${PROJECT_ROOT}/docs/"* "${DEB_ROOT}/usr/share/doc/${PKG_NAME}/" 2>/dev/null || true
fi
if [[ -f "${PROJECT_ROOT}/LICENSE" ]]; then
    cp -f "${PROJECT_ROOT}/LICENSE" "${DEB_ROOT}/usr/share/doc/${PKG_NAME}/copyright"
fi

# Compute installed size in KB
INSTALLED_SIZE="$(du -sk "${DEB_ROOT}/usr" | awk '{print $1}')"

cat > "${DEB_ROOT}/DEBIAN/control" <<EOF
Package: ${PKG_NAME}
Version: ${DEB_VERSION}
Section: sound
Priority: optional
Architecture: ${ARCH}
Installed-Size: ${INSTALLED_SIZE}
Depends: libc6 (>= 2.31), libasound2 | libasound2t64, libfreetype6, libx11-6, libxinerama1, libxrandr2, libxcursor1, libxcomposite1
Maintainer: LU5 <contato@lu5.art.br>
Homepage: https://lu5.art.br
Provides: vst3-plugin
Description: Md-101 — Periodic Table Polyphonic VST3 & Standalone Synthesizer
 Md-101 is a 16-voice polyphonic virtual MIDI instrument developed by LU5
 in C++17 / JUCE 8. Each of the 118 chemical elements of the Periodic Table
 acts as a unique synthesizer preset whose timbre is physically computed from
 its chemical family, atomic number (Z), atomic mass (A), and physical phase.
 .
 Includes both the 64-bit VST3 plugin (/usr/lib/vst3/Md-101.vst3) and the
 Standalone desktop synthesizer (/usr/bin/Md-101).
EOF

cat > "${DEB_ROOT}/DEBIAN/postinst" <<'EOF'
#!/bin/sh
set -e
if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database -q /usr/share/applications || true
fi
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache -q -t -f /usr/share/icons/hicolor || true
fi
exit 0
EOF
chmod 755 "${DEB_ROOT}/DEBIAN/postinst"

cat > "${DEB_ROOT}/DEBIAN/postrm" <<'EOF'
#!/bin/sh
set -e
if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database -q /usr/share/applications || true
fi
exit 0
EOF
chmod 755 "${DEB_ROOT}/DEBIAN/postrm"

mkdir -p "${PROJECT_ROOT}/dist"
OUT_DEB="${PROJECT_ROOT}/dist/${PKG_NAME}_${DEB_VERSION}_${ARCH}.deb"

dpkg-deb --root-owner-group --build "${DEB_ROOT}" "${OUT_DEB}"
echo "==> Debian package created: ${OUT_DEB}"
