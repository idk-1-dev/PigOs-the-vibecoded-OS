#!/bin/bash
set -e

VDM="/home/YOUR_USER/vibecoding/project/build/debug/vdm"
DEMO_TERM="/home/YOUR_USER/vibecoding/project/build/debug/demo-terminal"
SOCKET="/run/user/0/vdm-0"

cleanup() {
    sudo pkill -f "demo-terminal" 2>/dev/null || true
    sudo pkill -f "build/debug/vdm" 2>/dev/null || true
    sudo rm -f "$SOCKET"
    stty sane 2>/dev/null || true
    echo ""
    echo "VDM session ended."
}

trap cleanup EXIT

sudo mkdir -p /run/user/0
sudo mkdir -p /run/user/1000

sudo pkill -f "build/debug/vdm" 2>/dev/null || true
sudo pkill -f "demo-terminal" 2>/dev/null || true
sleep 1

sudo "$VDM" &
VDM_PID=$!
sleep 3

sudo VDM_DISPLAY="$SOCKET" "$DEMO_TERM" &

wait $VDM_PID 2>/dev/null || true
