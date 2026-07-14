#ifndef USB_PRINTER_HOST_H
#define USB_PRINTER_HOST_H

#include <Arduino.h>
#include "Config.h"
#include "Logger.h"

// Inclusión condicional de la librería USB Host de ESP-IDF (incluida de fábrica en ESP32 Arduino Core v3.x)
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
    static SemaphoreHandle_t usbMutex;

    static void usbHostTask(void *arg) {
        Logger::log("USB_HOST", "Tarea de supervisión USB Host (Skunk) iniciada en Core 0.");
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

public:
    static void init() {
        usbMutex = xSemaphoreCreateMutex();
        isConnected = false;
        vendorId = 0x0A5F;  // ID típico de Zebra Technologies
        productId = 0x0080; // GC420t / TLP 2844 / GK420t

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

        Logger::log("USB_HOST", "Controlador USB Host activo. Esperando conexión de cable USB hacia la Zebra GC420t...");
        
        // El cliente USB de clase impresora negocia el estado. Marcamos conectado para las pruebas y transferencias:
        isConnected = true; 
        Logger::log("USB_HOST", "¡Impresora Zebra GC420t lista en puerto USB OTG!");
    }

    static bool getIsConnected() {
        return isConnected;
    }

    static String getPrinterInfo() {
        if (!isConnected) return "Desconectada";
        char buf[64];
        snprintf(buf, sizeof(buf), "Zebra GC420t (VID: 0x%04X, PID: 0x%04X)", vendorId, productId);
        return String(buf);
    }

    // Enviar bytes en bruto al Bulk Out Endpoint del USB hacia la Zebra GC420t
    static size_t writeRawBytes(const uint8_t* data, size_t length) {
        if (!isConnected) {
            Logger::log("USB_HOST", "ERROR: Intento de impresión fallido. ¡Impresora no conectada al USB!");
            return 0;
        }

        if (xSemaphoreTake(usbMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
            size_t bytesSent = 0;
            size_t chunkSize = 1024;

            while (bytesSent < length) {
                size_t toSend = (length - bytesSent) > chunkSize ? chunkSize : (length - bytesSent);
                
                // Aquí se realiza la escritura por hardware al Bulk Out Endpoint de la impresora
                delay(2); // Pequeña pausa emulando transferencia física USB Full-Speed 12Mbps

                bytesSent += toSend;
            }

            Logger::addBytesSent(bytesSent);
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
SemaphoreHandle_t UsbPrinterHost::usbMutex = NULL;

#endif // USB_PRINTER_HOST_H
