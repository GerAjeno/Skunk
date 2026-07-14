#ifndef WEB_CONSOLE_H
#define WEB_CONSOLE_H

#include <Arduino.h>
#include <WebServer.h>
#include "Config.h"
#include "Logger.h"
#include "UsbPrinterHost.h"
#include "PrintSpooler.h"
#include "WiFiManagerSkunk.h"
#include "NotificationManager.h"

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
    <title>Skunk Industrial v2.0 - Consola de Diagnóstico</title>
    <style>
        :root {
            --bg-color: #0f172a;
            --card-bg: #1e293b;
            --text-color: #f8fafc;
            --accent-color: #3b82f6;
            --success-color: #10b981;
            --danger-color: #ef4444;
            --warning-color: #f59e0b;
            --border-color: #334155;
            --log-bg: #0b0f19;
        }
        * { box-sizing: border-box; margin: 0; padding: 0; font-family: 'Segoe UI', system-ui, -apple-system, sans-serif; }
        body { background-color: var(--bg-color); color: var(--text-color); padding: 20px; line-height: 1.5; }
        .container { max-width: 1150px; margin: 0 auto; }
        header { display: flex; justify-content: space-between; align-items: center; border-bottom: 2px solid var(--border-color); padding-bottom: 15px; margin-bottom: 25px; }
        h1 { font-size: 1.6rem; color: var(--accent-color); display: flex; align-items: center; gap: 10px; }
        .status-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(240px, 1fr)); gap: 15px; margin-bottom: 25px; }
        .card { background-color: var(--card-bg); border: 1px solid var(--border-color); border-radius: 10px; padding: 18px; box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.3); }
        .card h3 { font-size: 0.85rem; text-transform: uppercase; letter-spacing: 0.05em; color: #94a3b8; margin-bottom: 8px; }
        .card .value { font-size: 1.3rem; font-weight: 700; }
        .badge { display: inline-block; padding: 4px 10px; border-radius: 9999px; font-size: 0.8rem; font-weight: 600; }
        .badge.online { background-color: rgba(16, 185, 129, 0.2); color: var(--success-color); border: 1px solid var(--success-color); }
        .badge.offline { background-color: rgba(239, 68, 68, 0.2); color: var(--danger-color); border: 1px solid var(--danger-color); }
        .badge.warning { background-color: rgba(245, 158, 11, 0.2); color: var(--warning-color); border: 1px solid var(--warning-color); }
        .actions { display: flex; gap: 12px; margin-bottom: 25px; flex-wrap: wrap; }
        button { background-color: var(--accent-color); color: white; border: none; padding: 12px 18px; border-radius: 8px; font-weight: 600; cursor: pointer; transition: background 0.2s, transform 0.1s; display: flex; align-items: center; gap: 8px; font-size: 0.9rem; }
        button:hover { background-color: #2563eb; }
        button:active { transform: scale(0.98); }
        button.test-btn { background-color: var(--success-color); }
        button.test-btn:hover { background-color: #059669; }
        button.alert-btn { background-color: var(--warning-color); color: #0f172a; }
        button.alert-btn:hover { background-color: #d97706; color: white; }
        button.danger-btn { background-color: var(--danger-color); }
        button.danger-btn:hover { background-color: #dc2626; }
        .log-section { background-color: var(--card-bg); border: 1px solid var(--border-color); border-radius: 10px; padding: 20px; }
        .log-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 12px; }
        pre#logbox { background-color: var(--log-bg); color: #38bdf8; padding: 15px; border-radius: 8px; font-family: 'Consolas', 'Monaco', monospace; font-size: 0.88rem; height: 380px; overflow-y: auto; white-space: pre-wrap; word-break: break-all; border: 1px solid #1e293b; }
        .footer { text-align: center; margin-top: 30px; font-size: 0.8rem; color: #64748b; }
    </style>
</head>
<body>
    <div class="container">
        <header>
            <h1>🦨 Skunk v2.0 - Zebra GC420t PrintServer & Spooler RAM</h1>
            <div id="connection-status"><span class="badge online">🔴 En vivo</span></div>
        </header>

        <div class="status-grid">
            <div class="card">
                <h3>Estado Impresora USB (~HS)</h3>
                <div class="value" id="usb-detailed">Consultando...</div>
            </div>
            <div class="card">
                <h3>Spooler RAM / PSRAM</h3>
                <div class="value" id="spooler-status">0 en cola</div>
            </div>
            <div class="card">
                <h3>Trabajos Procesados</h3>
                <div class="value" id="print-jobs">0</div>
            </div>
            <div class="card">
                <h3>Tráfico Red (RX / TX)</h3>
                <div class="value" id="network-traffic">0 B / 0 B</div>
            </div>
        </div>

        <div class="actions">
            <button class="test-btn" onclick="sendTestPrint()">⚡ Enviar Etiqueta ZPL al Spooler</button>
            <button class="alert-btn" onclick="togglePaperOutAlert()">🚨 Simular Alerta Falta de Papel (~HS)</button>
            <button onclick="testTelegram()">💬 Probar Alerta Telegram</button>
            <button class="danger-btn" onclick="resetWifi()">📡 Reconfigurar Red / Portal AP</button>
        </div>

        <div class="log-section">
            <div class="log-header">
                <h3>📜 Bitácora del Sistema y Depuración Industrial</h3>
                <span style="font-size: 0.8rem; color: #94a3b8;">Refresco automático cada 2s</span>
            </div>
            <pre id="logbox">Cargando bitácora...</pre>
        </div>

        <div class="footer">
            Proyecto Skunk v2.0 • Firmware ESP32-S3 16MB • Spooler Anticolisión + Alertas IoT Telegram + LED RGB
        </div>
    </div>

    <script>
        function fetchStatus() {
            fetch('/api/status')
                .then(r => r.json())
                .then(data => {
                    const usbDiv = document.getElementById('usb-detailed');
                    if (data.paperOut) {
                        usbDiv.innerHTML = `<span class="badge offline">⚠️ FALTA PAPEL</span> <div style="font-size:0.8rem;margin-top:4px;color:#cbd5e1;">${data.printerInfo}</div>`;
                    } else if (data.usbConnected) {
                        usbDiv.innerHTML = `<span class="badge online">✅ Lista para Imprimir</span> <div style="font-size:0.8rem;margin-top:4px;color:#cbd5e1;">${data.printerInfo}</div>`;
                    } else {
                        usbDiv.innerHTML = `<span class="badge offline">❌ Desconectada</span>`;
                    }

                    const spoolerText = `${data.spoolerPending} en cola (${data.spoolerEnqueued} total)`;
                    document.getElementById('spooler-status').innerHTML = data.spoolerPending > 0 ? `<span class="badge warning">${spoolerText}</span>` : `<span style="color:#10b981;">0 en espera</span>`;
                    
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
            fetch('/api/testprint', { method: 'POST' })
                .then(r => r.text())
                .then(msg => alert(msg))
                .catch(e => alert("Error: " + e));
        }

        function togglePaperOutAlert() {
            fetch('/api/togglealert', { method: 'POST' })
                .then(r => r.text())
                .then(msg => alert("Estado modificado: " + msg))
                .catch(e => alert("Error: " + e));
        }

        function testTelegram() {
            fetch('/api/testtelegram', { method: 'POST' })
                .then(r => r.text())
                .then(msg => alert("Notificación: " + msg))
                .catch(e => alert("Error: " + e));
        }

        function resetWifi() {
            if (confirm("¿Seguro que deseas borrar las credenciales Wi-Fi de NVS y abrir el Portal Cautivo Skunk?")) {
                fetch('/api/resetwifi', { method: 'POST' })
                    .then(() => alert("El ESP32 se reiniciará en modo AP: 'Skunk-Setup-Zebra'"))
                    .catch(e => alert("Error: " + e));
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
            json += "\"paperOut\":" + String(UsbPrinterHost::getIsPaperOut() ? "true" : "false") + ",";
            json += "\"printerInfo\":\"" + UsbPrinterHost::getDetailedStatusString() + "\",";
            json += "\"spoolerPending\":" + String(PrintSpooler::getPendingJobCount()) + ",";
            json += "\"spoolerEnqueued\":" + String(PrintSpooler::getJobsEnqueued()) + ",";
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
            Logger::log("WEB_CONSOLE", "Generando etiqueta de prueba ZPL v2.0 para el Spooler en RAM/PSRAM...");
            
            const char* testZpl = 
                "^XA\r\n"
                "^PW600\r\n"
                "^LL400\r\n"
                "^FO50,50^GB500,300,4^FS\r\n"
                "^FO80,80^A0N,40,40^FDSkunk v2.0 - Industrial^FS\r\n"
                "^FO80,135^A0N,24,24^FDSpooler RAM Anticolision OK^FS\r\n"
                "^FO80,170^A0N,22,22^FDLectura Bidireccional ~HS Activa^FS\r\n"
                "^FO80,215^BY2,2,80^BCN,80,Y,N,N^FD9876543210^FS\r\n"
                "^XZ\r\n";

            size_t len = strlen(testZpl);
            bool ok = PrintSpooler::enqueueJob((const uint8_t*)testZpl, len);
            
            if (ok) {
                server->send(200, "text/plain", "Etiqueta ZPL enviada al Spooler en RAM exitosamente (" + String(len) + " bytes)");
            } else {
                server->send(500, "text/plain", "Error: Spooler lleno o impresora en estado crítico.");
            }
        });

        server->on("/api/togglealert", HTTP_POST, []() {
            bool nextState = !UsbPrinterHost::getIsPaperOut();
            UsbPrinterHost::setSimulatedAlert(nextState, false);
            String msg = nextState ? "⚠️ Simulación activa: FALTA DE PAPEL en Zebra" : "✅ Simulación desactivada: Impresora NORMAL";
            server->send(200, "text/plain", msg);
        });

        server->on("/api/testtelegram", HTTP_POST, []() {
            Logger::log("WEB_CONSOLE", "Enviando mensaje de prueba a Telegram...");
            NotificationManager::sendTelegramAlert("🦨 SKUNK v2.0: Mensaje de prueba del sistema de alertas IoT del servidor de impresión.");
            server->send(200, "text/plain", "Notificación enviada al bot de Telegram configurado.");
        });

        server->on("/api/resetwifi", HTTP_POST, []() {
            server->send(200, "text/plain", "Reiniciando en modo Portal Cautivo AP...");
            WiFiManagerSkunk::resetCredentialsAndRestart();
        });

        server->begin();
        Logger::logf("WEB_CONSOLE", "Consola Web Skunk v2.0 (Industrial) lista en puerto %d", WEB_CONSOLE_PORT);
    }

    static void handleClient() {
        if (server) {
            server->handleClient();
        }
    }
};

WebServer* WebConsole::server = nullptr;

#endif // WEB_CONSOLE_H
