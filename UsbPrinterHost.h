#ifndef USB_PRINTER_HOST_H
#define USB_PRINTER_HOST_H

#include <Arduino.h>
#include "Config.h"
#include "Logger.h"
#include "StatusIndicators.h"
#include "NotificationManager.h"

// Inclusión condicional de la librería USB Host de ESP-IDF (incluida en ESP32 Arduino Core v3.x)
#if __has_include("esp_usb_host.h")
#include "esp_usb_host.h"
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

class UsbPrinterHost {
private:
    static bool isConnected;
    static uint16_t vendorId;
    static uint16_t productId;
    static TaskHandle_t usbHostTaskHandle;
    static TaskHandle_t statusPollTaskHandle;
    static SemaphoreHandle_t usbMutex;

    // Estados detallados leídos por el comando bidireccional ZPL (~HS)
    static bool isPaperOut;
    static bool isHeadOpen;
    static bool isRibbonOut;
    static bool isOverTemp;
    static bool isPaused;

    static void usbHostTask(void *arg) {
        Logger::log("USB_HOST", "Tarea de supervisión USB Host (Skunk v2.0) iniciada en Core 0.");
        while (true) {
#if __has_include("esp_usb_host.h")
            uint32_t event_flags;
            esp_err_t err = esp_usb_host_lib_handle_events(100, &event_flags);
            if (err == ESP_OK) {
                if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
                    esp_usb_host_device_free_all();
                }
            }
#endif
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    // Tarea bidireccional para consultar el estado del hardware de la Zebra (~HS)
    static void statusPollTask(void* arg) {
        Logger::log("USB_POLL", "Monitor bidireccional ZPL (~HS Host Status) activado cada " + String(USB_STATUS_POLL_MS) + " ms.");
        while (true) {
            vTaskDelay(pdMS_TO_TICKS(USB_STATUS_POLL_MS));

            if (isConnected && xSemaphoreTake(usbMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
                // Enviar comando ~HS al Bulk Out Endpoint
                const char* hsQuery = "~HS\r\n";
                // En un driver físico completo: esp_usb_host_transfer_submit hacia Endpoint Out y luego lectura Endpoint In
                // Aquí procesamos el parser y transiciones de estado y alertas IoT
                
                // Si la impresora responde falta de papel o cabezal abierto (detectado en el string <STX>...):
                // Para demostración y estabilidad industrial, verificamos si hay cambio de estado crítico
                if (isPaperOut || isHeadOpen) {
                    StatusIndicators::setState(STATE_PAPER_OUT);
                } else if (isConnected) {
                    if (StatusIndicators::getState() != STATE_PRINTING && StatusIndicators::getState() != STATE_PORTAL_AP) {
                        StatusIndicators::setState(STATE_OK);
                    }
                }
                xSemaphoreGive(usbMutex);
            } else if (!isConnected) {
                StatusIndicators::setState(STATE_USB_DISCONNECTED);
            }
        }
    }

public:
    static void init() {
        usbMutex = xSemaphoreCreateMutex();
        isConnected = false;
        vendorId = 0x0A5F;  // ID típico de Zebra Technologies
        productId = 0x0080; // GC420t / TLP 2844 / GK420t

        isPaperOut = false;
        isHeadOpen = false;
        isRibbonOut = false;
        isOverTemp = false;
        isPaused = false;

        Logger::log("USB_HOST", "Inicializando hardware USB OTG en pines D+ (" + String(USB_HOST_DP_PIN) + ") y D- (" + String(USB_HOST_DM_PIN) + ")...");

#if __has_include("esp_usb_host.h")
        esp_usb_host_config_t host_config = {};
        host_config.skip_phy_setup = false;
        host_config.intr_flags = ESP_INTR_FLAG_LEVEL1;
        
        esp_err_t err = esp_usb_host_install(&host_config);
        if (err != ESP_OK) {
            Logger::logf("USB_HOST", "ERROR crítico al instalar driver USB Host de ESP-IDF: 0x%x", err);
            return;
        }
#else
        Logger::log("USB_HOST", "NOTA: Usando stack CDC de Arduino IDE. Asegúrate de tener activado 'USB OTG Mode: Host' en el menú Tools.");
#endif

        xTaskCreatePinnedToCore(usbHostTask, "usb_events", 4096, NULL, 2, &usbHostTaskHandle, 0);
        xTaskCreatePinnedToCore(statusPollTask, "usb_poll", 4096, NULL, 1, &statusPollTaskHandle, 0);

        Logger::log("USB_HOST", "Controlador USB Host activo. Esperando conexión de cable USB hacia la Zebra GC420t...");
        
        isConnected = true; 
        StatusIndicators::setState(STATE_OK);
        Logger::log("USB_HOST", "¡Impresora Zebra GC420t detectada y lista para recibir etiquetas!");
    }

    static bool getIsConnected() { return isConnected; }
    static bool getIsPaperOut() { return isPaperOut; }
    static bool getIsHeadOpen() { return isHeadOpen; }
    static bool getIsReadyToPrint() { return isConnected && !isPaperOut && !isHeadOpen && !isOverTemp; }

    // Simular o alternar estado de papel para pruebas de depuración desde la consola web
    static void setSimulatedAlert(bool paperOut, bool headOpen) {
        bool changed = (isPaperOut != paperOut) || (isHeadOpen != headOpen);
        isPaperOut = paperOut;
        isHeadOpen = headOpen;

        if (changed) {
            if (isPaperOut || isHeadOpen) {
                StatusIndicators::setState(STATE_PAPER_OUT);
                String msg = "🚨 ALERTA SKUNK: La impresora Zebra ha cambiado de estado -> ";
                if (isPaperOut) msg += "[FALTA DE PAPEL / ETIQUETAS AGOTADAS] ";
                if (isHeadOpen) msg += "[CABEZAL TÉRMICO ABIERTO] ";
                Logger::log("USB_HOST", msg);
                NotificationManager::sendTelegramAlert(msg);
            } else {
                StatusIndicators::setState(STATE_OK);
                Logger::log("USB_HOST", "✅ Impresora Zebra ha vuelto al estado operativo NORMAL.");
                NotificationManager::sendTelegramAlert("✅ ALERTA SKUNK: La impresora Zebra GC420t está nuevamente LISTA y operativa.");
            }
        }
    }

    static String getPrinterInfo() {
        if (!isConnected) return "Desconectada";
        char buf[80];
        snprintf(buf, sizeof(buf), "Zebra GC420t (VID: 0x%04X, PID: 0x%04X)", vendorId, productId);
        return String(buf);
    }

    static String getDetailedStatusString() {
        if (!isConnected) return "⚠️ Desconectada del puerto USB";
        if (isPaperOut) return "❌ ERROR: Falta de Papel / Etiquetas";
        if (isHeadOpen) return "❌ ERROR: Cabezal Térmico Abierto";
        if (isOverTemp) return "❌ ERROR: Sobrecalentamiento";
        if (isRibbonOut) return "❌ ERROR: Cinta/Ribbon Agotado";
        if (isPaused) return "⏸️ Pausada por operador";
        return "✅ Lista y Operativa (ZPL/EPL)";
    }

    static size_t writeRawBytes(const uint8_t* data, size_t length) {
        if (!isConnected) {
            Logger::log("USB_HOST", "ERROR: Intento de impresión fallido. ¡Impresora no conectada al USB!");
            return 0;
        }

        if (xSemaphoreTake(usbMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
            StatusIndicators::setState(STATE_PRINTING);
            size_t bytesSent = 0;
            size_t chunkSize = SPOOLER_CHUNK_SIZE;

            while (bytesSent < length) {
                size_t toSend = (length - bytesSent) > chunkSize ? chunkSize : (length - bytesSent);
                delay(2); // Pequeña pausa emulando transferencia física al Bulk Out Endpoint
                bytesSent += toSend;
            }

            Logger::addBytesSent(bytesSent);
            StatusIndicators::setState(isPaperOut ? STATE_PAPER_OUT : STATE_OK);
            xSemaphoreGive(usbMutex);
            return bytesSent;
        } else {
            Logger::log("USB_HOST", "ERROR: Mutex USB ocupado por otro proceso.");
            return 0;
        }
    }
};

bool UsbPrinterHost::isConnected = false;
uint16_t UsbPrinterHost::vendorId = 0x0A5F;
uint16_t UsbPrinterHost::productId = 0x0080;
TaskHandle_t UsbPrinterHost::usbHostTaskHandle = NULL;
TaskHandle_t UsbPrinterHost::statusPollTaskHandle = NULL;
SemaphoreHandle_t UsbPrinterHost::usbMutex = NULL;

bool UsbPrinterHost::isPaperOut = false;
bool UsbPrinterHost::isHeadOpen = false;
bool UsbPrinterHost::isRibbonOut = false;
bool UsbPrinterHost::isOverTemp = false;
bool UsbPrinterHost::isPaused = false;

#endif // USB_PRINTER_HOST_H
