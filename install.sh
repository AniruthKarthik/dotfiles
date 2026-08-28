#!/usr/bin/env bash
# ==============================================================================
# Dotfiles Setup & Package Installer Script
# Works on Arch Linux, Fedora, and Debian/Ubuntu-based distributions
# ==============================================================================

set -e

GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== Dotfiles & Dependencies Installer ===${NC}\n"

# Detect OS
if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS=$ID
    LIKE=$ID_LIKE
else
    echo -e "${RED}Error: Cannot detect Linux distribution from /etc/os-release${NC}"
    exit 1
fi

echo -e "Detected OS: ${GREEN}${NAME:-$OS}${NC}"

# Package lists per distribution
ARCH_PKGS=(
    hyprland
    waybar
    kitty
    fuzzel
    rofi-wayland
    swaync
    fastfetch
    oh-my-posh
    qt6ct
    pavucontrol
    grim
    slurp
    wl-clipboard
    cliphist
    gcc
    make
    pkg-config
    wayland-scanner
    wayland-protocols
    gtk-layer-shell
)

FEDORA_PKGS=(
    hyprland
    waybar
    kitty
    fuzzel
    rofi-wayland
    SwayNotificationCenter
    fastfetch
    qt6ct
    pavucontrol
    grim
    slurp
    wl-clipboard
    cliphist
    gcc
    make
    pkgconfig
    wayland-protocols-devel
    gtk-layer-shell-devel
)

DEBIAN_PKGS=(
    hyprland
    waybar
    kitty
    fuzzel
    sway-notification-center
    fastfetch
    qt6ct
    pavucontrol
    grim
    slurp
    wl-clipboard
    cliphist
    gcc
    make
    pkg-config
    libwayland-dev
    wayland-protocols
    libgtk-layer-shell-dev
)

install_arch() {
    echo -e "\n${BLUE}Installing packages via pacman...${NC}"
    sudo pacman -S --needed --noconfirm "${ARCH_PKGS[@]}"
}

install_fedora() {
    echo -e "\n${BLUE}Installing packages via dnf...${NC}"
    sudo dnf install -y "${FEDORA_PKGS[@]}"
}

install_debian() {
    echo -e "\n${BLUE}Installing packages via apt...${NC}"
    sudo apt update && sudo apt install -y "${DEBIAN_PKGS[@]}"
}

case "$OS" in
    arch|manjaro|endeavouros|garuda)
        install_arch
        ;;
    fedora)
        install_fedora
        ;;
    ubuntu|debian|pop)
        install_debian
        ;;
    *)
        if [[ "$LIKE" == *"arch"* ]]; then
            install_arch
        elif [[ "$LIKE" == *"fedora"* ]]; then
            install_fedora
        elif [[ "$LIKE" == *"debian"* || "$LIKE" == *"ubuntu"* ]]; then
            install_debian
        else
            echo -e "${YELLOW}Warning: Untested distribution '$OS'. Please install packages manually:${NC}"
            echo "${ARCH_PKGS[*]}"
        fi
        ;;
esac

# Build mouse-edge workspace switcher
echo -e "\n${BLUE}Building Hyprland edge workspace switcher helper...${NC}"
if [ -f "$HOME/.config/hypr/scripts/Makefile" ]; then
    make -C "$HOME/.config/hypr/scripts" clean && make -C "$HOME/.config/hypr/scripts"
    echo -e "${GREEN}✓ Edge switcher binary built successfully.${NC}"
fi

echo -e "\n${GREEN}=== Installation Complete! ===${NC}"
echo -e "You can now reload Hyprland with: ${BLUE}hyprctl reload${NC}\n"
