#ifndef NOTIFICATION_MANAGER_H
#define NOTIFICATION_MANAGER_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>
#include "Logger.h"

class NotificationManager {
private:
    static String telegramBotToken;
    static String telegramChatId;
    static bool alertsEnabled;
    static unsigned long lastAlertMillis;

public:
    static void init() {
        Preferences prefs;
        prefs.begin("skunk_notify", true);
        telegramBotToken = prefs.getString("tg_token", "");
        telegramChatId = prefs.getString("tg_chatid", "");
        alertsEnabled = prefs.getBool("alerts_on", false);
        prefs.end();

        lastAlertMillis = 0;
        Logger::logf("NOTIFY", "Sistema de Alertas IoT (Telegram) inicializado. Estado: %s", alertsEnabled ? "ACTIVO" : "DESACTIVADO");
    }

    static void saveConfig(const String& token, const String& chatId, bool enabled) {
        Preferences prefs;
        prefs.begin("skunk_notify", false);
        prefs.putString("tg_token", token);
        prefs.putString("tg_chatid", chatId);
        prefs.putBool("alerts_on", enabled);
        prefs.end();

        telegramBotToken = token;
        telegramChatId = chatId;
        alertsEnabled = enabled;
        Logger::log("NOTIFY", "Configuración de alertas de Telegram actualizada en memoria NVS.");
    }

    static String getBotToken() { return telegramBotToken; }
    static String getChatId() { return telegramChatId; }
    static bool getIsAlertsEnabled() { return alertsEnabled; }

    // Enviar alerta por Telegram en un hilo/tarea temporal o directamente cuando se detecte incidencia en ZPL
    static void sendTelegramAlert(const String& message) {
        if (!alertsEnabled || telegramBotToken.isEmpty() || telegramChatId.isEmpty()) {
            return;
        }

        // Evitar saturar de mensajes por la misma alerta (máximo 1 alerta por cada 60 segundos)
        if (millis() - lastAlertMillis < 60000 && lastAlertMillis != 0) {
            return;
        }
        lastAlertMillis = millis();

        Logger::log("NOTIFY", "Enviando alerta SSL a Telegram: " + message);

        // Se crea tarea o se ejecuta rápido mediante WiFiClientSecure
        WiFiClientSecure client;
        client.setInsecure(); // No verificar certificado raíz temporalmente para máxima velocidad

        if (client.connect("api.telegram.org", 443)) {
            String url = "/bot" + telegramBotToken + "/sendMessage";
            String payload = "{\"chat_id\":\"" + telegramChatId + "\",\"text\":\"" + message + "\"}";

            client.println("POST " + url + " HTTP/1.1");
            client.println("Host: api.telegram.org");
            client.println("Content-Type: application/json");
            client.print("Content-Length: ");
            client.println(payload.length());
            client.println();
            client.print(payload);

            unsigned long timeout = millis();
            while (client.connected() && millis() - timeout < 3000) {
                if (client.available()) {
                    String line = client.readStringUntil('\n');
                    if (line.indexOf("200 OK") != -1) {
                        Logger::log("NOTIFY", "¡Alerta enviada a Telegram exitosamente!");
                        break;
                    }
                }
            }
            client.stop();
        } else {
            Logger::log("NOTIFY", "ERROR: No se pudo conectar con api.telegram.org (SSL/TLS fallido).");
        }
    }
};

String NotificationManager::telegramBotToken = "";
String NotificationManager::telegramChatId = "";
bool NotificationManager::alertsEnabled = false;
unsigned long NotificationManager::lastAlertMillis = 0;

#endif // NOTIFICATION_MANAGER_H
