#!/usr/bin/env bash
# ==============================================================================
# Md-101 — Periodic Table Polyphonic VST3 & Standalone Synthesizer
# Linux Installer Script (Ubuntu, Debian, Fedora, Arch, openSUSE, etc.)
# Developed by LU5 (https://lu5.art.br)
# ==============================================================================
set -euo pipefail

APP_NAME="Md-101"
APP_ID="md-101"
APP_VERSION="1.1.2"
VENDOR="LU5"
WEBSITE="https://lu5.art.br"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." 2>/dev/null || pwd)"

# Detect default language from environment (pt or en)
if [[ "${LANG:-}" == pt* ]] || [[ "${LC_ALL:-}" == pt* ]]; then
    UI_LANG="pt"
else
    UI_LANG="en"
fi

INSTALL_MODE=""          # "user" or "system"
INSTALL_VST3=1
INSTALL_STANDALONE=1
AUTO_YES=0
UNINSTALL_FLAG=0
CUSTOM_PREFIX=""
CUSTOM_VST3_DIR=""

print_banner() {
    echo ""
    echo "======================================================================"
    echo "  [101] Md — ${APP_NAME} v${APP_VERSION} (${VENDOR} - ${WEBSITE})"
    if [[ "${UI_LANG}" == "pt" ]]; then
        echo "  Instalador Linux — Sintetizador VST3 & Standalone da Tabela Periódica"
    else
        echo "  Linux Installer — Periodic Table VST3 & Standalone Synthesizer"
    fi
    echo "======================================================================"
    echo ""
}

usage() {
    if [[ "${UI_LANG}" == "pt" ]]; then
        cat <<EOF
Uso: $(basename "$0") [opções]

Opções:
  --user               Instala para o usuário atual em ~/.vst3 e ~/.local/bin (padrão sem root)
  --system             Instala para todo o sistema em /usr/lib/vst3 e /usr/local/bin (requer sudo/root)
  --vst3-only          Instala apenas o plugin VST3 (Md-101.vst3)
  --standalone-only    Instala apenas o aplicativo Standalone (Md-101)
  --prefix <dir>       Define o prefixo de instalação (ex.: /usr/local ou ~/.local)
  --vst3-dir <dir>     Define o diretório de destino do VST3
  --uninstall          Remove o Md-101 do sistema
  -y, --yes            Modo não-interativo (confirma automaticamente)
  --lang <pt|en>       Define o idioma do instalador (pt ou en)
  -h, --help           Exibe esta ajuda
EOF
    else
        cat <<EOF
Usage: $(basename "$0") [options]

Options:
  --user               Install for current user in ~/.vst3 and ~/.local/bin (default when non-root)
  --system             Install system-wide in /usr/lib/vst3 and /usr/local/bin (requires sudo/root)
  --vst3-only          Install only the VST3 plugin (Md-101.vst3)
  --standalone-only    Install only the Standalone application (Md-101)
  --prefix <dir>       Override installation prefix (e.g., /usr/local or ~/.local)
  --vst3-dir <dir>     Override target VST3 directory
  --uninstall          Uninstall Md-101
  -y, --yes            Non-interactive mode (assume yes)
  --lang <pt|en>       Set installer language (pt or en)
  -h, --help           Show this help message
EOF
    fi
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --user)
            INSTALL_MODE="user"
            shift
            ;;
        --system)
            INSTALL_MODE="system"
            shift
            ;;
        --vst3-only)
            INSTALL_VST3=1
            INSTALL_STANDALONE=0
            shift
            ;;
        --standalone-only)
            INSTALL_VST3=0
            INSTALL_STANDALONE=1
            shift
            ;;
        --prefix)
            if [[ $# -lt 2 || -z "${2:-}" ]]; then
                echo "Error: --prefix requires a non-empty directory path." >&2
                exit 1
            fi
            CUSTOM_PREFIX="${2}"
            shift 2
            ;;
        --vst3-dir)
            if [[ $# -lt 2 || -z "${2:-}" ]]; then
                echo "Error: --vst3-dir requires a non-empty directory path." >&2
                exit 1
            fi
            CUSTOM_VST3_DIR="${2}"
            shift 2
            ;;
        --uninstall)
            UNINSTALL_FLAG=1
            shift
            ;;
        -y|--yes)
            AUTO_YES=1
            shift
            ;;
        --lang)
            if [[ $# -lt 2 || ( "${2:-}" != "pt" && "${2:-}" != "en" ) ]]; then
                echo "Error: --lang must be 'pt' or 'en'." >&2
                exit 1
            fi
            UI_LANG="${2}"
            shift 2
            ;;
        -h|--help)
            print_banner
            usage
            exit 0
            ;;
        *)
            echo "Unknown option / Opção desconhecida: $1" >&2
            usage
            exit 1
            ;;
    esac
done

validate_abs_dir_path() {
    local label="$1"
    local val="$2"
    if [[ -z "${val}" ]]; then
        return 0
    fi
    if [[ "${val}" != /* ]] || [[ "${val}" == *$'\n'* ]] || [[ "${val}" == *$'\r'* ]]; then
        echo "Error: ${label} must be an absolute path without control/newline characters." >&2
        exit 1
    fi
}

validate_abs_dir_path "--prefix" "${CUSTOM_PREFIX}"
validate_abs_dir_path "--vst3-dir" "${CUSTOM_VST3_DIR}"

if [[ "${UNINSTALL_FLAG}" -eq 1 ]]; then
    UNINSTALL_ARGS=(--lang "${UI_LANG}")
    [[ "${AUTO_YES}" -eq 1 ]] && UNINSTALL_ARGS+=(--yes)
    [[ -n "${INSTALL_MODE}" ]] && UNINSTALL_ARGS+=("--${INSTALL_MODE}")
    if [[ -f "${SCRIPT_DIR}/uninstall_linux.sh" ]]; then
        exec bash "${SCRIPT_DIR}/uninstall_linux.sh" "${UNINSTALL_ARGS[@]}"
    elif [[ -f "${SCRIPT_DIR}/uninstall.sh" ]]; then
        exec bash "${SCRIPT_DIR}/uninstall.sh" "${UNINSTALL_ARGS[@]}"
    fi
fi

print_banner

# Determine installation mode if not explicitly set
if [[ -z "${INSTALL_MODE}" ]]; then
    if [[ "${EUID:-$(id -u)}" -eq 0 ]]; then
        INSTALL_MODE="system"
    elif [[ "${AUTO_YES}" -eq 1 ]]; then
        INSTALL_MODE="user"
    else
        if [[ "${UI_LANG}" == "pt" ]]; then
            echo "Escolha o modo de instalação:"
            echo "  [1] Usuário atual (Recomendado — ~/.vst3 e ~/.local/bin, não requer sudo)"
            echo "  [2] Todo o sistema (/usr/lib/vst3 e /usr/local/bin, requer sudo)"
            read -r -p "Opção [1]: " mode_choice
        else
            echo "Choose installation mode:"
            echo "  [1] Current user (Recommended — ~/.vst3 and ~/.local/bin, no sudo required)"
            echo "  [2] System-wide (/usr/lib/vst3 and /usr/local/bin, requires sudo)"
            read -r -p "Choice [1]: " mode_choice
        fi
        case "${mode_choice:-1}" in
            2) INSTALL_MODE="system" ;;
            *) INSTALL_MODE="user" ;;
        esac
        echo ""
    fi
fi

# Configure target paths based on mode
SUDO_CMD=""
if [[ "${INSTALL_MODE}" == "system" ]]; then
    umask 022
    if [[ "${EUID:-$(id -u)}" -ne 0 ]]; then
        if command -v sudo >/dev/null 2>&1; then
            SUDO_CMD="sudo"
        else
            echo "Error: root privileges or sudo required for --system installation." >&2
            exit 1
        fi
    fi
    PREFIX="${CUSTOM_PREFIX:-/usr/local}"
    VST3_DIR="${CUSTOM_VST3_DIR:-/usr/lib/vst3}"
    BIN_DIR="${PREFIX}/bin"
    APP_DIR="/usr/share/applications"
    ICON_DIR="/usr/share/icons/hicolor/scalable/apps"
    DOC_DIR="/usr/share/doc/${APP_ID}"
else
    PREFIX="${CUSTOM_PREFIX:-${HOME}/.local}"
    VST3_DIR="${CUSTOM_VST3_DIR:-${HOME}/.vst3}"
    BIN_DIR="${PREFIX}/bin"
    APP_DIR="${PREFIX}/share/applications"
    ICON_DIR="${PREFIX}/share/icons/hicolor/scalable/apps"
    DOC_DIR="${PREFIX}/share/doc/${APP_ID}"
fi

# Locate built artifacts (supports bundled tarball/run package OR source repo build/)
VST3_SRC=""
for candidate in \
    "${SCRIPT_DIR}/Md-101.vst3" \
    "${SCRIPT_DIR}/payload/Md-101.vst3" \
    "${PROJECT_ROOT}/build/Md101_artefacts/Release/VST3/Md-101.vst3" \
    "${PROJECT_ROOT}/build/Md101_artefacts/Debug/VST3/Md-101.vst3"; do
    if [[ -d "${candidate}" ]]; then
        VST3_SRC="${candidate}"
        break
    fi
done

STANDALONE_SRC=""
for candidate in \
    "${SCRIPT_DIR}/Md-101" \
    "${SCRIPT_DIR}/payload/Md-101" \
    "${PROJECT_ROOT}/build/Md101_artefacts/Release/Standalone/Md-101" \
    "${PROJECT_ROOT}/build/Md101_artefacts/Debug/Standalone/Md-101"; do
    if [[ -f "${candidate}" ]]; then
        STANDALONE_SRC="${candidate}"
        break
    fi
done

if [[ "${INSTALL_VST3}" -eq 1 && -z "${VST3_SRC}" ]]; then
    if [[ "${UI_LANG}" == "pt" ]]; then
        echo "Erro: Pacote Md-101.vst3 não encontrado. Compile primeiro com ./build_linux.sh Release." >&2
    else
        echo "Error: Md-101.vst3 bundle not found. Please build first using ./build_linux.sh Release." >&2
    fi
    exit 1
fi

if [[ "${INSTALL_STANDALONE}" -eq 1 && -z "${STANDALONE_SRC}" ]]; then
    if [[ "${UI_LANG}" == "pt" ]]; then
        echo "Aviso: Executável Standalone 'Md-101' não encontrado; instalando apenas o plugin VST3."
    else
        echo "Warning: Standalone binary 'Md-101' not found; installing VST3 plugin only."
    fi
    INSTALL_STANDALONE=0
fi

# Show summary before installing
if [[ "${UI_LANG}" == "pt" ]]; then
    echo "Resumo da instalação (${INSTALL_MODE}):"
    [[ "${INSTALL_VST3}" -eq 1 ]]       && echo "  • Plugin VST3       : ${VST3_DIR}/Md-101.vst3"
    [[ "${INSTALL_STANDALONE}" -eq 1 ]] && echo "  • App Standalone    : ${BIN_DIR}/Md-101"
    [[ "${INSTALL_STANDALONE}" -eq 1 ]] && echo "  • Atalho no Menu    : ${APP_DIR}/md-101.desktop"
    echo "  • Documentação      : ${DOC_DIR}"
    echo "  • Desinstalador     : ${BIN_DIR}/md-101-uninstall"
else
    echo "Installation summary (${INSTALL_MODE}):"
    [[ "${INSTALL_VST3}" -eq 1 ]]       && echo "  • VST3 Plugin       : ${VST3_DIR}/Md-101.vst3"
    [[ "${INSTALL_STANDALONE}" -eq 1 ]] && echo "  • Standalone App    : ${BIN_DIR}/Md-101"
    [[ "${INSTALL_STANDALONE}" -eq 1 ]] && echo "  • Desktop Launcher  : ${APP_DIR}/md-101.desktop"
    echo "  • Documentation     : ${DOC_DIR}"
    echo "  • Uninstaller       : ${BIN_DIR}/md-101-uninstall"
fi
echo ""

if [[ "${AUTO_YES}" -eq 0 ]]; then
    if [[ "${UI_LANG}" == "pt" ]]; then
        read -r -p "Deseja prosseguir com a instalação? [S/n]: " confirm
        if [[ "${confirm:-S}" =~ ^[Nn]$ ]]; then
            echo "Instalação cancelada."
            exit 0
        fi
    else
        read -r -p "Proceed with installation? [Y/n]: " confirm
        if [[ "${confirm:-Y}" =~ ^[Nn]$ ]]; then
            echo "Installation cancelled."
            exit 0
        fi
    fi
fi

# 1. Install VST3 Plugin
if [[ "${INSTALL_VST3}" -eq 1 ]]; then
    if [[ "${UI_LANG}" == "pt" ]]; then
        echo "==> Instalando plugin VST3 em ${VST3_DIR}/Md-101.vst3 ..."
    else
        echo "==> Installing VST3 plugin to ${VST3_DIR}/Md-101.vst3 ..."
    fi
    ${SUDO_CMD} mkdir -p "${VST3_DIR}"
    ${SUDO_CMD} rm -rf "${VST3_DIR}/Md-101.vst3"
    ${SUDO_CMD} cp -r "${VST3_SRC}" "${VST3_DIR}/Md-101.vst3"
    ${SUDO_CMD} chmod -R u+rwX,go+rX,go-w "${VST3_DIR}/Md-101.vst3"
    if [[ -d "${VST3_DIR}/Md-101.vst3/Contents/x86_64-linux" ]]; then
        ${SUDO_CMD} chmod 755 "${VST3_DIR}/Md-101.vst3/Contents/x86_64-linux/"*.so 2>/dev/null || true
    fi
fi

# 2. Install Standalone Application & Desktop Integration
if [[ "${INSTALL_STANDALONE}" -eq 1 ]]; then
    if [[ "${UI_LANG}" == "pt" ]]; then
        echo "==> Instalando aplicativo Standalone em ${BIN_DIR}/Md-101 ..."
    else
        echo "==> Installing Standalone application to ${BIN_DIR}/Md-101 ..."
    fi
    ${SUDO_CMD} mkdir -p "${BIN_DIR}"
    ${SUDO_CMD} cp -f "${STANDALONE_SRC}" "${BIN_DIR}/Md-101"
    ${SUDO_CMD} chmod 755 "${BIN_DIR}/Md-101"
    ${SUDO_CMD} ln -sf "${BIN_DIR}/Md-101" "${BIN_DIR}/md-101"

    # Install Icon
    ICON_SRC=""
    for candidate in \
        "${SCRIPT_DIR}/md-101.svg" \
        "${SCRIPT_DIR}/assets/md-101.svg" \
        "${PROJECT_ROOT}/assets/md-101.svg"; do
        if [[ -f "${candidate}" ]]; then
            ICON_SRC="${candidate}"
            break
        fi
    done

    if [[ -n "${ICON_SRC}" ]]; then
        ${SUDO_CMD} mkdir -p "${ICON_DIR}"
        ${SUDO_CMD} cp -f "${ICON_SRC}" "${ICON_DIR}/md-101.svg"
        ${SUDO_CMD} chmod 644 "${ICON_DIR}/md-101.svg"
    fi

    # Install .desktop launcher with exact Exec path
    ${SUDO_CMD} mkdir -p "${APP_DIR}"
    TMP_DESKTOP="$(mktemp)"
    cat > "${TMP_DESKTOP}" <<EOF
[Desktop Entry]
Type=Application
Name=Md-101
GenericName=Periodic Table Synthesizer
GenericName[pt]=Sintetizador da Tabela Periódica
Comment=16-Voice Polyphonic Periodic Table Synthesizer by LU5
Comment[pt]=Sintetizador Polifônico de 16 Vozes Baseado na Tabela Periódica por LU5
Exec=${BIN_DIR}/Md-101
Icon=md-101
Terminal=false
Categories=AudioVideo;Audio;Music;Midi;Synthesizer;
Keywords=VST3;Synth;Synthesizer;MIDI;Periodic Table;Mendelevium;LU5;
StartupNotify=true
EOF
    ${SUDO_CMD} cp -f "${TMP_DESKTOP}" "${APP_DIR}/md-101.desktop"
    ${SUDO_CMD} chmod 644 "${APP_DIR}/md-101.desktop"
    rm -f "${TMP_DESKTOP}"

    if command -v update-desktop-database >/dev/null 2>&1; then
        ${SUDO_CMD} update-desktop-database "${APP_DIR}" >/dev/null 2>&1 || true
    fi
fi

# 3. Install Documentation & License
${SUDO_CMD} mkdir -p "${DOC_DIR}"
for docfile in \
    "${SCRIPT_DIR}/README.md" "${SCRIPT_DIR}/README-en.md" "${SCRIPT_DIR}/LICENSE" \
    "${PROJECT_ROOT}/README.md" "${PROJECT_ROOT}/README-en.md" "${PROJECT_ROOT}/LICENSE"; do
    if [[ -f "${docfile}" ]]; then
        ${SUDO_CMD} cp -f "${docfile}" "${DOC_DIR}/"
    fi
done

for docdir in "${SCRIPT_DIR}/docs" "${PROJECT_ROOT}/docs"; do
    if [[ -d "${docdir}" ]]; then
        ${SUDO_CMD} cp -rf "${docdir}/"* "${DOC_DIR}/" 2>/dev/null || true
        break
    fi
done
${SUDO_CMD} chmod -R u+rwX,go+rX,go-w "${DOC_DIR}"

# 4. Install Uninstaller Script
${SUDO_CMD} mkdir -p "${BIN_DIR}"
UNINSTALL_SRC=""
for candidate in "${SCRIPT_DIR}/uninstall_linux.sh" "${SCRIPT_DIR}/uninstall.sh" "${PROJECT_ROOT}/installer/linux/uninstall_linux.sh"; do
    if [[ -f "${candidate}" ]]; then
        UNINSTALL_SRC="${candidate}"
        break
    fi
done

if [[ -n "${UNINSTALL_SRC}" ]]; then
    ${SUDO_CMD} cp -f "${UNINSTALL_SRC}" "${BIN_DIR}/md-101-uninstall"
    ${SUDO_CMD} chmod 755 "${BIN_DIR}/md-101-uninstall"
fi

echo ""
echo "======================================================================"
if [[ "${UI_LANG}" == "pt" ]]; then
    echo "  ✔ Md-101 v${APP_VERSION} instalado com sucesso!"
    echo "======================================================================"
    [[ "${INSTALL_VST3}" -eq 1 ]]       && echo "  • VST3 instalado em : ${VST3_DIR}/Md-101.vst3"
    [[ "${INSTALL_STANDALONE}" -eq 1 ]] && echo "  • Standalone        : ${BIN_DIR}/Md-101"
    echo "  • Manual HTML       : ${DOC_DIR}/manual.html"
    echo "  • Para desinstalar  : ${BIN_DIR}/md-101-uninstall"
    echo ""
    echo "  Abra sua DAW (Reaper, Bitwig, Ardour, Waveform, Carla...) e"
    echo "  reescaneie os plugins VST3 para usar o Md-101!"
else
    echo "  ✔ Md-101 v${APP_VERSION} installed successfully!"
    echo "======================================================================"
    [[ "${INSTALL_VST3}" -eq 1 ]]       && echo "  • VST3 installed to : ${VST3_DIR}/Md-101.vst3"
    [[ "${INSTALL_STANDALONE}" -eq 1 ]] && echo "  • Standalone        : ${BIN_DIR}/Md-101"
    echo "  • HTML Manual       : ${DOC_DIR}/manual-en.html"
    echo "  • To uninstall      : ${BIN_DIR}/md-101-uninstall"
    echo ""
    echo "  Open your DAW (Reaper, Bitwig, Ardour, Waveform, Carla...) and"
    echo "  rescan your VST3 plugins to play Md-101!"
fi
echo "======================================================================"
