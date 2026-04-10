#!/bin/bash
set -e

# VDM Display Server Launcher
# Run this when user types 'wm' in the pigOS shell
# Paths for vibecoding-comp system

VDM_DIR="/home/User/vibecoding-comp/pigOS/vdm/project"
VDM="$VDM_DIR/build/debug/vdm"
DEMO_LASH="$VDM_DIR/build/debug/demo-lash"
SOCKET="/run/user/$(id -u)/vdm-0"

cleanup() {
    echo ""
    echo "Cleaning up VDM session..."
    sudo pkill -f "demo-lash" 2>/dev/null || true
    sudo pkill -f "build/debug/vdm" 2>/dev/null || true
    sudo rm -f "$SOCKET"
    stty sane 2>/dev/null || true
    echo "VDM session ended. Welcome back to pigOS."
}

trap cleanup EXIT

# Create socket directory
sudo mkdir -p /run/user/$(id -u)

# Kill any existing VDM instances
sudo pkill -f "build/debug/vdm" 2>/dev/null || true
sudo pkill -f "demo-lash" 2>/dev/null || true
sleep 1

echo "Starting VDM Display Server..."
echo "Press Ctrl+Alt+Backspace to exit."
echo ""

# Start the display server
sudo "$VDM" &
VDM_PID=$!
sleep 2

# Start the lash shell terminal
sudo VDM_DISPLAY="$SOCKET" "$DEMO_LASH" &

# Wait for VDM to finish
wait $VDM_PID 2>/dev/null || true
