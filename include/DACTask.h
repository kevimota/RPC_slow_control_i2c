#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include "FEBManager.h"

struct DACCommand {
    uint8_t cmd;      // 0=set, 1=enable, 2=disable, 3=setall, 4=setfeb, 5=enableall, 6=disableall
    int8_t feb;
    int8_t chip;
    int8_t channel;
    float voltage;
    uint8_t dacType;  // 0=threshold, 1=vmon
};

class DACTask {
public:
    static void begin(FEBManager* febs) {
        _febs = febs;
        _queue = xQueueCreate(8, sizeof(DACCommand));
        xTaskCreatePinnedToCore(taskFunc, "dac", 4096, nullptr, 1, &_handle, 1);
    }

    static bool post(const DACCommand& cmd) {
        return xQueueSend(_queue, &cmd, 0) == pdTRUE;
    }

private:
    static FEBManager* _febs;
    static QueueHandle_t _queue;
    static TaskHandle_t _handle;

    static void taskFunc(void*) {
        DACCommand cmd;
        while (true) {
            if (xQueueReceive(_queue, &cmd, portMAX_DELAY) == pdTRUE) {
                switch (cmd.cmd) {
                    case 0:
                        _febs[cmd.feb].setDAC(cmd.chip, cmd.channel, cmd.voltage);
                        _febs[cmd.feb].update();
                        break;
                    case 1:
                        _febs[cmd.feb].enableDAC(cmd.chip);
                        _febs[cmd.feb].update();
                        break;
                    case 2:
                        _febs[cmd.feb].disableDAC(cmd.chip);
                        _febs[cmd.feb].update();
                        break;
                    case 3: {
                        int chStart = (cmd.dacType == 0) ? 0 : 2;
                        int chEnd = chStart + 1;
                        for (int f = 0; f < 4; f++) {
                            for (int c = 0; c < 2; c++)
                                for (int ch = chStart; ch <= chEnd; ch++)
                                    _febs[f].setDAC(c, ch + 1, cmd.voltage);
                            _febs[f].update();
                        }
                        break;
                    }
                    case 4: {
                        int chStart = (cmd.dacType == 0) ? 0 : 2;
                        int chEnd = chStart + 1;
                        for (int c = 0; c < 2; c++)
                            for (int ch = chStart; ch <= chEnd; ch++)
                                _febs[cmd.feb].setDAC(c, ch + 1, cmd.voltage);
                        _febs[cmd.feb].update();
                        break;
                    }
                    case 5:
                        for (int f = 0; f < 4; f++)
                            for (int c = 0; c < 2; c++) {
                                _febs[f].enableDAC(c);
                                _febs[f].update();
                            }
                        break;
                    case 6:
                        for (int f = 0; f < 4; f++)
                            for (int c = 0; c < 2; c++) {
                                _febs[f].disableDAC(c);
                                _febs[f].update();
                            }
                        break;
                }
            }
        }
    }
};

FEBManager* DACTask::_febs = nullptr;
QueueHandle_t DACTask::_queue = nullptr;
TaskHandle_t DACTask::_handle = nullptr;
