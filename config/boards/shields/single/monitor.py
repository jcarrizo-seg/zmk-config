#!/usr/bin/env python3
"""
Script para monitorear logs de batería de ZMK
Filtra y muestra solo información relevante de batería y LEDs
"""

import serial
import serial.tools.list_ports
import time
import re
import sys
from datetime import datetime
import argparse

class BatteryMonitor:
    def __init__(self, port=None, baudrate=115200):
        self.port = port
        self.baudrate = baudrate
        self.ser = None
        
        # Patrones para filtrar logs relevantes
        self.patterns = {
            'battery_status': r'=== BATTERY STATUS ===',
            'battery_level': r'Battery level: (\d+)%',
            'battery_voltage': r'Battery voltage: (\d+) mV \(([\d.]+) V\)',
            'led_status': r'LED Status: (\w+) \((\w+)\)',
            'module_start': r'=== BATTERY LED MODULE STARTED ===',
            'initial_level': r'Initial battery level: (\d+)%',
            'initial_voltage': r'Initial battery voltage: (\d+) mV',
            'led_pins': r'LEDs configured on pins:'
        }
        
    def find_zmk_device(self):
        """Busca automáticamente el dispositivo ZMK"""
        ports = serial.tools.list_ports.comports()
        zmk_ports = []
        
        for port in ports:
            # Buscar dispositivos que probablemente sean ZMK
            if any(keyword in port.description.lower() for keyword in 
                   ['usb serial', 'cdc', 'acm', 'arduino', 'nice']):
                zmk_ports.append(port.device)
                
        if zmk_ports:
            print(f"🔍 Dispositivos encontrados: {zmk_ports}")
            return zmk_ports[0]  # Usar el primero encontrado
        return None
    
    def connect(self):
        """Conecta al dispositivo serie"""
        if not self.port:
            self.port = self.find_zmk_device()
            if not self.port:
                print("❌ No se pudo encontrar dispositivo ZMK")
                print("💡 Asegúrate de que el teclado esté conectado y con firmware de debug")
                return False
        
        try:
            self.ser = serial.Serial(self.port, self.baudrate, timeout=1)
            print(f"✅ Conectado a {self.port} @ {self.baudrate} baudios")
            print(f"📅 Iniciado a las {datetime.now().strftime('%H:%M:%S')}")
            print("=" * 60)
            return True
        except serial.SerialException as e:
            print(f"❌ Error conectando a {self.port}: {e}")
            return False
    
    def parse_line(self, line):
        """Parsea una línea y extrae información relevante"""
        line = line.strip()
        timestamp = datetime.now().strftime('%H:%M:%S')
        
        # Buscar patrones específicos
        for pattern_name, pattern in self.patterns.items():
            match = re.search(pattern, line)
            if match:
                if pattern_name == 'battery_status':
                    print(f"\n🔋 [{timestamp}] ACTUALIZACIÓN DE BATERÍA")
                    print("-" * 50)
                    
                elif pattern_name == 'battery_level':
                    level = int(match.group(1))
                    bar = self.get_battery_bar(level)
                    print(f"📊 Nivel: {level}% {bar}")
                    
                elif pattern_name == 'battery_voltage':
                    mv = int(match.group(1))
                    v = float(match.group(2))
                    print(f"⚡ Voltaje: {mv} mV ({v:.2f} V)")
                    
                elif pattern_name == 'led_status':
                    color = match.group(1)
                    level_desc = match.group(2)
                    emoji = self.get_led_emoji(color)
                    print(f"💡 LED: {emoji} {color} ({level_desc})")
                    
                elif pattern_name == 'module_start':
                    print(f"\n🚀 [{timestamp}] MÓDULO DE BATERÍA INICIADO")
                    print("=" * 50)
                    
                elif pattern_name == 'initial_level':
                    level = int(match.group(1))
                    bar = self.get_battery_bar(level)
                    print(f"📊 Nivel inicial: {level}% {bar}")
                    
                elif pattern_name == 'initial_voltage':
                    mv = int(match.group(1))
                    v = mv / 1000.0
                    print(f"⚡ Voltaje inicial: {mv} mV ({v:.2f} V)")
                    
                elif pattern_name == 'led_pins':
                    print(f"📌 {line}")
                
                return True
        
        return False
    
    def get_battery_bar(self, level):
        """Genera una barra visual del nivel de batería"""
        bars = level // 10
        empty = 10 - bars
        if level > 80:
            color = "🟩"
        elif level > 60:
            color = "🟦"
        elif level > 30:
            color = "🟨"
        else:
            color = "🟥"
        
        return f"[{color * bars}{'⬜' * empty}]"
    
    def get_led_emoji(self, color):
        """Obtiene emoji correspondiente al color del LED"""
        emojis = {
            'GREEN': '🟢',
            'BLUE': '🔵', 
            'YELLOW': '🟡',
            'RED': '🔴'
        }
        return emojis.get(color.upper(), '⚪')
    
    def monitor(self):
        """Inicia el monitoreo de logs"""
        if not self.connect():
            return
        
        print("🎯 Monitoreando logs de batería...")
        print("💡 Presiona Ctrl+C para detener")
        print("=" * 60)
        
        try:
            while True:
                if self.ser.in_waiting > 0:
                    try:
                        line = self.ser.readline().decode('utf-8', errors='ignore')
                        if line:
                            self.parse_line(line)
                    except UnicodeDecodeError:
                        continue
                time.sleep(0.01)  # Pequeña pausa para no sobrecargar la CPU
                
        except KeyboardInterrupt:
            print(f"\n\n⏹️  Monitoreo detenido a las {datetime.now().strftime('%H:%M:%S')}")
        finally:
            if self.ser:
                self.ser.close()
                print("🔌 Conexión cerrada")

def main():
    parser = argparse.ArgumentParser(description='Monitor de batería para ZMK')
    parser.add_argument('-p', '--port', help='Puerto serie (ej: /dev/ttyACM0, COM3)')
    parser.add_argument('-b', '--baudrate', type=int, default=115200, help='Velocidad de baudios')
    parser.add_argument('-l', '--list', action='store_true', help='Listar puertos disponibles')
    
    args = parser.parse_args()
    
    if args.list:
        print("🔍 Puertos serie disponibles:")
        ports = serial.tools.list_ports.comports()
        for port in ports:
            print(f"  📍 {port.device} - {port.description}")
        return
    
    monitor = BatteryMonitor(args.port, args.baudrate)
    monitor.monitor()

if __name__ == "__main__":
    main()