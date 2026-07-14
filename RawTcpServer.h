#ifndef RAW_TCP_SERVER_H
#define RAW_TCP_SERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include <vector>
#include "Config.h"
#include "Logger.h"
#include "UsbPrinterHost.h"
#include "PrintSpooler.h"

class RawTcpServer {
private:
    static WiFiServer* printServer;
    static TaskHandle_t tcpTaskHandle;

    static void tcpListenTask(void* arg) {
        Logger::logf("TCP_9100", "Servidor Skunk Raw JetDirect escuchando en puerto %d (Spooler Activo)...", RAW_PRINT_PORT);
        uint8_t buffer[2048];

        while (true) {
            WiFiClient client = printServer->available();
            if (client) {
                IPAddress clientIp = client.remoteIP();
                Logger::logf("TCP_9100", "¡Nueva conexión de impresión desde %s!", clientIp.toString().c_str());

                unsigned long startMillis = millis();
                std::vector<uint8_t> jobAccumulator;
                jobAccumulator.reserve(8192); // Preparar espacio inicial para la etiqueta

                while (client.connected() || client.available()) {
                    int bytesAvailable = client.available();
                    if (bytesAvailable > 0) {
                        int toRead = bytesAvailable > (int)sizeof(buffer) ? sizeof(buffer) : bytesAvailable;
                        int bytesRead = client.read(buffer, toRead);
                        if (bytesRead > 0) {
                            Logger::addBytesReceived(bytesRead);
                            jobAccumulator.insert(jobAccumulator.end(), buffer, buffer + bytesRead);
                        }
                    } else {
                        delay(2);
                        // Si pasan más de 1.5 segundos sin recibir datos adicionales o si el cliente cierra:
                        if (millis() - startMillis > 1500 && !client.available()) {
                            break;
                        }
                    }
                }

                client.stop();
                unsigned long elapsed = millis() - startMillis;

                if (!jobAccumulator.empty()) {
                    Logger::logf("TCP_9100", "Recepción finalizada: %d bytes de etiqueta. Pasando al Spooler en RAM/PSRAM...", (int)jobAccumulator.size());
                    bool enqueued = PrintSpooler::enqueueJob(jobAccumulator.data(), jobAccumulator.size());
                    if (!enqueued) {
                        Logger::log("TCP_9100", "❌ ERROR: No se pudo ingresar el trabajo al Spooler.");
                    }
                } else {
                    Logger::log("TCP_9100", "Conexión cerrada sin recibir datos de impresión.");
                }
            }
            vTaskDelay(pdMS_TO_TICKS(15));
        }
    }

public:
    static void init() {
        printServer = new WiFiServer(RAW_PRINT_PORT);
        printServer->begin();
        printServer->setNoDelay(true);

        xTaskCreatePinnedToCore(tcpListenTask, "tcp_printer", 8192, NULL, 3, &tcpTaskHandle, 1);
    }
};

WiFiServer* RawTcpServer::printServer = nullptr;
TaskHandle_t RawTcpServer::tcpTaskHandle = NULL;

#endif // RAW_TCP_SERVER_H
