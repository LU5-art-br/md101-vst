#!/usr/bin/env bash
# ==============================================================================
# Md-101 — Master Linux Installer & Package Builder
# Builds:
#   1. Self-extracting universal installer: dist/Md-101-<version>-Linux-x64-Installer.run
#   2. Debian/Ubuntu package (.deb):        dist/md-101_<version>_amd64.deb
#   3. Portable archive with installer:     dist/Md-101-<version>-Linux-x64.tar.gz
# Developed by LU5 (https://lu5.art.br)
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_TYPE="${1:-Release}"
VERSION="${2:-${GITHUB_REF_NAME:-v1.1.2}}"

VST3_PATH="${SCRIPT_DIR}/build/Md101_artefacts/${BUILD_TYPE}/VST3/Md-101.vst3"
STANDALONE_PATH="${SCRIPT_DIR}/build/Md101_artefacts/${BUILD_TYPE}/Standalone/Md-101"

if [[ ! -d "${VST3_PATH}" ]]; then
    echo "==> Artifacts not found in build/. Building Md-101 (${BUILD_TYPE}) first..."
    bash "${SCRIPT_DIR}/build_linux.sh" "${BUILD_TYPE}"
fi

mkdir -p "${SCRIPT_DIR}/dist"

PKG_DIR_NAME="Md-101-${VERSION}-Linux-x64"
STAGE_BASE="$(mktemp -d)"
trap 'rm -rf "${STAGE_BASE}"' EXIT

STAGE_PKG="${STAGE_BASE}/${PKG_DIR_NAME}"
mkdir -p "${STAGE_PKG}/docs" "${STAGE_PKG}/assets"

echo "==> Staging Linux distribution files..."
cp -r "${VST3_PATH}" "${STAGE_PKG}/Md-101.vst3"
if [[ -f "${STANDALONE_PATH}" ]]; then
    cp -f "${STANDALONE_PATH}" "${STAGE_PKG}/Md-101"
    chmod 755 "${STAGE_PKG}/Md-101"
fi

cp -f "${SCRIPT_DIR}/installer/linux/install_linux.sh" "${STAGE_PKG}/install.sh"
cp -f "${SCRIPT_DIR}/installer/linux/uninstall_linux.sh" "${STAGE_PKG}/uninstall.sh"
chmod 755 "${STAGE_PKG}/install.sh" "${STAGE_PKG}/uninstall.sh"

if [[ -f "${SCRIPT_DIR}/assets/md-101.svg" ]]; then
    cp -f "${SCRIPT_DIR}/assets/md-101.svg" "${STAGE_PKG}/assets/md-101.svg"
fi
if [[ -f "${SCRIPT_DIR}/installer/linux/md-101.desktop" ]]; then
    cp -f "${SCRIPT_DIR}/installer/linux/md-101.desktop" "${STAGE_PKG}/md-101.desktop"
fi

for doc in README.md README-en.md LICENSE; do
    [[ -f "${SCRIPT_DIR}/${doc}" ]] && cp -f "${SCRIPT_DIR}/${doc}" "${STAGE_PKG}/"
done
if [[ -d "${SCRIPT_DIR}/docs" ]]; then
    cp -rf "${SCRIPT_DIR}/docs/"* "${STAGE_PKG}/docs/" 2>/dev/null || true
fi

# 1. Create Portable .tar.gz Archive (includes install.sh & uninstall.sh)
TARBALL_OUT="${SCRIPT_DIR}/dist/${PKG_DIR_NAME}.tar.gz"
tar -czf "${TARBALL_OUT}" -C "${STAGE_BASE}" "${PKG_DIR_NAME}"
echo "==> Created tarball with installer scripts: ${TARBALL_OUT}"

# 2. Create Self-Extracting Universal Installer (.run)
PAYLOAD_TAR="${STAGE_BASE}/payload.tar.gz"
tar -czf "${PAYLOAD_TAR}" -C "${STAGE_PKG}" .

RUN_OUT="${SCRIPT_DIR}/dist/Md-101-${VERSION}-Linux-x64-Installer.run"
cat > "${RUN_OUT}" <<'EOF_HEADER'
#!/usr/bin/env bash
# ==============================================================================
# Md-101 — Self-Extracting Linux Installer (.run)
# Developed by LU5 (https://lu5.art.br)
# ==============================================================================
set -euo pipefail

TMP_EXTRACT="$(mktemp -d)"
cleanup() {
    rm -rf "${TMP_EXTRACT}"
}
trap cleanup EXIT

ARCHIVE_LINE="$(awk '/^__ARCHIVE_BELOW__$/ {print NR + 1; exit 0; }' "$0")"
tail -n +"${ARCHIVE_LINE}" "$0" | tar -xzf - --no-same-owner -C "${TMP_EXTRACT}"

chmod +x "${TMP_EXTRACT}/install.sh" "${TMP_EXTRACT}/uninstall.sh"
exec bash "${TMP_EXTRACT}/install.sh" "$@"
__ARCHIVE_BELOW__
EOF_HEADER

cat "${PAYLOAD_TAR}" >> "${RUN_OUT}"
chmod 755 "${RUN_OUT}"
echo "==> Created self-extracting installer: ${RUN_OUT}"

# 3. Create Debian/Ubuntu (.deb) Package if dpkg-deb is available
if command -v dpkg-deb >/dev/null 2>&1; then
    bash "${SCRIPT_DIR}/installer/linux/build_deb.sh" "${VERSION}" "${BUILD_TYPE}"
else
    echo "==> Note: dpkg-deb not found; skipping .deb generation (self-extracting .run and .tar.gz built)."
fi

echo ""
echo "==> Linux installers generated successfully in dist/:"
ls -lh "${SCRIPT_DIR}/dist/"
