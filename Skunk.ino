/**
 * ============================================================================
 * PROYECTO SKUNK - ESP32-S3 USB Host Print Server para Zebra GC420t
 * Compatible 100% con Arduino IDE
 * ============================================================================
 * 
 * Funcionalidades principales:
 * 1. Controlador USB OTG Host en pines D+ y D- para comunicarse con la Zebra GC420t.
 * 2. Servidor TCP RAW en puerto 9100 (Estándar JetDirect / AppSocket / Zebra Plugin Android).
 * 3. Servidor Web HTTP en puerto 80 con Consola de Diagnóstico y registro de LOGS circular en vivo.
 * 4. Gestión de Wi-Fi con soporte para IP estática o DHCP y auto-reconexión.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>

#include "Config.h"
#include "Logger.h"
#include "UsbPrinterHost.h"
#include "RawTcpServer.h"
#include "WebConsole.h"

unsigned long lastWifiCheck = 0;

void setupWifi() {
    Logger::log("WIFI", "Conectando a la red Wi-Fi: " + String(WIFI_SSID));

    if (USE_STATIC_IP) {
        if (!WiFi.config(LOCAL_IP, GATEWAY, SUBNET, DNS1, DNS2)) {
            Logger::log("WIFI", "ADVERTENCIA: Falló la configuración de IP estática.");
        } else {
            Logger::log("WIFI", "Configuración IP Estática aplicada: " + LOCAL_IP.toString());
        }
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Logger::log("WIFI", "¡Conectado exitosamente! IP asignada: " + WiFi.localIP().toString());
        Logger::logf("WIFI", "Señal RSSI: %d dBm", WiFi.RSSI());

        // Iniciar anuncio mDNS para facilitar descubrimiento en red
        if (MDNS.begin(DEVICE_HOSTNAME)) {
            MDNS.addService("http", "tcp", WEB_CONSOLE_PORT);
            MDNS.addService("pdl-datastream", "tcp", RAW_PRINT_PORT);
            Logger::log("MDNS", "Servicio mDNS activo: http://" + String(DEVICE_HOSTNAME) + ".local/");
        }
    } else {
        Logger::log("WIFI", "ERROR: No se pudo conectar al Wi-Fi. Reintentando en bucle principal...");
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    // 1. Inicializar sistema de logs en RAM para el panel web
    Logger::init();
    Logger::log("BOOT", "==================================================================");
    Logger::log("BOOT", "Iniciando Proyecto SKUNK - ESP32-S3 Zebra GC420t PrintServer v1.0");
    Logger::log("BOOT", "==================================================================");

    // 2. Conectar a red Wi-Fi
    setupWifi();

    // 3. Inicializar USB Host (Pines D+ y D- hacia el puerto USB de la impresora)
    UsbPrinterHost::init();

    // 4. Inicializar Servidor TCP RAW en puerto 9100 (Para Zebra Print Service Plugin)
    RawTcpServer::init();

    // 5. Inicializar Servidor Web de Diagnóstico y Logs en puerto 80
    WebConsole::init();

    Logger::log("BOOT", "¡Proyecto Skunk completamente operativo y listo para imprimir!");
}

void loop() {
    // Manejar peticiones HTTP de la Consola Web de Diagnóstico
    WebConsole::handleClient();

    // Verificar el estado de la conexión Wi-Fi periódicamente (cada 15 segundos)
    if (millis() - lastWifiCheck > 15000) {
        lastWifiCheck = millis();
        if (WiFi.status() != WL_CONNECTED) {
            Logger::log("WIFI", "ALERTA: Desconexión Wi-Fi detectada. Intentando reconectar...");
            WiFi.disconnect();
            WiFi.reconnect();
        }
    }

    delay(5);
}
