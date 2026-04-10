#!/bin/bash
# scripts/genconfig.sh - Generate kernel/autoconf.h from .config
set -e

CONFIG_FILE="${1:-kernel/.config}"
OUTPUT_FILE="${2:-kernel/autoconf.h}"

if [ ! -f "$CONFIG_FILE" ]; then
    echo "ERROR: $CONFIG_FILE not found. Run 'make menuconfig' first."
    exit 1
fi

cat > "$OUTPUT_FILE" << 'HEADER'
/* autoconf.h - Auto-generated from .config by genconfig.sh
 * DO NOT EDIT - use 'make menuconfig' to change options
 */
#pragma once

HEADER

while IFS= read -r line; do
    # Skip comments and empty lines
    [[ "$line" =~ ^#.*$ ]] && continue
    [[ -z "$line" ]] && continue

    if [[ "$line" =~ ^CONFIG_([A-Za-z0-9_]+)=y$ ]]; then
        echo "#define CONFIG_${BASH_REMATCH[1]} 1" >> "$OUTPUT_FILE"
    elif [[ "$line" =~ ^CONFIG_([A-Za-z0-9_]+)=m$ ]]; then
        echo "#define CONFIG_${BASH_REMATCH[1]}_MODULE 1" >> "$OUTPUT_FILE"
    elif [[ "$line" =~ ^CONFIG_([A-Za-z0-9_]+)=([0-9]+)$ ]]; then
        echo "#define CONFIG_${BASH_REMATCH[1]} ${BASH_REMATCH[2]}" >> "$OUTPUT_FILE"
    elif [[ "$line" =~ ^CONFIG_([A-Za-z0-9_]+)=\"(.*)\"$ ]]; then
        echo "#define CONFIG_${BASH_REMATCH[1]} \"${BASH_REMATCH[2]}\"" >> "$OUTPUT_FILE"
    elif [[ "$line" =~ ^#\ CONFIG_([A-Za-z0-9_]+)\ is\ not\ set$ ]]; then
        echo "/* CONFIG_${BASH_REMATCH[1]} is not set */" >> "$OUTPUT_FILE"
    fi
done < "$CONFIG_FILE"

echo "[OK] Generated $OUTPUT_FILE from $CONFIG_FILE"
