#!/usr/bin/env bash
# ==============================================================================
# Flameshot High-DPI Lossless PNG Launcher Script for Hyprland (Wayland)
# Passes full physical display resolution without fractional rounding blurring
# ==============================================================================

export QT_AUTO_SCREEN_SCALE_FACTOR=1
export QT_ENABLE_HIGHDPI_SCALING=1
export QT_SCALE_FACTOR_ROUNDING_POLICY=PassThrough
export XDG_CURRENT_DESKTOP=Hyprland
export XDG_SESSION_TYPE=wayland

exec flameshot gui "$@"
