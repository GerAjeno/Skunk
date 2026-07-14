# 🦨 Proyecto Skunk - ESP32-S3 USB Host Print Server para Zebra GC420t
**Versión para Arduino IDE 2.x / Core ESP32 v3.x**

Este proyecto transforma un módulo **ESP32-S3** en un servidor de red Wi-Fi (**Raw TCP Port 9100 Bridge**) con **USB Host OTG** para conectar impresoras térmicas **Zebra GC420t**. 

Incluye un panel web de diagnóstico con **sistema de LOGS circular en tiempo real (`http://192.168.1.201/`)** para depurar problemas físicos o de red cuando la impresora esté en el almacén.

---

## 📂 Estructura del Proyecto para Arduino IDE
Para que Arduino IDE reconozca el proyecto sin errores, la carpeta debe llamarse exactamente **`Skunk`** y contener:
* `Skunk.ino` (Sketch principal)
* `Config.h` (Parámetros de red Wi-Fi, IP fija, puertos y pines USB)
* `Logger.h` (Motor de logs y búfer en RAM)
* `UsbPrinterHost.h` (Controlador USB Host OTG)
* `RawTcpServer.h` (Servidor en puerto TCP 9100)
* `WebConsole.h` (Interfaz web oscura de diagnóstico)

Al abrir `Skunk.ino` en Arduino IDE, todos los archivos `.h` aparecerán como pestañas en la parte superior.

---

## ⚙️ Configuración Exacta en Arduino IDE

1. Asegúrate de tener instalado el paquete de tarjetas **esp32 by Espressif Systems (v3.0.0 o superior)** desde el *Boards Manager*.
2. Ve al menú **Tools (Herramientas)** y selecciona:
   * **Board (Tarjeta)**: `ESP32S3 Dev Module` (o `ESP32-S3-DevKitC-1`)
   * **USB Mode**: `Hardware CDC and JTAG` (o `USB OTG Mode: Host`)
   * **USB CDC On Boot**: `Enabled`
   * **Partition Scheme**: `Default 4MB with spiffs`
   * **Core Debug Level**: `Info` o `Debug`
3. Abre la pestaña `Config.h` y coloca tus credenciales de Wi-Fi:
   ```cpp
   #define WIFI_SSID     "Mi_Red_Empresa"
   #define WIFI_PASSWORD "ClaveSecreta123"
   ```
4. Conecta tu ESP32-S3 al puerto USB de la PC y presiona **Upload (Subir)**.

---

## 🔌 Conexiones Físicas USB OTG (ESP32-S3 a Zebra GC420t)

En los módulos **ESP32-S3 DevKitC** estándar, el puerto USB OTG está asignado a:

| Pin ESP32-S3 | Señal USB | Color Cable USB |
| :--- | :--- | :--- |
| **GPIO 20** | **USB D+** | Verde |
| **GPIO 19** | **USB D-** | Blanco |
| **GND** | **GND** | Negro |
| **5V / VBUS** | **+5V** | Rojo |

> [!NOTE]
> Si utilizas una placa ESP32-S3 con dos conectores USB Tipo-C (uno rotulado `UART` / `COM` y otro `USB` / `OTG`), puedes conectar la impresora directamente en el conector `USB` usando un cable o adaptador OTG sin soldar cables.

---

## 📱 Cómo Imprimir desde Celulares Android

1. En cada celular Android, instala desde Google Play Store el **"Zebra Print Service Plugin"**.
2. Abre el plugin, pulsa los 3 puntos y selecciona **Añadir impresora mediante dirección IP**.
3. Escribe la IP fija de tu ESP32-S3 y puerto `9100` (ej. `192.168.1.201:9100`).
4. Al entrar a tu App Web o Chrome en el celular y darle a **Imprimir**, aparecerá tu impresora en el menú desplegable nativo de Android.
