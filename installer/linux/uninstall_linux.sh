#!/usr/bin/env bash
# ==============================================================================
# Md-101 — Periodic Table Polyphonic VST3 & Standalone Synthesizer
# Linux Uninstaller Script
# Developed by LU5 (https://lu5.art.br)
# ==============================================================================
set -euo pipefail

APP_NAME="Md-101"
APP_ID="md-101"
APP_VERSION="1.1.1"

if [[ "${LANG:-}" == pt* ]] || [[ "${LC_ALL:-}" == pt* ]]; then
    UI_LANG="pt"
else
    UI_LANG="en"
fi

MODE="auto"
AUTO_YES=0

while [[ $# -gt 0 ]]; do
    case "$1" in
        --user)
            MODE="user"
            shift
            ;;
        --system)
            MODE="system"
            shift
            ;;
        --all)
            MODE="all"
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
            echo "Usage: $(basename "$0") [--user | --system | --all] [-y|--yes] [--lang pt|en]"
            exit 0
            ;;
        *)
            echo "Unknown option / Opção desconhecida: $1" >&2
            echo "Usage: $(basename "$0") [--user | --system | --all] [-y|--yes] [--lang pt|en]" >&2
            exit 1
            ;;
    esac
done

echo ""
echo "======================================================================"
if [[ "${UI_LANG}" == "pt" ]]; then
    echo "  Desinstalador do ${APP_NAME} v${APP_VERSION}"
else
    echo "  ${APP_NAME} v${APP_VERSION} Uninstaller"
fi
echo "======================================================================"

if [[ "${AUTO_YES}" -eq 0 ]]; then
    if [[ "${UI_LANG}" == "pt" ]]; then
        read -r -p "Deseja remover o Md-101? [s/N]: " confirm
        if [[ ! "${confirm:-N}" =~ ^[SsYy]$ ]]; then
            echo "Operação cancelada."
            exit 0
        fi
    else
        read -r -p "Remove Md-101? [y/N]: " confirm
        if [[ ! "${confirm:-N}" =~ ^[Yy]$ ]]; then
            echo "Operation cancelled."
            exit 0
        fi
    fi
fi

remove_user_files() {
    rm -rf "${HOME}/.vst3/Md-101.vst3"
    rm -f  "${HOME}/.local/bin/Md-101" "${HOME}/.local/bin/md-101" "${HOME}/.local/bin/md-101-uninstall"
    rm -f  "${HOME}/.local/share/applications/md-101.desktop"
    rm -f  "${HOME}/.local/share/icons/hicolor/scalable/apps/md-101.svg"
    rm -rf "${HOME}/.local/share/doc/${APP_ID}"
}

remove_system_files() {
    local sudo_cmd=""
    if [[ "${EUID:-$(id -u)}" -ne 0 ]]; then
        if command -v sudo >/dev/null 2>&1; then
            sudo_cmd="sudo"
        else
            return 0
        fi
    fi
    ${sudo_cmd} rm -rf "/usr/lib/vst3/Md-101.vst3" "/usr/local/lib/vst3/Md-101.vst3"
    ${sudo_cmd} rm -f  "/usr/local/bin/Md-101" "/usr/local/bin/md-101" "/usr/local/bin/md-101-uninstall"
    ${sudo_cmd} rm -f  "/usr/bin/Md-101" "/usr/bin/md-101" "/usr/bin/md-101-uninstall"
    ${sudo_cmd} rm -f  "/usr/share/applications/md-101.desktop" "/usr/local/share/applications/md-101.desktop"
    ${sudo_cmd} rm -f  "/usr/share/icons/hicolor/scalable/apps/md-101.svg"
    ${sudo_cmd} rm -rf "/usr/share/doc/${APP_ID}" "/usr/local/share/doc/${APP_ID}"
}

if [[ "${MODE}" == "user" ]]; then
    remove_user_files
elif [[ "${MODE}" == "system" ]]; then
    remove_system_files
elif [[ "${MODE}" == "all" ]]; then
    remove_user_files
    remove_system_files
else
    # Auto mode: remove user files, and if running as root or system install exists, remove system files
    remove_user_files
    if [[ "${EUID:-$(id -u)}" -eq 0 ]] || [[ -d "/usr/lib/vst3/Md-101.vst3" ]] || [[ -f "/usr/local/bin/Md-101" ]]; then
        remove_system_files
    fi
fi

if [[ "${UI_LANG}" == "pt" ]]; then
    echo "✔ Md-101 removido com sucesso."
else
    echo "✔ Md-101 uninstalled successfully."
fi
