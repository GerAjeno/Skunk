#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ==========================================
// CONFIGURACIÓN DE RED WI-FI (PROYECTO SKUNK)
// ==========================================
#define WIFI_SSID           "TU_RED_WIFI"
#define WIFI_PASSWORD       "TU_CLAVE_WIFI"

// Configuración de IP Fija (Recomendado para que la app de Android siempre lo encuentre)
// Pon USE_STATIC_IP en false si prefieres que tu router le asigne IP por DHCP
#define USE_STATIC_IP       true
const IPAddress LOCAL_IP(192, 168, 1, 201);
const IPAddress GATEWAY(192, 168, 1, 1);
const IPAddress SUBNET(255, 255, 255, 0);
const IPAddress DNS1(8, 8, 8, 8);
const IPAddress DNS2(1, 1, 1, 1);

// ==========================================
// PUERTOS DE SERVICIO
// ==========================================
#define RAW_PRINT_PORT      9100    // Puerto estándar JetDirect / Raw TCP Print (Zebra Plugin)
#define WEB_CONSOLE_PORT    80      // Puerto para el panel web de diagnóstico y logs

// ==========================================
// CONFIGURACIÓN USB OTG HOST (ESP32-S3)
// ==========================================
// En los ESP32-S3 DevKitC estándar, el puerto USB OTG está en:
// GPIO 20 (USB D+) y GPIO 19 (USB D-)
#define USB_HOST_DP_PIN     20
#define USB_HOST_DM_PIN     19

// Tamaño del búfer circular para Logs en RAM (mostrados en la Web Console)
#define LOG_BUFFER_LINES    60
#define LOG_LINE_MAX_LEN    128

// Nombre del dispositivo para mDNS / Hostname en la red
#define DEVICE_HOSTNAME     "skunk-zebra-printserver"

#endif // CONFIG_H
