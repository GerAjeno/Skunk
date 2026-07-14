#ifndef WIFI_MANAGER_SKUNK_H
#define WIFI_MANAGER_SKUNK_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include "Config.h"
#include "Logger.h"
#include "StatusIndicators.h"
#include "NotificationManager.h"

class WiFiManagerSkunk {
private:
    static bool isApMode;
    static DNSServer dnsServer;
    static WebServer* apServer;
    static String currentSsid;

    static void setupPortalUi() {
        apServer = new WebServer(80);

        apServer->on("/", HTTP_GET, []() {
            String html = R"rawliteral(
<!DOCTYPE html><html lang="es"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Portal Cautivo - Skunk Setup</title>
<style>
body { background: #0f172a; color: #f8fafc; font-family: sans-serif; padding: 20px; display:flex; justify-content:center; }
.card { background: #1e293b; padding: 25px; border-radius: 12px; max-width: 450px; width: 100%; box-shadow: 0 10px 25px rgba(0,0,0,0.5); }
h2 { color: #3b82f6; margin-bottom: 20px; }
label { display: block; margin-top: 15px; font-size: 0.9rem; color: #94a3b8; }
input, select { width: 100%; padding: 12px; margin-top: 6px; border-radius: 8px; border: 1px solid #334155; background: #0f172a; color: white; }
button { width: 100%; background: #10b981; color: white; border: none; padding: 14px; margin-top: 25px; border-radius: 8px; font-weight: bold; cursor: pointer; }
button:hover { background: #059669; }
</style></head>
<body><div class="card">
<h2>🦨 Skunk PrintServer - Configuración Wi-Fi</h2>
<p style="font-size:0.85rem; color:#cbd5e1;">Conéctate a la red de tu almacén para habilitar la impresión desde los dispositivos Android.</p>
<form action="/save" method="POST">
<label>Nombre de Red (SSID):</label>
<input type="text" name="ssid" required placeholder="Ej. Mi_Red_Almacen">
<label>Contraseña Wi-Fi:</label>
<input type="password" name="password" placeholder="Contraseña de red">
<label>Token Bot Telegram (Opcional - Alertas):</label>
<input type="text" name="tg_token" placeholder="123456789:ABCdefGHI...">
<label>Chat ID Telegram (Opcional):</label>
<input type="text" name="tg_chatid" placeholder="-100123456789">
<button type="submit">💾 Guardar Configuración y Reiniciar</button>
</form>
</div></body></html>
            )rawliteral";
            apServer->send(200, "text/html; charset=utf-8", html);
        });

        apServer->on("/save", HTTP_POST, []() {
            String newSsid = apServer->arg("ssid");
            String newPass = apServer->arg("password");
            String tgToken = apServer->arg("tg_token");
            String tgChat = apServer->arg("tg_chatid");

            Preferences prefs;
            prefs.begin("skunk_wifi", false);
            prefs.putString("ssid", newSsid);
            prefs.putString("password", newPass);
            prefs.end();

            if (!tgToken.isEmpty() && !tgChat.isEmpty()) {
                NotificationManager::saveConfig(tgToken, tgChat, true);
            }

            String resp = "<html><body style='background:#0f172a;color:white;font-family:sans-serif;text-align:center;padding:50px;'>";
            resp += "<h2>✅ Configuración Guardada</h2><p>El ESP32 se está reiniciando para conectarse a: <b>" + newSsid + "</b></p></body></html>";
            apServer->send(200, "text/html; charset=utf-8", resp);

            delay(1500);
            ESP.restart();
        });

        apServer->onNotFound([]() {
            apServer->sendHeader("Location", "http://192.168.4.1/", true);
            apServer->send(302, "text/plain", "");
        });

        apServer->begin();
    }

public:
    static void init() {
        Preferences prefs;
        prefs.begin("skunk_wifi", true);
        String savedSsid = prefs.getString("ssid", DEFAULT_WIFI_SSID);
        String savedPass = prefs.getString("password", DEFAULT_WIFI_PASSWORD);
        prefs.end();

        currentSsid = savedSsid;
        Logger::log("WIFI", "Intentando conectar a Wi-Fi: " + savedSsid);

        if (DEFAULT_USE_STATIC_IP) {
            WiFi.config(DEFAULT_LOCAL_IP, DEFAULT_GATEWAY, DEFAULT_SUBNET, DEFAULT_DNS1, DEFAULT_DNS2);
        }

        WiFi.mode(WIFI_STA);
        WiFi.begin(savedSsid.c_str(), savedPass.c_str());

        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 35) {
            delay(500);
            Serial.print(".");
            attempts++;
        }
        Serial.println();

        if (WiFi.status() == WL_CONNECTED) {
            isApMode = false;
            Logger::log("WIFI", "✅ ¡Conexión exitosa! IP local: " + WiFi.localIP().toString());
            Logger::logf("WIFI", "Señal RSSI: %d dBm", WiFi.RSSI());
            StatusIndicators::setState(STATE_OK);

            if (MDNS.begin(DEVICE_HOSTNAME)) {
                MDNS.addService("http", "tcp", WEB_CONSOLE_PORT);
                MDNS.addService("pdl-datastream", "tcp", RAW_PRINT_PORT);
                Logger::log("MDNS", "Servicio mDNS activo en http://" + String(DEVICE_HOSTNAME) + ".local/");
            }
        } else {
            // No se pudo conectar. Levantar portal cautivo de configuración
            isApMode = true;
            StatusIndicators::setState(STATE_PORTAL_AP);
            Logger::log("WIFI", "⚠️ No se pudo conectar a " + savedSsid + ". Activando Portal Cautivo AP...");

            WiFi.mode(WIFI_AP);
            WiFi.softAP(AP_PORTAL_SSID, AP_PORTAL_PASSWORD);
            IPAddress apIP = WiFi.softAPIP();

            dnsServer.start(DNS_PORT, "*", apIP);
            setupPortalUi();

            Logger::log("WIFI", "📡 Portal Cautivo activo en SSID: '" + String(AP_PORTAL_SSID) + "' -> IP: http://192.168.4.1/");
        }
    }

    static bool getIsApMode() { return isApMode; }
    static String getSsid() { return currentSsid; }

    static void loop() {
        if (isApMode) {
            dnsServer.processNextRequest();
            if (apServer) apServer->handleClient();
        }
    }

    static void resetCredentialsAndRestart() {
        Preferences prefs;
        prefs.begin("skunk_wifi", false);
        prefs.clear();
        prefs.end();
        Logger::log("WIFI", "Credenciales borradas de NVS. Reiniciando en modo Portal Cautivo...");
        delay(500);
        ESP.restart();
    }
};

bool WiFiManagerSkunk::isApMode = false;
DNSServer WiFiManagerSkunk::dnsServer;
WebServer* WiFiManagerSkunk::apServer = nullptr;
String WiFiManagerSkunk::currentSsid = "";

#endif // WIFI_MANAGER_SKUNK_H
