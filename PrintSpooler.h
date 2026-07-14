#ifndef PRINT_SPOOLER_H
#define PRINT_SPOOLER_H

#include <Arduino.h>
#include <vector>
#include "Config.h"
#include "Logger.h"
#include "UsbPrinterHost.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

struct SpoolerJob {
    uint8_t* buffer;
    size_t length;
    unsigned long timestamp;
};

class PrintSpooler {
private:
    static std::vector<SpoolerJob> jobQueue;
    static SemaphoreHandle_t queueMutex;
    static TaskHandle_t spoolerTaskHandle;
    static unsigned long jobsEnqueued;
    static unsigned long jobsProcessed;

    static void spoolerTask(void* arg) {
        Logger::log("SPOOLER", "Motor del Spooler en RAM/PSRAM iniciado. Listo para encolar hasta " + String(SPOOLER_MAX_JOBS) + " trabajos.");
        while (true) {
            bool hasJobs = false;

            if (xSemaphoreTake(queueMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                hasJobs = !jobQueue.empty();
                xSemaphoreGive(queueMutex);
            }

            if (hasJobs) {
                // Verificar si la impresora está en estado óptimo para imprimir (!isPaperOut, conectada)
                if (UsbPrinterHost::getIsReadyToPrint()) {
                    SpoolerJob currentJob;
                    bool jobReady = false;

                    if (xSemaphoreTake(queueMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                        if (!jobQueue.empty()) {
                            currentJob = jobQueue.front();
                            jobReady = true;
                        }
                        xSemaphoreGive(queueMutex);
                    }

                    if (jobReady) {
                        Logger::logf("SPOOLER", "Despachando trabajo de la cola (%u bytes). Quedan %d trabajos en espera.", (unsigned int)currentJob.length, (int)jobQueue.size() - 1);
                        size_t written = UsbPrinterHost::writeRawBytes(currentJob.buffer, currentJob.length);

                        if (written == currentJob.length) {
                            // Liberar memoria RAM / PSRAM del trabajo procesado
                            if (currentJob.buffer) free(currentJob.buffer);

                            if (xSemaphoreTake(queueMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                                if (!jobQueue.empty()) jobQueue.erase(jobQueue.begin());
                                jobsProcessed++;
                                xSemaphoreGive(queueMutex);
                            }
                            Logger::log("SPOOLER", "Trabajo completado y removido de la cola con éxito.");
                        } else {
                            Logger::log("SPOOLER", "ADVERTENCIA: Transferencia USB incompleta. Reintentando en el próximo ciclo.");
                            vTaskDelay(pdMS_TO_TICKS(2000));
                        }
                    }
                } else {
                    // La impresora está sin papel o desconectada. El Spooler mantiene el trabajo seguro en RAM/PSRAM
                    static unsigned long lastWaitLog = 0;
                    if (millis() - lastWaitLog > 10000) {
                        lastWaitLog = millis();
                        Logger::logf("SPOOLER", "⏳ Spooler en espera: %d trabajos en cola retenidos hasta resolver alerta de impresora (%s).", (int)jobQueue.size(), UsbPrinterHost::getDetailedStatusString().c_str());
                    }
                    vTaskDelay(pdMS_TO_TICKS(1000));
                }
            } else {
                vTaskDelay(pdMS_TO_TICKS(50));
            }
        }
    }

public:
    static void init() {
        queueMutex = xSemaphoreCreateMutex();
        jobsEnqueued = 0;
        jobsProcessed = 0;

        xTaskCreatePinnedToCore(spoolerTask, "spooler_task", 6144, NULL, 2, &spoolerTaskHandle, 1);
    }

    static bool enqueueJob(const uint8_t* data, size_t length) {
        if (length == 0 || !data) return false;

        if (xSemaphoreTake(queueMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
            if (jobQueue.size() >= SPOOLER_MAX_JOBS) {
                Logger::log("SPOOLER", "❌ ERROR CRÍTICO: La cola del Spooler está llena (" + String(SPOOLER_MAX_JOBS) + " trabajos). No se puede aceptar más hasta liberar espacio.");
                xSemaphoreGive(queueMutex);
                return false;
            }

            // Intentar alojar en PSRAM si está disponible en ESP32-S3 (ps_malloc), sino en RAM estándar (malloc)
            uint8_t* newBuffer = nullptr;
#if defined(BOARD_HAS_PSRAM)
            if (psramFound()) {
                newBuffer = (uint8_t*)ps_malloc(length);
            }
#endif
            if (!newBuffer) {
                newBuffer = (uint8_t*)malloc(length);
            }

            if (!newBuffer) {
                Logger::log("SPOOLER", "❌ ERROR CRÍTICO: Memoria insuficiente para alojar trabajo de " + String(length) + " bytes.");
                xSemaphoreGive(queueMutex);
                return false;
            }

            memcpy(newBuffer, data, length);
            SpoolerJob newJob;
            newJob.buffer = newBuffer;
            newJob.length = length;
            newJob.timestamp = millis();

            jobQueue.push_back(newJob);
            jobsEnqueued++;
            Logger::incrementJobs();

            Logger::logf("SPOOLER", "📥 Trabajo de %u bytes encolado en RAM/PSRAM. Total en cola: %d", (unsigned int)length, (int)jobQueue.size());
            xSemaphoreGive(queueMutex);
            return true;
        }

        return false;
    }

    static int getPendingJobCount() {
        int count = 0;
        if (xSemaphoreTake(queueMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
            count = (int)jobQueue.size();
            xSemaphoreGive(queueMutex);
        }
        return count;
    }

    static unsigned long getJobsEnqueued() { return jobsEnqueued; }
    static unsigned long getJobsProcessed() { return jobsProcessed; }

    static void clearQueue() {
        if (xSemaphoreTake(queueMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
            for (auto& job : jobQueue) {
                if (job.buffer) free(job.buffer);
            }
            jobQueue.clear();
            Logger::log("SPOOLER", "🗑️ Cola del Spooler vaciada manualmente por el administrador.");
            xSemaphoreGive(queueMutex);
        }
    }
};

std::vector<SpoolerJob> PrintSpooler::jobQueue;
SemaphoreHandle_t PrintSpooler::queueMutex = NULL;
TaskHandle_t PrintSpooler::spoolerTaskHandle = NULL;
unsigned long PrintSpooler::jobsEnqueued = 0;
unsigned long PrintSpooler::jobsProcessed = 0;

#endif // PRINT_SPOOLER_H
