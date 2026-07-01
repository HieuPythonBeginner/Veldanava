#!/usr/bin/env bash
# install.sh - Install velrun system-wide or user-only
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BIN_NAME="velrun"
FULL_NAME="running_true_dragon_core"

echo "=== Veldanava Ecosystem Installer ==="
echo ""

# Check if we have sudo
if sudo -n true 2>/dev/null; then
    echo "[*] sudo detected - installing system-wide to /usr/local/bin/"
    sudo cp "$SCRIPT_DIR/build/${BIN_NAME}" "/usr/local/bin/${BIN_NAME}"
    sudo cp "$SCRIPT_DIR/build/${FULL_NAME}" "/usr/local/bin/${FULL_NAME}"
    sudo chmod +x "/usr/local/bin/${BIN_NAME}"
    sudo chmod +x "/usr/local/bin/${FULL_NAME}"
    echo "[+] Installed to /usr/local/bin/${BIN_NAME}"
    echo "[+] Installed to /usr/local/bin/${FULL_NAME}"
    echo ""
    echo "You can now run: velrun <file>"
else
    echo "[*] No sudo - installing user-only to ~/.local/bin/"
    mkdir -p "$HOME/.local/bin"
    cp "$SCRIPT_DIR/build/${BIN_NAME}" "$HOME/.local/bin/${BIN_NAME}"
    cp "$SCRIPT_DIR/build/${FULL_NAME}" "$HOME/.local/bin/${FULL_NAME}"
    chmod +x "$HOME/.local/bin/${BIN_NAME}"
    chmod +x "$HOME/.local/bin/${FULL_NAME}"
    echo "[+] Installed to ~/.local/bin/${BIN_NAME}"
    echo "[+] Installed to ~/.local/bin/${FULL_NAME}"
    echo ""

    # Check if ~/.local/bin is in PATH
    if ! echo "$PATH" | grep -q "$HOME/.local/bin"; then
        echo "[!] ~/.local/bin is not in your PATH."
        echo "    Adding it to ~/.bashrc..."
        echo 'export PATH="$HOME/.local/bin:$PATH"' >> "$HOME/.bashrc"
        echo ""
        echo "Restart your shell or run: source ~/.bashrc"
    fi

    echo ""
    echo "You can now run: $HOME/.local/bin/${BIN_NAME} <file>"
    echo "Or after restarting shell: ${BIN_NAME} <file>"
fi

echo ""
echo "Supported extensions: .veldanava .veldora .velzard .velgrynd"
