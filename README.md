# Personal Linux Dotfiles (`~/.config`)

A portable, modular Linux dotfiles repository designed for long-term backup and multi-system deployment across machines, hardware, and distributions.

---

## 📁 Repository Structure

```text
~/.config/
├── hypr/                 # Hyprland compositor configuration
│   ├── hyprland.lua      # Main entry point for Hyprland configuration
│   ├── custom.lua        # User custom overrides & autostart integrations
│   ├── conf/             # Modular configurations (keybindings, windows, layout)
│   └── scripts/          # Workspace & utility scripts (includes hypr-edge-switcher)
├── waybar/               # Waybar status bar configuration & themes
├── kitty/                # Kitty terminal emulator configuration
├── quickshell/           # Quickshell widgets and bars
├── rofi/                 # Rofi application launcher configuration
├── swaync/               # SwayNotificationCenter configuration
├── nvim/                 # Neovim editor configuration
├── fish/                 # Fish shell configuration & aliases
├── ml4w/                 # ML4W dotfiles scripts and utilities
├── gtk-3.0/ & gtk-4.0/   # GTK appearance settings
├── ohmyposh/             # Prompt theme configuration
└── README.md             # Repository documentation
```

---

## ⚡ Hyprland Edge Workspace Switcher

This repository contains an **event-driven mouse-edge workspace switcher** built for Hyprland 0.56.2+.

### Architecture
- **Event-Driven**: Built in C using `gtk-layer-shell` and `wlr-layer-shell`. Creates 1-pixel wide transparent overlay surfaces along outer screen boundaries.
- **0.00% Idle CPU**: Sleeps in kernel `epoll_wait` when the cursor is anywhere on screen. Zero polling, zero high-frequency timers.
- **Wayland Focus Semantics**: Uses `wl_pointer.enter` to trigger workspace switches (`e-1` / `e+1`) once per edge contact. Uses `wl_pointer.leave` to re-arm the trigger upon cursor inward movement.
- **Direct UNIX Socket IPC**: Connects to `$XDG_RUNTIME_DIR/hypr/$HYPRLAND_INSTANCE_SIGNATURE/.socket.sock` for sub-millisecond execution without shell process overhead.
- **Multi-Monitor Geometry**: Computes outer display layout boundaries automatically to ignore internal borders between side-by-side monitors.

### Building the Helper
The source code and Makefile are included in `hypr/scripts/`:
```bash
cd ~/.config/hypr/scripts
make clean && make
```
The binary `hypr-edge-switcher` is automatically launched by `hypr/custom.lua` when Hyprland starts or reloads.

---

## ⚙️ Installation & Deployment

### 1. Fresh System Setup
Clone this repository directly to `~/.config`:
```bash
git clone <your-private-repo-url> ~/.config
```

### 2. Dependencies
Ensure the following packages are installed on your Linux distribution:
- **Compositor & Environment**: `hyprland`, `waybar`, `kitty`, `rofi-wayland`, `swaync`, `quickshell`
- **Development & Build**: `gcc`, `make`, `pkg-config`, `gtk-layer-shell`, `gtk3`
- **Shell & Tools**: `fish`, `neovim`, `fastfetch`, `oh-my-posh`

### 3. Machine-Specific Setup
Some settings (like screen resolutions and monitor coordinates) vary by hardware:
1. **Monitors**:
   Copy the template and set your display outputs:
   ```bash
   cp ~/.config/hypr/monitors.conf.example ~/.config/hypr/monitors.conf
   ```
   Edit `monitors.conf` using `hyprctl monitors` output.

2. **Secrets & Credentials**:
   Never commit private keys, API tokens, or GitHub tokens. Place secret environment variables in un-tracked local files (e.g. `~/.bashrc.local` or `~/.config/secrets/`).

---

## 🔒 Secrets & Exclusions

The `.gitignore` in this repository automatically excludes:
- Credentials & tokens (`gh/`, `.env`, `*_token`, `*.pem`, `*.key`)
- Heavy application caches (`BraveSoftware/`, `mozilla/`, `Antigravity IDE/`)
- Runtime lockfiles, sockets, logs, and compiled binaries (`hypr/scripts/hypr-edge-switcher`)

---

## 🔄 Restoration on New Installs

```bash
# 1. Back up existing ~/.config if present
mv ~/.config ~/.config.bak

# 2. Clone repository
git clone <your-git-url> ~/.config

# 3. Build edge switcher
make -C ~/.config/hypr/scripts

# 4. Reload Hyprland
hyprctl reload
```
