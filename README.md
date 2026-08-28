# Personal Linux Dotfiles (`~/.config`)

A portable, modular Linux dotfiles repository designed for long-term backup and multi-system deployment across machines, hardware, and distributions.

---

## Quick Start & Automated Installation

To restore and set up this configuration on any Linux system (Arch, Fedora, Ubuntu/Debian):

```bash
# 1. Clone repository directly into ~/.config
git clone https://github.com/AniruthKarthik/dotfiles.git ~/.config

# 2. Run the automated installer script (installs fuzzel, hyprland, waybar, kitty, etc.)
cd ~/.config && ./install.sh

# 3. Reload Hyprland
hyprctl reload
```

The `./install.sh` script automatically detects your distribution package manager (`pacman`, `dnf`, `apt`), installs all required applications (including `fuzzel`, `waybar`, `kitty`, `swaync`, `rofi`, compilation toolchains), and builds the C edge workspace switcher binary.

---

## Repository Structure

```text
~/.config/
├── install.sh            # One-click automated setup & package installer script
├── hypr/                 # Hyprland compositor configuration
│   ├── hyprland.lua      # Main entry point for Hyprland configuration
│   ├── custom.lua        # User custom overrides & autostart integrations
│   ├── conf/             # Modular configurations (keybindings, windows, layout)
│   └── scripts/          # Workspace & utility scripts (includes hypr-edge-switcher)
├── waybar/               # Waybar status bar configuration & themes
├── kitty/                # Kitty terminal emulator configuration
├── quickshell/           # Quickshell widgets and bars
├── rofi/                 # Rofi application launcher configuration
├── fuzzel/               # Fuzzel application launcher configuration
├── swaync/               # SwayNotificationCenter configuration
├── nvim/                 # Neovim editor configuration
├── fish/                 # Fish shell configuration & aliases
├── ml4w/                 # ML4W dotfiles scripts and utilities
├── gtk-3.0/ & gtk-4.0/   # GTK appearance settings
├── ohmyposh/             # Prompt theme configuration
└── README.md             # Repository documentation
```

---

## Hyprland Edge Workspace Switcher

This repository contains a **pure Wayland event-driven mouse-edge workspace switcher** built for Hyprland 0.56.2+.

### Architecture
- **Pure Wayland Client (`libwayland-client`)**: Eliminates heavy GTK/Qt runtime overhead. Memory footprint is ~2.4 MB RSS.
- **0.00% Idle CPU**: Sleeps in kernel `epoll_wait` via `wl_display_dispatch()`. Zero polling, zero high-frequency timers.
- **Wayland Focus Semantics**: Uses `wl_pointer.enter` to trigger workspace switches (`e-1` / `e+1`) once per edge contact. Uses `wl_pointer.leave` to re-arm the trigger upon cursor inward movement.
- **Direct UNIX Socket IPC**: Connects directly to `$XDG_RUNTIME_DIR/hypr/$HYPRLAND_INSTANCE_SIGNATURE/.socket.sock` for sub-millisecond execution without shell/subprocess overhead.
- **Multi-Monitor Geometry**: Queries Hyprland IPC JSON monitor topology (`j/monitors`) at startup and computes outer display layout boundaries automatically to ignore internal borders between side-by-side monitors.

### Building the Helper
The source code and Makefile are included in `hypr/scripts/`:
```bash
cd ~/.config/hypr/scripts
make clean && make
```
The binary `hypr-edge-switcher` is automatically launched by `hypr/custom.lua` when Hyprland starts or reloads.

---

## Dependencies & Package List

Installed automatically by `./install.sh`:
- **Desktop & Compositor**: `hyprland`, `hyprpaper`, `hyprlock`, `hypridle`, `hyprpicker`, `hyprsunset`, `waybar`, `kitty`, `fuzzel`, `flameshot`, `rofi-wayland`, `swaync`
- **Media & Audio Controls**: `playerctl`, `brightnessctl`, `pamixer`, `pavucontrol`, `network-manager-applet`
- **Utilities & OCR**: `grim`, `slurp`, `wl-clipboard`, `cliphist`, `jq`, `bc`, `imagemagick`, `tesseract`
- **Shell & Tools**: `fish`, `neovim`, `btop`, `fastfetch`, `oh-my-posh`, `nwg-look`, `qt6ct`
- **Build Chain**: `gcc`, `make`, `pkg-config`, `wayland-scanner`, `wayland-protocols`, `gtk-layer-shell`

---

## Machine-Specific Setup

1. **Monitors**:
   Copy the template and set your display outputs:
   ```bash
   cp ~/.config/hypr/monitors.conf.example ~/.config/hypr/monitors.conf
   ```
   Edit `monitors.conf` using `hyprctl monitors` output.

2. **Secrets & Credentials**:
   Never commit private keys, API tokens, or GitHub tokens. Place secret environment variables in un-tracked local files (e.g. `~/.bashrc.local` or `~/.config/secrets/`).

---

## Secrets & Exclusions

The `.gitignore` in this repository automatically excludes:
- Credentials & tokens (`gh/`, `.env`, `*_token`, `*.pem`, `*.key`)
- Heavy application caches (`BraveSoftware/`, `mozilla/`, `Antigravity IDE/`)
- Runtime lockfiles, sockets, logs, and compiled binaries (`hypr/scripts/hypr-edge-switcher`)
