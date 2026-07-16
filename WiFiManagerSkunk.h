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
#include "SkunkIcon.h"

class WiFiManagerSkunk {
private:
    static bool isApMode;
    static DNSServer dnsServer;
    static WebServer* apServer;
    static String currentSsid;

    static void setupPortalUi() {
        apServer = new WebServer(80);

        apServer->on("/logo.png", HTTP_GET, []() {
            apServer->sendHeader("Cache-Control", "public, max-age=86400");
            apServer->send_P(200, "image/png", SKUNK_ICON_PNG, SKUNK_ICON_PNG_LEN);
        });
        apServer->on("/favicon.ico", HTTP_GET, []() {
            apServer->sendHeader("Cache-Control", "public, max-age=86400");
            apServer->send_P(200, "image/png", SKUNK_ICON_PNG, SKUNK_ICON_PNG_LEN);
        });
        apServer->on("/apple-touch-icon.png", HTTP_GET, []() {
            apServer->sendHeader("Cache-Control", "public, max-age=86400");
            apServer->send_P(200, "image/png", SKUNK_ICON_PNG, SKUNK_ICON_PNG_LEN);
        });

        apServer->on("/", HTTP_GET, []() {
            String html = R"rawliteral(
<!DOCTYPE html><html lang="es"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Portal Cautivo - Skunk AMOLED Setup</title>
<link rel="icon" type="image/png" href="/logo.png">
<link rel="apple-touch-icon" href="/logo.png">
<meta name="theme-color" content="#000000">
<style>
body { background: #000000; color: #f8fafc; font-family: 'Segoe UI', system-ui, -apple-system, sans-serif; padding: 20px; display:flex; justify-content:center; align-items:center; min-height:90vh; }
.card { background: #0b0f19; padding: 30px; border-radius: 16px; max-width: 480px; width: 100%; border: 1px solid #1e293b; box-shadow: 0 15px 35px rgba(0,0,0,0.9); }
.header { display: flex; align-items: center; gap: 15px; margin-bottom: 24px; border-bottom: 1px solid #1e293b; padding-bottom: 18px; }
.logo { width: 56px; height: 56px; border-radius: 12px; border: 2px solid #3b82f6; box-shadow: 0 0 15px rgba(59,130,246,0.35); object-fit: cover; }
h2 { color: #38bdf8; font-size: 1.35rem; font-weight: 800; }
.sub { font-size: 0.8rem; color: #94a3b8; text-transform: uppercase; letter-spacing: 0.05em; }
label { display: block; margin-top: 16px; font-size: 0.88rem; font-weight:600; color: #cbd5e1; }
input { width: 100%; padding: 13px; margin-top: 6px; border-radius: 10px; border: 1px solid #334155; background: #05070c; color: white; font-size: 0.95rem; }
input:focus { border-color: #3b82f6; outline: none; box-shadow: 0 0 10px rgba(59,130,246,0.3); }
button { width: 100%; background: #10b981; color: #000000; border: none; padding: 15px; margin-top: 28px; border-radius: 10px; font-weight: 800; font-size: 1rem; cursor: pointer; box-shadow: 0 0 15px rgba(16,185,129,0.35); transition: all 0.2s; }
button:hover { background: #059669; color: white; box-shadow: 0 0 20px rgba(16,185,129,0.5); }
.note { font-size: 0.82rem; color: #64748b; margin-top: 20px; text-align: center; }
</style></head>
<body><div class="card">
<div class="header">
    <img src="/logo.png" alt="Skunk Icon" class="logo">
    <div>
        <h2>🦨 Skunk PrintServer</h2>
        <div class="sub">Configuración Wi-Fi (AMOLED)</div>
    </div>
</div>
<p style="font-size:0.88rem; color:#cbd5e1; line-height:1.5;">Conecta el módulo ESP32-S3 a la red local de tu almacén para habilitar la impresión inalámbrica desde teléfonos Android.</p>
<form action="/save" method="POST">
<label>Nombre de Red Wi-Fi (SSID):</label>
<input type="text" name="ssid" required placeholder="Ej. Mi_Red_Almacen">
<label>Contraseña Wi-Fi:</label>
<input type="password" name="password" placeholder="Contraseña de red">
<label>Token Bot Telegram (Opcional - Alertas):</label>
<input type="text" name="tg_token" placeholder="123456789:ABCdefGHI...">
<label>Chat ID Telegram (Opcional):</label>
<input type="text" name="tg_chatid" placeholder="-100123456789">
<button type="submit">💾 Guardar y Conectar</button>
</form>
<div class="note">Proyecto Skunk v2.0 • Modo AMOLED True Black (#000000)</div>
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

            String resp = "<html><head><meta name='theme-color' content='#000000'><link rel='icon' href='/logo.png'></head><body style='background:#000000;color:white;font-family:sans-serif;display:flex;justify-content:center;align-items:center;min-height:90vh;'>";
            resp += "<div style='background:#0b0f19;padding:40px;border-radius:16px;border:1px solid #1e293b;text-align:center;'>";
            resp += "<img src='/logo.png' style='width:64px;height:64px;border-radius:12px;margin-bottom:20px;'>";
            resp += "<h2 style='color:#38bdf8;'>✅ Configuración Guardada</h2><p style='color:#cbd5e1;margin-top:10px;'>El módulo Skunk se está reiniciando para conectar a:<br><b>" + newSsid + "</b></p></div></body></html>";
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

            Logger::log("WIFI", "📡 Portal Cautivo AMOLED activo en SSID: '" + String(AP_PORTAL_SSID) + "' -> IP: http://192.168.4.1/");
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
