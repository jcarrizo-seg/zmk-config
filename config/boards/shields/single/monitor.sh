#!/bin/bash
# Script simple para monitorear logs de batería ZMK
# Uso: ./monitor_battery.sh [puerto]

# Configuración
PORT=${1:-"/dev/ttyACM0"}  # Puerto por defecto
BAUDRATE=115200

echo "🔋 Monitor de Batería ZMK"
echo "========================"
echo "📍 Puerto: $PORT"
echo "⚡ Baudios: $BAUDRATE"
echo "💡 Presiona Ctrl+C para detener"
echo "========================"

# Verificar si el puerto existe
if [ ! -e "$PORT" ]; then
    echo "❌ Error: Puerto $PORT no encontrado"
    echo "🔍 Puertos disponibles:"
    ls /dev/tty* | grep -E "(ACM|USB)" | head -5
    exit 1
fi

# Verificar dependencias
if ! command -v tio &> /dev/null && ! command -v minicom &> /dev/null; then
    echo "❌ Error: Se necesita 'tio' o 'minicom'"
    echo "💡 Instalar con: sudo apt install tio (o minicom)"
    exit 1
fi

# Función para limpiar al salir
cleanup() {
    echo -e "\n⏹️  Monitor detenido"
    exit 0
}
trap cleanup INT

# Usar tio si está disponible, sino minicom
if command -v tio &> /dev/null; then
    echo "🔌 Conectando con tio..."
    tio -b $BAUDRATE $PORT | while read line; do
        timestamp=$(date '+%H:%M:%S')
        
        # Filtrar solo líneas relevantes
        if [[ $line == *"BATTERY STATUS"* ]]; then
            echo -e "\n🔋 [$timestamp] ACTUALIZACIÓN DE BATERÍA"
            echo "----------------------------------------"
        elif [[ $line == *"Battery level:"* ]]; then
            level=$(echo $line | grep -o '[0-9]\+%')
            echo "📊 Nivel: $level"
        elif [[ $line == *"Battery voltage:"* ]]; then
            voltage=$(echo $line | grep -o '[0-9]\+ mV ([0-9.]\+ V)')
            echo "⚡ Voltaje: $voltage"
        elif [[ $line == *"LED Status:"* ]]; then
            led_info=$(echo $line | sed 's/.*LED Status: //')
            case $led_info in
                *"GREEN"*) echo "💡 LED: 🟢 $led_info" ;;
                *"BLUE"*) echo "💡 LED: 🔵 $led_info" ;;
                *"YELLOW"*) echo "💡 LED: 🟡 $led_info" ;;
                *"RED"*) echo "💡 LED: 🔴 $led_info" ;;
                *) echo "💡 LED: $led_info" ;;
            esac
        elif [[ $line == *"BATTERY LED MODULE STARTED"* ]]; then
            echo -e "\n🚀 [$timestamp] MÓDULO INICIADO"
            echo "=============================="
        fi
    done
else
    echo "🔌 Conectando con minicom..."
    minicom -b $BAUDRATE -D $PORT
fi