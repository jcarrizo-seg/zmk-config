#!/bin/bash

# monitor_zmk_complete.sh - Script para capturar logs completos de ZMK
# Uso: ./monitor_zmk_complete.sh

DEVICE="/dev/ttyACM0"
LOG_FILE="zmk_full_log_$(date +%Y%m%d_%H%M%S).txt"

echo "=== ZMK Complete Debug Monitor ==="
echo "📁 Full log file: $LOG_FILE"
echo "🔌 Waiting for device connection..."
echo "🔄 You can now disconnect/reconnect USB to capture full sequence"
echo "⏹️  Press Ctrl+C to stop and view summary"
echo

# Función para verificar dispositivo
wait_for_device() {
    while [ ! -e "$DEVICE" ]; do
        echo "⏳ Waiting for $DEVICE..."
        sleep 1
    done
    echo "✅ Device $DEVICE found!"
}

# Función de limpieza al salir
cleanup() {
    echo
    echo "============================================="
    echo "🏁 MONITORING SESSION ENDED"
    echo "============================================="
    
    if [ -f "$LOG_FILE" ]; then
        echo "📊 Session Summary:"
        echo "   Total lines: $(wc -l < "$LOG_FILE")"
        echo
        
        echo "🔋 Battery Events:"
        grep -i "battery\|vddh\|charge" "$LOG_FILE" | tail -3
        echo
        
        echo "🔌 USB Events:"
        grep -i "usb\|power\|cdc" "$LOG_FILE" | tail -3
        echo
        
        echo "🔴 Errors:"
        grep -i "error\|err>" "$LOG_FILE" | tail -2
        echo
        
        echo "📂 Full log saved as: $LOG_FILE"
        echo "   View with: less $LOG_FILE"
        echo "   Filter USB: grep -i usb $LOG_FILE"
        echo "   Filter Battery: grep -i battery $LOG_FILE"
    fi
    
    exit 0
}

trap cleanup SIGINT SIGTERM

# Esperar dispositivo inicial (si no está conectado)
wait_for_device

# Configurar puerto serie
echo "⚙️  Configuring serial port..."
stty -F "$DEVICE" 115200 cs8 -cstopb -parenb

echo "📡 Starting log capture..."
echo "   Green = Info | Yellow = Warning | Red = Error | Blue = Debug"
echo

# Monitoreo principal con salida dual
cat "$DEVICE" | tee "$LOG_FILE" | while read -r line; do
    
    # Agregar timestamp local para referencia
    local_time=$(date '+%H:%M:%S.%3N')
    
    # Mostrar TODAS las líneas con filtro visual
    if [[ "$line" == *TEST_LOG* ]]; then
        
        # Extraer timestamp del log
        if [[ "$line" =~ \[([0-9:.,]+)\] ]]; then
            zmk_time="${BASH_REMATCH[1]}"
        else
            zmk_time="??:??:??.???"
        fi
        
        # Extraer nivel
        if [[ "$line" =~ \<([a-z]+)\> ]]; then
            level="${BASH_REMATCH[1]}"
        else
            level="???"
        fi
        
        # Formato con colores mejorado
        case "$level" in
            "err") 
                echo -e "\033[1;31m[$zmk_time] 🔴 $line\033[0m" 
                ;;
            "wrn") 
                echo -e "\033[1;33m[$zmk_time] 🟡 $line\033[0m" 
                ;;
            "inf") 
                if echo "$line" | grep -qi "usb\|battery.*changed\|init\|power"; then
                    echo -e "\033[1;32m[$zmk_time] 🟢 $line\033[0m"
                else
                    echo -e "\033[32m[$zmk_time] ℹ️  $line\033[0m"
                fi
                ;;
            "dbg") 
                if echo "$line" | grep -qi "vddh\|usb\|power"; then
                    echo -e "\033[36m[$zmk_time] 🔵 $line\033[0m"
                fi
                ;;
            *) 
                echo -e "\033[37m[$zmk_time] 📋 $line\033[0m" 
                ;;
        esac
    else
        # Líneas no filtradas en gris claro (solo a archivo)
        echo -e "\033[2m[$local_time] $line\033[0m" >/dev/null
    fi
    
    # Detectar desconexión de dispositivo
    if [ ! -e "$DEVICE" ]; then
        echo -e "\033[1;31m❌ Device disconnected! Waiting for reconnection...\033[0m"
        wait_for_device
        echo -e "\033[1;32m🔄 Device reconnected! Resuming capture...\033[0m"
        stty -F "$DEVICE" 115200 cs8 -cstopb -parenb
    fi
    
done