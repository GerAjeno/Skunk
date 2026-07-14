#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// CONFIGURACIÓN POR DEFECTO DE RED WI-FI (PROYECTO SKUNK v2.0 Industrial)
// Si no hay credenciales guardadas en NVS (EEPROM), se utilizarán estas:
// ============================================================================
#define DEFAULT_WIFI_SSID     "Mi_Red_Almacen"
#define DEFAULT_WIFI_PASSWORD "ClaveAlmacen123"

// Configuración de IP Fija (Recomendado para que la app de Android siempre lo encuentre)
#define DEFAULT_USE_STATIC_IP true
const IPAddress DEFAULT_LOCAL_IP(192, 168, 1, 201);
const IPAddress DEFAULT_GATEWAY(192, 168, 1, 1);
const IPAddress DEFAULT_SUBNET(255, 255, 255, 0);
const IPAddress DEFAULT_DNS1(8, 8, 8, 8);
const IPAddress DEFAULT_DNS2(1, 1, 1, 1);

// Nombre y contraseña del Portal Cautivo Wi-Fi AP cuando no hay red configurada
#define AP_PORTAL_SSID        "Skunk-Setup-Zebra"
#define AP_PORTAL_PASSWORD    "12345678"
#define AP_PORTAL_TIMEOUT_SEC 180 // Tiempo máximo en modo AP antes de reintentar STA

// ============================================================================
// PUERTOS DE SERVICIO
// ==========================================
#define RAW_PRINT_PORT        9100    // Puerto estándar JetDirect / Raw TCP Print (Zebra Plugin)
#define WEB_CONSOLE_PORT      80      // Puerto para el panel web de diagnóstico y configuración
#define DNS_PORT              53      // Puerto para redirección DNS del Portal Cautivo

// ============================================================================
// CONFIGURACIÓN USB OTG HOST (ESP32-S3)
// ============================================================================
// En los ESP32-S3 DevKitC estándar, el puerto USB OTG está en:
// GPIO 20 (USB D+) y GPIO 19 (USB D-)
#define USB_HOST_DP_PIN       20
#define USB_HOST_DM_PIN       19

// Intervalo de consulta bidireccional ZPL (~HS - Host Status) al puerto USB en milisegundos
#define USB_STATUS_POLL_MS    3000

// ============================================================================
// CONFIGURACIÓN DE SPOOLER DE IMPRESIÓN (COLA EN PSRAM / RAM)
// ============================================================================
#define SPOOLER_MAX_JOBS      30      // Número máximo de trabajos simultáneos en cola
#define SPOOLER_CHUNK_SIZE    1024    // Tamaño de bloque para transferencias al USB

// ============================================================================
// CONFIGURACIÓN DE INDICADORES VISUALES (LED RGB / BUZZER)
// ============================================================================
// Muchos módulos ESP32-S3 de 16MB traen un LED RGB WS2812 integrado en GPIO 48 (o GPIO 38)
#define RGB_LED_PIN           48
#define BUZZER_PIN            -1      // Pon un número de GPIO (ej. 4) si conectas un zumbador, o -1 si está desactivado

// ============================================================================
// CONFIGURACIÓN DE LOGS Y DISPOSITIVO
// ============================================================================
#define LOG_BUFFER_LINES      60
#define LOG_LINE_MAX_LEN      160
#define DEVICE_HOSTNAME       "skunk-zebra-printserver"

#endif // CONFIG_H
