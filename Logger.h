#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include <vector>
#include "Config.h"

class Logger {
private:
    static std::vector<String> logRingBuffer;
    static size_t ringHead;
    static unsigned long totalBytesReceived;
    static unsigned long totalBytesSent;
    static unsigned long totalPrintJobs;

public:
    static void init() {
        logRingBuffer.reserve(LOG_BUFFER_LINES);
        log("SISTEMA", "Logger Skunk inicializado. Búfer circular de depuración activo (" + String(LOG_BUFFER_LINES) + " líneas).");
    }

    static void log(const char* module, const String& message) {
        unsigned long sec = millis() / 1000;
        unsigned long min = sec / 60;
        sec %= 60;
        char timeStr[16];
        snprintf(timeStr, sizeof(timeStr), "%02lu:%02lu", min, sec);

        String formatted = "[" + String(timeStr) + "] [" + String(module) + "] " + message;
        
        // Imprimir por Serial (Monitor Serie de Arduino IDE)
        Serial.println(formatted);

        // Almacenar en búfer circular en RAM para la Consola Web del celular/PC
        if (logRingBuffer.size() < LOG_BUFFER_LINES) {
            logRingBuffer.push_back(formatted);
        } else {
            logRingBuffer[ringHead] = formatted;
            ringHead = (ringHead + 1) % LOG_BUFFER_LINES;
        }
    }

    static void logf(const char* module, const char* format, ...) {
        char buf[LOG_LINE_MAX_LEN];
        va_list args;
        va_start(args, format);
        vsnprintf(buf, sizeof(buf), format, args);
        va_end(args);
        log(module, String(buf));
    }

    static void addBytesReceived(unsigned long bytes) { totalBytesReceived += bytes; }
    static void addBytesSent(unsigned long bytes) { totalBytesSent += bytes; }
    static void incrementJobs() { totalPrintJobs++; }

    static unsigned long getBytesReceived() { return totalBytesReceived; }
    static unsigned long getBytesSent() { return totalBytesSent; }
    static unsigned long getPrintJobs() { return totalPrintJobs; }

    static String getLogsAsJson() {
        String json = "[";
        size_t count = logRingBuffer.size();
        for (size_t i = 0; i < count; i++) {
            size_t idx = (ringHead + i) % count;
            if (i > 0) json += ",";
            String escaped = logRingBuffer[idx];
            escaped.replace("\"", "\\\"");
            escaped.replace("\r", "");
            escaped.replace("\n", "");
            json += "\"" + escaped + "\"";
        }
        json += "]";
        return json;
    }
};

// Definición de variables estáticas
std::vector<String> Logger::logRingBuffer;
size_t Logger::ringHead = 0;
unsigned long Logger::totalBytesReceived = 0;
unsigned long Logger::totalBytesSent = 0;
unsigned long Logger::totalPrintJobs = 0;

#endif // LOGGER_H
