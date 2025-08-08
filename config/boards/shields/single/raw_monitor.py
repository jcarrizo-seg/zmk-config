#!/usr/bin/env python3
"""
Monitor básico que muestra TODOS los logs sin filtrar
Para diagnosticar si llegan datos del teclado
"""

import serial
import time
from datetime import datetime

def raw_monitor(port="/dev/ttyACM0"):
    try:
        ser = serial.Serial(port, 115200, timeout=1)
        print(f"🔌 Conectado a {port}")
        print(f"📅 {datetime.now().strftime('%H:%M:%S')} - Mostrando TODOS los logs")
        print("=" * 60)
        print("💡 Presiona una tecla en el teclado o espera eventos de batería")
        print("💡 Presiona Ctrl+C para salir")
        print("=" * 60)
        
        line_count = 0
        while True:
            if ser.in_waiting > 0:
                try:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        line_count += 1
                        timestamp = datetime.now().strftime('%H:%M:%S.%f')[:-3]
                        print(f"[{timestamp}] {line}")
                        
                        # Destacar líneas relevantes
                        if any(word in line.lower() for word in ['battery', 'led', 'init']):
                            print(f"    ⭐ RELEVANTE: {line}")
                            
                except UnicodeDecodeError:
                    continue
            else:
                # Mostrar contador cada 10 segundos si no hay datos
                if int(time.time()) % 10 == 0:
                    print(f"⏱️  {datetime.now().strftime('%H:%M:%S')} - Esperando datos... (líneas recibidas: {line_count})")
                    time.sleep(1)
            
            time.sleep(0.01)
            
    except KeyboardInterrupt:
        print(f"\n⏹️  Monitor detenido. Total líneas: {line_count}")
    except Exception as e:
        print(f"❌ Error: {e}")
    finally:
        if 'ser' in locals():
            ser.close()

if __name__ == "__main__":
    import sys
    port = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyACM0"
    raw_monitor(port)