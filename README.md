# 🦨 Proyecto Skunk v2.0 Industrial - ESP32-S3 USB Host Print Server
**Versión para Arduino IDE 2.x / Core ESP32 v3.x (Especializado para módulos de 16MB Flash + PSRAM)**

Este proyecto transforma un módulo **ESP32-S3** en un servidor de impresión de grado industrial (**Raw TCP Port 9100 + USB Host OTG**) especializado en impresoras térmicas **Zebra GC420t** y optimizado para despachos masivos desde teléfonos **Android**.

---

## 🌟 Novedades e Innovaciones en la versión 2.0 Industrial

1. **🔍 Lectura Bidireccional Inteligente (`~HS` Host Status ZPL)**: El ESP32 consulta en segundo plano el estado de los sensores internos de la Zebra. Detecta instantáneamente **falta de papel / etiquetas agotadas (`Paper Out`)**, **tapa del cabezal térmico abierta (`Head Open`)** o desconexión del cable USB.
2. **🌐 Portal Cautivo Wi-Fi con Memoria NVS (Zero-Code Setup)**: Si el router cambia de clave o mueves la impresora a otro almacén, el ESP32 levanta automáticamente la red Wi-Fi `Skunk-Setup-Zebra`. Te conectas con tu celular a `http://192.168.4.1/`, seleccionas tu Wi-Fi, guardas y ¡listo!
3. **🚦 Micro-Cola de Impresión (Spooler RAM/PSRAM) Anticolisión**: Si dos o más celulares Android imprimen exactamente al mismo tiempo o si la impresora se queda sin papel en medio de una orden, los trabajos se encolan ordenadamente en la memoria de alta velocidad del módulo de 16MB sin rechazar conexiones ni perder datos.
4. **🔔 Alertas IoT por Telegram & LED Neopixel WS2812**: 
   * Envía notificaciones Push SSL/TLS a tu celular o al supervisor si una impresora se queda sin papel (`⚠️ ALERTA SKUNK: La impresora Zebra se ha quedado sin papel`).
   * Iluminación de estado en el LED RGB de la placa (🟢 Verde = OK, 🔵 Azul = Imprimiendo, 🟡 Amarillo = Modo Portal Cautivo, 🔴 Rojo Parpadeante = Falta de papel).

---

## 📂 Estructura del Proyecto en Arduino IDE

| Archivo / Pestaña | Descripción |
| :--- | :--- |
| **`Skunk.ino`** | Sketch principal. Orquesta los motores del Spooler, Portal Cautivo, USB Host y Telegram. |
| **`Config.h`** | Parámetros por defecto de red, pines USB OTG (`DP=20, DM=19`), LED Neopixel (`PIN=48`) y capacidad de cola. |
| **`WiFiManagerSkunk.h`** | Motor del Portal Cautivo NVS (`http://192.168.4.1/`) y auto-reconexión. |
| **`PrintSpooler.h`** | Micro-cola de impresión y seguro anticolisión en memoria PSRAM / RAM. |
| **`UsbPrinterHost.h`** | Controlador USB OTG con sensor bidireccional ZPL (`~HS`). |
| **`NotificationManager.h`** | Cliente SSL/TLS para disparar alertas inmediatas a Telegram Bot API. |
| **`StatusIndicators.h`** | Animaciones LED RGB Neopixel WS2812 y alertas acústicas. |
| **`RawTcpServer.h`** | Servidor JetDirect en puerto `9100` conectado al Spooler. |
| **`WebConsole.h`** | Panel web de diagnóstico industrial, botones de simulación y monitoreo circular cada 2s (`http://<ip-esp32>/`). |
| **`Logger.h`** | Bitácora en RAM en tiempo real. |

---

## ⚙️ Configuración en Arduino IDE 2.x

1. Selecciona en **Tools > Board**: **`ESP32S3 Dev Module`** *(o `ESP32-S3-DevKitC-1`)*.
2. Opciones de hardware recomendadas para el módulo de 16MB:
   * **USB Mode**: `USB OTG Mode: Host` *(Crucial para activar el USB Host D+/D-)*
   * **USB CDC On Boot**: `Enabled`
   * **Partition Scheme**: `16MB Flash (3MB APP/9.9MB FATFS/LittleFS)` o `8MB with Spiffs (16MB Flash)`
   * **PSRAM**: `OPI PSRAM` o `QSPI PSRAM` *(según tu módulo N16R8)*
3. Haz clic en **Upload (Subir)**.

---

## 📱 Cómo Imprimir desde Celulares Android
1. Instala el app **Zebra Print Service Plugin** desde Google Play Store.
2. Añade tu impresora con la IP del ESP32 y el puerto `9100` (ej. `192.168.1.201:9100`).
3. Abre Chrome o tu App Web en Android y dale a **Imprimir**. ¡El Spooler de Skunk se encargará del resto!
