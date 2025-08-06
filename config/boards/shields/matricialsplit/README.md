# Battery Status LEDs Module

Módulo personalizado para ZMK que proporciona indicación visual del estado de batería usando LEDs individuales.

## Características

- **Indicación de batería baja** (LED azul)
- **Indicación de carga** (LED rojo)  
- **Indicación de batería cargada** (LED verde)
- **Detección automática** de conexión USB
- **Umbrales configurables** para estados de batería
- **Validación estricta** de configuración en tiempo de compilación

## Estados de LEDs

| Condición | Batería | USB | LED Rojo | LED Verde | LED Azul |
|-----------|---------|-----|----------|-----------|----------|
| Sin sensor | N/A | Cualquiera | OFF | **ON** | OFF |
| Batería baja | < umbral inferior | NO | OFF | OFF | **ON** |
| Batería OK | ≥ umbral inferior | NO | OFF | OFF | OFF |
| Cargando | < umbral superior | SÍ | **ON** | OFF | OFF |
| Cargada | ≥ umbral superior | SÍ | OFF | **ON** | OFF |

## Configuración

### 1. Device Tree (mi_teclado.overlay)

#### Configuración Mínima
```devicetree
/ {
    battery_status_leds: battery_status_leds {
        compatible = "zmk,battery-status-leds";
        status = "okay";
        
        red-gpio = <&gpio1 4 GPIO_ACTIVE_HIGH>;
        green-gpio = <&gpio1 6 GPIO_ACTIVE_HIGH>;
        blue-gpio = <&gpio1 11 GPIO_ACTIVE_HIGH>;
        // Usa defaults: low=20%, full=95%
    };
};
```

#### Configuración Personalizada
```devicetree
/ {
    battery_status_leds: battery_status_leds {
        compatible = "zmk,battery-status-leds";
        status = "okay";
        
        red-gpio = <&gpio1 4 GPIO_ACTIVE_HIGH>;
        green-gpio = <&gpio1 6 GPIO_ACTIVE_HIGH>;
        blue-gpio = <&gpio1 11 GPIO_ACTIVE_HIGH>;
        
        low-threshold = <25>;   // Batería baja a 25%
        full-threshold = <90>;  // Cargada a 90%
    };
};
```

### 2. Archivos Requeridos

```
config/boards/shields/mi_teclado/
├── mi_teclado.overlay              # Configuración Device Tree
├── battery_status_leds.c           # Código principal
├── CMakeLists.txt                  # Configuración build
├── Kconfig.defconfig               # Configuración Kconfig
└── dts/bindings/
    └── zmk,battery-status-leds.yaml # Binding definition
```

## Hardware

### Pines Recomendados (nRF52840)
- **LED Rojo:** P1.04 (compatible nice!nano)
- **LED Verde:** P1.06 (compatible nice!nano)  
- **LED Azul:** P1.11 (compatible nice!nano)

### Circuito Sugerido
```
nRF52840 Pin ──┐
               │
              ┌▼┐ Resistencia 
              └┬┘ (220Ω - 1kΩ)
               │
              ┌▼┐ LED
              └┬┘
               │
              GND
```

## Validaciones

### Umbrales
- **low-threshold:** 5-40% (default: 20%)
- **full-threshold:** 60-100% (default: 95%)
- **Gap mínimo:** 10% entre umbrales

### Errores de Compilación
El módulo fallará la compilación si:
- Los umbrales están fuera de rango
- `full-threshold ≤ low-threshold + 10`
- Faltan propiedades GPIO requeridas

## Dependencias ZMK

### Configuraciones Requeridas
- `CONFIG_ZMK_BATTERY_REPORTING=y`
- `CONFIG_GPIO=y`
- `CONFIG_LOG=y` (recomendado para debugging)

### Eventos ZMK Utilizados
- `zmk_battery_state_changed` - Cambios de nivel de batería
- `zmk_usb_conn_state_changed` - Conexión/desconexión USB

## Debugging

### Logs Útiles
```bash
# Ver logs durante compilación
west build --pristine

# Ver logs en runtime (si tienes RTT configurado)
# Buscar líneas que contengan:
# - "Battery Status LEDs"
# - "Battery: X%, USB: connected/disconnected"
# - "Thresholds: low=X%, full=Y%"
```

### Testing
1. **Sin batería:** LED verde debe estar encendido
2. **Con batería baja:** Desconectar USB → LED azul
3. **Cargando:** Conectar USB con batería < 95% → LED rojo  
4. **Cargada:** Batería ≥ 95% con USB → LED verde

## Troubleshooting

### LED no se enciende
- Verificar conexiones GPIO
- Verificar que `status = "okay"` esté presente
- Revisar logs de inicialización

### Comportamiento inesperado
- Verificar umbrales configurados
- Revisar estado de batería: `zmk_battery_state_of_charge()`
- Verificar estado USB: `zmk_usb_is_powered()`

### Error de compilación
- Verificar sintaxis Device Tree
- Confirmar que umbrales estén en rango válido
- Verificar que todos los archivos estén presentes

## Roadmap Futuro

- [ ] Soporte para LEDs RGB con PWM
- [ ] Patrones de parpadeo configurables  
- [ ] Auto-apagado después de timeout
- [ ] Soporte para teclados split (ambos lados)
- [ ] Publicación como módulo ZMK oficial