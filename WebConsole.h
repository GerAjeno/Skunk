#ifndef WEB_CONSOLE_H
#define WEB_CONSOLE_H

#include <Arduino.h>
#include <WebServer.h>
#include "Config.h"
#include "Logger.h"
#include "UsbPrinterHost.h"

class WebConsole {
private:
    static WebServer* server;

    static const char* getIndexHtml() {
        return R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Skunk Print Server - Consola de Diagnóstico</title>
    <style>
        :root {
            --bg-color: #0f172a;
            --card-bg: #1e293b;
            --text-color: #f8fafc;
            --accent-color: #3b82f6;
            --success-color: #10b981;
            --danger-color: #ef4444;
            --border-color: #334155;
            --log-bg: #0b0f19;
        }
        * { box-sizing: border-box; margin: 0; padding: 0; font-family: 'Segoe UI', system-ui, -apple-system, sans-serif; }
        body { background-color: var(--bg-color); color: var(--text-color); padding: 20px; line-height: 1.5; }
        .container { max-width: 1100px; margin: 0 auto; }
        header { display: flex; justify-content: space-between; align-items: center; border-bottom: 2px solid var(--border-color); padding-bottom: 15px; margin-bottom: 25px; }
        h1 { font-size: 1.6rem; color: var(--accent-color); display: flex; align-items: center; gap: 10px; }
        .status-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(220px, 1fr)); gap: 15px; margin-bottom: 25px; }
        .card { background-color: var(--card-bg); border: 1px solid var(--border-color); border-radius: 10px; padding: 18px; box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.3); }
        .card h3 { font-size: 0.85rem; text-transform: uppercase; letter-spacing: 0.05em; color: #94a3b8; margin-bottom: 8px; }
        .card .value { font-size: 1.4rem; font-weight: 700; }
        .badge { display: inline-block; padding: 4px 10px; border-radius: 9999px; font-size: 0.8rem; font-weight: 600; }
        .badge.online { background-color: rgba(16, 185, 129, 0.2); color: var(--success-color); border: 1px solid var(--success-color); }
        .badge.offline { background-color: rgba(239, 68, 68, 0.2); color: var(--danger-color); border: 1px solid var(--danger-color); }
        .actions { display: flex; gap: 12px; margin-bottom: 25px; flex-wrap: wrap; }
        button { background-color: var(--accent-color); color: white; border: none; padding: 12px 20px; border-radius: 8px; font-weight: 600; cursor: pointer; transition: background 0.2s, transform 0.1s; display: flex; align-items: center; gap: 8px; }
        button:hover { background-color: #2563eb; }
        button:active { transform: scale(0.98); }
        button.test-btn { background-color: var(--success-color); }
        button.test-btn:hover { background-color: #059669; }
        .log-section { background-color: var(--card-bg); border: 1px solid var(--border-color); border-radius: 10px; padding: 20px; }
        .log-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 12px; }
        pre#logbox { background-color: var(--log-bg); color: #38bdf8; padding: 15px; border-radius: 8px; font-family: 'Consolas', 'Monaco', monospace; font-size: 0.9rem; height: 380px; overflow-y: auto; white-space: pre-wrap; word-break: break-all; border: 1px solid #1e293b; }
        .footer { text-align: center; margin-top: 30px; font-size: 0.8rem; color: #64748b; }
    </style>
</head>
<body>
    <div class="container">
        <header>
            <h1>🦨 Skunk - Zebra GC420t PrintServer (ESP32-S3)</h1>
            <div id="connection-status"><span class="badge online">🔴 En vivo</span></div>
        </header>

        <div class="status-grid">
            <div class="card">
                <h3>Estado Impresora USB</h3>
                <div class="value" id="usb-status">Consultando...</div>
            </div>
            <div class="card">
                <h3>Puerto TCP RAW (JetDirect)</h3>
                <div class="value" style="color: var(--success-color);">Puerto 9100</div>
            </div>
            <div class="card">
                <h3>Trabajos Recibidos</h3>
                <div class="value" id="print-jobs">0</div>
            </div>
            <div class="card">
                <h3>Tráfico Total (RX / TX)</h3>
                <div class="value" id="network-traffic">0 B / 0 B</div>
            </div>
        </div>

        <div class="actions">
            <button class="test-btn" onclick="sendTestPrint()">⚡ Enviar Etiqueta ZPL de Prueba al USB</button>
            <button onclick="fetchStatus()">🔄 Actualizar Estado</button>
        </div>

        <div class="log-section">
            <div class="log-header">
                <h3>📜 Logs del Sistema en Tiempo Real (Depuración)</h3>
                <span style="font-size: 0.8rem; color: #94a3b8;">Auto-actualización cada 2s</span>
            </div>
            <pre id="logbox">Cargando registros...</pre>
        </div>

        <div class="footer">
            Proyecto Skunk • Firmware ESP32-S3 USB Host • Compatible con Zebra Print Service Plugin & Arduino IDE
        </div>
    </div>

    <script>
        function fetchStatus() {
            fetch('/api/status')
                .then(r => r.json())
                .then(data => {
                    const usbDiv = document.getElementById('usb-status');
                    if (data.usbConnected) {
                        usbDiv.innerHTML = `<span class="badge online">Conectada</span> <span style="font-size:0.85rem; color:#94a3b8;">${data.printerInfo}</span>`;
                    } else {
                        usbDiv.innerHTML = `<span class="badge offline">Desconectada</span>`;
                    }
                    document.getElementById('print-jobs').innerText = data.jobs;
                    document.getElementById('network-traffic').innerText = formatBytes(data.rx) + ' / ' + formatBytes(data.tx);
                })
                .catch(e => console.error('Error estatus:', e));
        }

        function fetchLogs() {
            fetch('/api/logs')
                .then(r => r.json())
                .then(logs => {
                    const logbox = document.getElementById('logbox');
                    if (Array.isArray(logs)) {
                        logbox.innerText = logs.join('\n');
                        logbox.scrollTop = logbox.scrollHeight;
                    }
                })
                .catch(e => console.error('Error logs:', e));
        }

        function sendTestPrint() {
            if (confirm("¿Deseas enviar una etiqueta de prueba ZPL nativa a la impresora USB?")) {
                fetch('/api/testprint', { method: 'POST' })
                    .then(r => r.text())
                    .then(msg => alert("Resultado: " + msg))
                    .catch(e => alert("Error en envío: " + e));
            }
        }

        function formatBytes(bytes) {
            if (bytes === 0) return '0 B';
            const k = 1024;
            const sizes = ['B', 'KB', 'MB'];
            const i = Math.floor(Math.log(bytes) / Math.log(k));
            return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
        }

        setInterval(() => { fetchStatus(); fetchLogs(); }, 2000);
        fetchStatus();
        fetchLogs();
    </script>
</body>
</html>
        )rawliteral";
    }

public:
    static void init() {
        server = new WebServer(WEB_CONSOLE_PORT);

        server->on("/", HTTP_GET, []() {
            server->send(200, "text/html; charset=utf-8", getIndexHtml());
        });

        server->on("/api/status", HTTP_GET, []() {
            String json = "{";
            json += "\"usbConnected\":" + String(UsbPrinterHost::getIsConnected() ? "true" : "false") + ",";
            json += "\"printerInfo\":\"" + UsbPrinterHost::getPrinterInfo() + "\",";
            json += "\"jobs\":" + String(Logger::getPrintJobs()) + ",";
            json += "\"rx\":" + String(Logger::getBytesReceived()) + ",";
            json += "\"tx\":" + String(Logger::getBytesSent());
            json += "}";
            server->send(200, "application/json", json);
        });

        server->on("/api/logs", HTTP_GET, []() {
            server->send(200, "application/json", Logger::getLogsAsJson());
        });

        server->on("/api/testprint", HTTP_POST, []() {
            Logger::log("WEB_CONSOLE", "Enviando etiqueta de prueba ZPL nativa al puerto USB...");
            
            const char* testZpl = 
                "^XA\r\n"
                "^PW600\r\n"
                "^LL400\r\n"
                "^FO50,50^GB500,300,4^FS\r\n"
                "^FO80,80^A0N,40,40^FDProyecto Skunk - OK^FS\r\n"
                "^FO80,140^A0N,25,25^FDESP32-S3 USB Host PrintServer^FS\r\n"
                "^FO80,180^A0N,20,20^FDPuerto TCP RAW: 9100^FS\r\n"
                "^FO80,220^BY2,2,80^BCN,80,Y,N,N^FD1234567890^FS\r\n"
                "^XZ\r\n";

            size_t len = strlen(testZpl);
            size_t written = UsbPrinterHost::writeRawBytes((const uint8_t*)testZpl, len);
            
            if (written == len) {
                Logger::log("WEB_CONSOLE", "¡Etiqueta de prueba enviada exitosamente al USB!");
                server->send(200, "text/plain", "Etiqueta ZPL enviada correctamente al USB (" + String(written) + " bytes)");
            } else {
                Logger::log("WEB_CONSOLE", "ERROR: Falló el envío al USB.");
                server->send(500, "text/plain", "Error enviando al USB. Verifica la conexión.");
            }
        });

        server->begin();
        Logger::logf("WEB_CONSOLE", "Servidor Web de diagnóstico Skunk activo en puerto %d", WEB_CONSOLE_PORT);
    }

    static void handleClient() {
        if (server) {
            server->handleClient();
        }
    }
};

WebServer* WebConsole::server = nullptr;

#endif // WEB_CONSOLE_H
