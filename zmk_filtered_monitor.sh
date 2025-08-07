#!/bin/bash

echo "=== ZMK Filtered Debug Monitor ==="
echo "Filtering for relevant logs only..."
echo "Press Ctrl+C to stop"
echo

# Verificar dispositivo
if [ ! -e /dev/ttyACM0 ]; then
    echo "Error: /dev/ttyACM0 not found"
    exit 1
fi

# Monitorear con filtros
stty -F /dev/ttyACM0 115200 cs8 -cstopb -parenb
cat /dev/ttyACM0 | while read -r line; do
    # Filtrar logs relevantes
    if echo "$line" | grep -qE "(inf>|wrn>|err>).*battery|LED|USB|zmk|Battery|Charging|split|keyboard"; then
        # Extraer timestamp y mensaje limpio
        timestamp=$(echo "$line" | sed 's/.*\[\([0-9:.,]*\)\].*/\1/')
        level=$(echo "$line" | sed 's/.*<\([^>]*\)>.*/\1/')
        module=$(echo "$line" | sed 's/.*> \([^:]*\):.*/\1/')
        message=$(echo "$line" | sed 's/.*> [^:]*: \(.*\)/\1/')
        
        # Formato limpio con colores
        case "$level" in
            "err") echo -e "\033[31m[$timestamp] ERROR [$module]: $message\033[0m" ;;
            "wrn") echo -e "\033[33m[$timestamp] WARN  [$module]: $message\033[0m" ;;
            "inf") echo -e "\033[32m[$timestamp] INFO  [$module]: $message\033[0m" ;;
            "dbg") echo -e "\033[36m[$timestamp] DEBUG [$module]: $message\033[0m" ;;
            *) echo "[$timestamp] $line" ;;
        esac
    fi
done
