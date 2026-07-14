/**
 * ============================================================================
 * PROYECTO SKUNK v2.0 Industrial - ESP32-S3 (16MB) USB Host Print Server
 * para Impresoras Zebra GC420t - Compatible 100% con Arduino IDE
 * ============================================================================
 * 
 * Funcionalidades y Mejoras implementadas (v2.0):
 * 1. Lectura bidireccional ZPL (~HS Host Status) al puerto USB para detectar Falta de Papel y alertas.
 * 2. Portal Cautivo Wi-Fi con memoria NVS EEPROM (No requiere reprogramar para cambiar clave/red).
 * 3. Micro-Cola de Impresión (Spooler en RAM/PSRAM) con seguro anticolisión ante múltiples celulares.
 * 4. Alertas IoT (Telegram SSL/TLS) e Indicadores Visuales en tiempo real con LED RGB Neopixel WS2812.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>

#include "Config.h"
#include "Logger.h"
#include "StatusIndicators.h"
#include "NotificationManager.h"
#include "WiFiManagerSkunk.h"
#include "UsbPrinterHost.h"
#include "PrintSpooler.h"
#include "RawTcpServer.h"
#include "WebConsole.h"

unsigned long lastWifiCheck = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);

    // 1. Inicializar indicadores visuales y sonoros (LED RGB WS2812 / Buzzer)
    StatusIndicators::init();

    // 2. Inicializar sistema de bitácora en RAM para la Consola Web
    Logger::init();
    Logger::log("BOOT", "========================================================================");
    Logger::log("BOOT", "Iniciando PROYECTO SKUNK v2.0 Industrial - ESP32-S3 (16MB) PrintServer");
    Logger::log("BOOT", "========================================================================");

    // 3. Inicializar gestor de notificaciones IoT (Telegram SSL/TLS)
    NotificationManager::init();

    // 4. Inicializar Wi-Fi y Portal Cautivo NVS
    WiFiManagerSkunk::init();

    // 5. Inicializar Controlador USB Host OTG y consulta bidireccional (~HS ZPL)
    UsbPrinterHost::init();

    // 6. Inicializar Spooler de Impresión en RAM/PSRAM para gestión anticolisión
    PrintSpooler::init();

    // 7. Inicializar Servidor TCP RAW en puerto 9100 (Redirigiendo al Spooler)
    RawTcpServer::init();

    // 8. Inicializar Servidor Web de Diagnóstico Industrial en puerto 80
    WebConsole::init();

    Logger::log("BOOT", "✅ ¡Proyecto Skunk v2.0 operativo y supervisando en todos los canales!");
}

void loop() {
    // 1. Procesar Portal Cautivo (DNS y WebServer en modo AP) si se activó la reconfiguración
    WiFiManagerSkunk::loop();

    // 2. Manejar peticiones HTTP del Panel Web de Diagnóstico
    WebConsole::handleClient();

    // 3. Actualizar animaciones visuales (parpadeos de alertas Neopixel en caso de falta de papel)
    StatusIndicators::loop();

    // 4. Supervisión y auto-reconexión Wi-Fi si estamos en modo Estación (STA)
    if (!WiFiManagerSkunk::getIsApMode()) {
        if (millis() - lastWifiCheck > 15000) {
            lastWifiCheck = millis();
            if (WiFi.status() != WL_CONNECTED) {
                Logger::log("WIFI", "⚠️ ALERTA: Desconexión temporal detectada en red local. Intentando reconectar...");
                WiFi.disconnect();
                WiFi.reconnect();
            }
        }
    }

    delay(5);
}
