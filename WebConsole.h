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
#include "SkunkIcon.h"

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
    <title>Skunk Industrial v2.0 - Consola AMOLED</title>
    <link rel="icon" type="image/png" href="/logo.png">
    <link rel="apple-touch-icon" href="/logo.png">
    <meta name="theme-color" content="#000000">
    <style>
        :root {
            --bg-color: #000000;         /* AMOLED True Black */
            --card-bg: #0b0f19;          /* Deep Carbon / OLED contrast */
            --card-border: #1e293b;
            --text-color: #f8fafc;
            --text-muted: #94a3b8;
            --accent-blue: #3b82f6;
            --accent-glow: rgba(59, 130, 246, 0.35);
            --success-color: #10b981;
            --success-glow: rgba(16, 185, 129, 0.35);
            --danger-color: #ef4444;
            --danger-glow: rgba(239, 68, 68, 0.4);
            --warning-color: #f59e0b;
            --log-bg: #05070c;
        }
        * { box-sizing: border-box; margin: 0; padding: 0; font-family: 'Segoe UI', system-ui, -apple-system, sans-serif; }
        body { background-color: var(--bg-color); color: var(--text-color); padding: 20px; line-height: 1.5; min-height: 100vh; }
        .container { max-width: 1180px; margin: 0 auto; }
        
        header.skunk-header { 
            display: flex; justify-content: space-between; align-items: center; 
            background: linear-gradient(145deg, #0f172a, #0b0f19);
            border: 1px solid var(--card-border);
            border-radius: 16px; padding: 16px 24px; margin-bottom: 24px;
            box-shadow: 0 8px 30px rgba(0, 0, 0, 0.9);
        }
        .brand { display: flex; align-items: center; gap: 16px; }
        .app-icon { width: 56px; height: 56px; border-radius: 12px; border: 2px solid #3b82f6; box-shadow: 0 0 15px var(--accent-glow); object-fit: cover; }
        h1 { font-size: 1.6rem; font-weight: 800; background: linear-gradient(90deg, #60a5fa, #38bdf8); -webkit-background-clip: text; -webkit-text-fill-color: transparent; }
        .subtitle { font-size: 0.82rem; color: var(--text-muted); font-weight: 500; letter-spacing: 0.05em; text-transform: uppercase; }
        
        .status-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 16px; margin-bottom: 24px; }
        .card { 
            background-color: var(--card-bg); border: 1px solid var(--card-border); border-radius: 14px; 
            padding: 20px; box-shadow: 0 6px 20px rgba(0, 0, 0, 0.8); transition: transform 0.2s, border-color 0.2s;
        }
        .card:hover { border-color: var(--accent-blue); transform: translateY(-2px); }
        .card h3 { font-size: 0.82rem; text-transform: uppercase; letter-spacing: 0.08em; color: var(--text-muted); margin-bottom: 10px; }
        .card .value { font-size: 1.35rem; font-weight: 700; display: flex; align-items: center; gap: 10px; }
        
        .badge { display: inline-flex; align-items: center; gap: 6px; padding: 5px 12px; border-radius: 9999px; font-size: 0.82rem; font-weight: 700; }
        .badge.online { background-color: rgba(16, 185, 129, 0.15); color: var(--success-color); border: 1px solid var(--success-color); box-shadow: 0 0 10px var(--success-glow); }
        .badge.offline { background-color: rgba(239, 68, 68, 0.15); color: var(--danger-color); border: 1px solid var(--danger-color); box-shadow: 0 0 10px var(--danger-glow); }
        .badge.warning { background-color: rgba(245, 158, 11, 0.15); color: var(--warning-color); border: 1px solid var(--warning-color); }
        
        .actions { display: grid; grid-template-columns: repeat(auto-fit, minmax(220px, 1fr)); gap: 14px; margin-bottom: 24px; }
        button { 
            background-color: #1e293b; color: white; border: 1px solid var(--card-border); 
            padding: 14px 18px; border-radius: 12px; font-weight: 600; cursor: pointer; 
            transition: all 0.2s ease; display: flex; align-items: center; justify-content: center; gap: 10px; font-size: 0.9rem;
            box-shadow: 0 4px 12px rgba(0,0,0,0.5);
        }
        button:hover { background-color: #334155; border-color: var(--accent-blue); box-shadow: 0 0 15px var(--accent-glow); }
        button:active { transform: scale(0.97); }
        button.test-btn { background-color: rgba(16, 185, 129, 0.2); border-color: var(--success-color); color: #34d399; }
        button.test-btn:hover { background-color: var(--success-color); color: #000000; box-shadow: 0 0 18px var(--success-glow); }
        button.alert-btn { background-color: rgba(245, 158, 11, 0.2); border-color: var(--warning-color); color: #fbbf24; }
        button.alert-btn:hover { background-color: var(--warning-color); color: #000000; }
        button.danger-btn { background-color: rgba(239, 68, 68, 0.2); border-color: var(--danger-color); color: #f87171; }
        button.danger-btn:hover { background-color: var(--danger-color); color: #ffffff; box-shadow: 0 0 18px var(--danger-glow); }
        
        .log-section { background-color: var(--card-bg); border: 1px solid var(--card-border); border-radius: 14px; padding: 22px; box-shadow: 0 8px 25px rgba(0, 0, 0, 0.9); }
        .log-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 14px; }
        pre#logbox { 
            background-color: var(--log-bg); color: #38bdf8; padding: 18px; border-radius: 10px; 
            font-family: 'Consolas', 'Monaco', monospace; font-size: 0.88rem; height: 390px; 
            overflow-y: auto; white-space: pre-wrap; word-break: break-all; border: 1px solid #1e293b; 
        }
        pre#logbox::-webkit-scrollbar { width: 8px; }
        pre#logbox::-webkit-scrollbar-track { background: var(--log-bg); }
        pre#logbox::-webkit-scrollbar-thumb { background: #334155; border-radius: 4px; }
        
        .footer { text-align: center; margin-top: 35px; font-size: 0.82rem; color: #475569; padding-bottom: 20px; }
    </style>
</head>
<body>
    <div class="container">
        <header class="skunk-header">
            <div class="brand">
                <img src="/logo.png" alt="Skunk Logo" class="app-icon">
                <div>
                    <h1>Skunk v2.0 <span style="font-size:1rem; color:#10b981;">● AMOLED</span></h1>
                    <div class="subtitle">Zebra GC420t • USB Host & Spooler RAM</div>
                </div>
            </div>
            <div id="connection-status"><span class="badge online">🔴 En vivo (9100/80)</span></div>
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
                <h3>📜 Bitácora de Sistema Industrial (Modo Oscuro AMOLED)</h3>
                <span style="font-size: 0.8rem; color: #64748b;">Refresco automático cada 2s</span>
            </div>
            <pre id="logbox">Cargando bitácora del servidor Skunk...</pre>
        </div>

        <div class="footer">
            Proyecto Skunk v2.0 • Modo AMOLED True Black (#000000) • ESP32-S3 16MB • Zebra GC420t
        </div>
    </div>

    <script>
        function fetchStatus() {
            fetch('/api/status')
                .then(r => r.json())
                .then(data => {
                    const usbDiv = document.getElementById('usb-detailed');
                    if (data.paperOut) {
                        usbDiv.innerHTML = `<span class="badge offline">⚠️ FALTA PAPEL</span> <div style="font-size:0.8rem;margin-top:4px;color:#94a3b8;">${data.printerInfo}</div>`;
                    } else if (data.usbConnected) {
                        usbDiv.innerHTML = `<span class="badge online">✅ Lista para Imprimir</span> <div style="font-size:0.8rem;margin-top:4px;color:#94a3b8;">${data.printerInfo}</div>`;
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

        // Sirviendo iconos de la app Skunk
        server->on("/logo.png", HTTP_GET, []() {
            server->sendHeader("Cache-Control", "public, max-age=86400");
            server->send_P(200, "image/png", SKUNK_ICON_PNG, SKUNK_ICON_PNG_LEN);
        });
        server->on("/favicon.ico", HTTP_GET, []() {
            server->sendHeader("Cache-Control", "public, max-age=86400");
            server->send_P(200, "image/png", SKUNK_ICON_PNG, SKUNK_ICON_PNG_LEN);
        });
        server->on("/apple-touch-icon.png", HTTP_GET, []() {
            server->sendHeader("Cache-Control", "public, max-age=86400");
            server->send_P(200, "image/png", SKUNK_ICON_PNG, SKUNK_ICON_PNG_LEN);
        });

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
        Logger::logf("WEB_CONSOLE", "Consola Web AMOLED Skunk v2.0 lista en puerto %d", WEB_CONSOLE_PORT);
    }

    static void handleClient() {
        if (server) {
            server->handleClient();
        }
    }
};

WebServer* WebConsole::server = nullptr;

#endif // WEB_CONSOLE_H
