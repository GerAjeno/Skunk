#ifndef RAW_TCP_SERVER_H
#define RAW_TCP_SERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include "Config.h"
#include "Logger.h"
#include "UsbPrinterHost.h"

class RawTcpServer {
private:
    static WiFiServer* printServer;
    static TaskHandle_t tcpTaskHandle;

    static void tcpListenTask(void* arg) {
        Logger::logf("TCP_9100", "Servidor Skunk Raw JetDirect escuchando en puerto %d...", RAW_PRINT_PORT);
        uint8_t buffer[2048];

        while (true) {
            WiFiClient client = printServer->available();
            if (client) {
                IPAddress clientIp = client.remoteIP();
                Logger::logf("TCP_9100", "¡Nueva orden de impresión desde %s!", clientIp.toString().c_str());
                Logger::incrementJobs();

                unsigned long startMillis = millis();
                unsigned long totalReceived = 0;

                while (client.connected() || client.available()) {
                    int bytesAvailable = client.available();
                    if (bytesAvailable > 0) {
                        int toRead = bytesAvailable > (int)sizeof(buffer) ? sizeof(buffer) : bytesAvailable;
                        int bytesRead = client.read(buffer, toRead);
                        if (bytesRead > 0) {
                            totalReceived += bytesRead;
                            Logger::addBytesReceived(bytesRead);

                            size_t written = UsbPrinterHost::writeRawBytes(buffer, bytesRead);
                            if (written != (size_t)bytesRead) {
                                Logger::logf("TCP_9100", "ADVERTENCIA: Recibidos %d bytes pero escritos %u bytes", bytesRead, (unsigned int)written);
                            }
                        }
                    } else {
                        delay(2);
                        if (millis() - startMillis > 10000 && !client.available()) {
                            break;
                        }
                    }
                }

                client.stop();
                unsigned long elapsed = millis() - startMillis;
                Logger::logf("TCP_9100", "Trabajo finalizado: %lu bytes transferidos por USB a la Zebra en %lu ms.", totalReceived, elapsed);
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
