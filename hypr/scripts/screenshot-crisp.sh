#!/usr/bin/env bash
# ==============================================================================
# Ultra-Crisp Native Wayland Lossless PNG Screenshot Handler for Hyprland
# Captures 100% 1:1 physical display pixels without Qt double-scaling blur
# ==============================================================================

SAVE_DIR="$HOME/Pictures/Screenshots"
mkdir -p "$SAVE_DIR"

TIMESTAMP=$(date +'%Y-%m-%d_%H-%M-%S')
FILENAME="screenshot_${TIMESTAMP}.png"
FILEPATH="${SAVE_DIR}/${FILENAME}"

# Freeze screen for region selection
hyprpicker -r -z &
PICKER_PID=$!
trap 'kill "$PICKER_PID" 2>/dev/null' EXIT
sleep 0.05

# Let user select region via slurp
REGION=$(slurp -b "#00000080" -c "#888888ff" -w 1) || exit 0
if [ -z "$REGION" ]; then
    kill "$PICKER_PID" 2>/dev/null
    exit 0
fi

# Unfreeze screen
kill "$PICKER_PID" 2>/dev/null
trap - EXIT

# Capture raw 1:1 physical resolution framebuffer directly to PNG
grim -g "$REGION" -t png "$FILEPATH"

if [ -f "$FILEPATH" ]; then
    # Copy lossless PNG data to system clipboard
    wl-copy -t image/png < "$FILEPATH"

    # Send notification
    notify-send -a "Screen Capture" -i camera-photo-symbolic -t 2500 \
        "Screenshot Captured" "Saved to ${FILENAME} & copied to clipboard"
fi
