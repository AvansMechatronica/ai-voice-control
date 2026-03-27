#include <Arduino.h>
#include "CommandProcessor.h"


#define DEBUG_COMMAND_PROCESSOR

const char *words[] = {
    "forward",
    "backward",
    "left",
    "right",
    "_nonsense",
};

void commandQueueProcessorTask(void *param)
{
    CommandProcessor *commandProcessor = (CommandProcessor *)param;
    while (true)
    {
        uint16_t commandIndex = 0;
        if (xQueueReceive(commandProcessor->m_command_queue_handle, &commandIndex, portMAX_DELAY) == pdTRUE)
        {
            commandProcessor->processCommand(commandIndex);
        }
    }
}

int calcDuty(int ms)
{
    // 50Hz = 20ms period
    return (65536 * ms) / 20000;
}

#if defined(INCLUDE_MOTORS)
const int leftForward = 1600;
const int leftBackward = 1400;
const int leftStop = 1500;
const int rightBackward = 1600;
const int rightForward = 1445;
const int rightStop = 1500;
#endif

void CommandProcessor::processCommand(uint16_t commandIndex)
{
    digitalWrite(ONBOARD_LED_PIN, HIGH);
    switch (commandIndex)
    {
    case 0: // forward
#ifdef DEBUG_COMMAND_PROCESSOR
        Serial.printf("Processing forward command\n");

#if defined(INCLUDE_MOTORS)
        ledcWrite(0, calcDuty(leftForward));
        ledcWrite(1, calcDuty(rightForward));
#endif
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        break;
    case 1: // backward
#ifdef DEBUG_COMMAND_PROCESSOR
        Serial.printf("Processing backward command\n");
#endif
#if defined(INCLUDE_MOTORS)
        ledcWrite(0, calcDuty(leftBackward));
        ledcWrite(1, calcDuty(rightBackward));
#endif
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        break;
    case 2: // left
#ifdef DEBUG_COMMAND_PROCESSOR
        Serial.printf("Processing left command\n");
#endif
#if defined(INCLUDE_MOTORS)
        ledcWrite(0, calcDuty(leftBackward));
        ledcWrite(1, calcDuty(rightForward));
#endif
        vTaskDelay(500 / portTICK_PERIOD_MS);
        break;
    case 3: // right
#ifdef DEBUG_COMMAND_PROCESSOR
        Serial.printf("Processing right command\n");
#endif
#if defined(INCLUDE_MOTORS) 
        ledcWrite(0, calcDuty(leftForward));
        ledcWrite(1, calcDuty(rightBackward));
#endif
        vTaskDelay(500 / portTICK_PERIOD_MS);
        break;
    default:
#ifdef DEBUG_COMMAND_PROCESSOR
        Serial.printf("Unknown command index %d\n", commandIndex);
#endif
        break;
    }
    digitalWrite(ONBOARD_LED_PIN, LOW);
#if defined(INCLUDE_MOTORS)
    ledcWrite(0, calcDuty(leftStop));  // stop
    ledcWrite(1, calcDuty(rightStop)); // stop
#endif
}

CommandProcessor::CommandProcessor()
{
    pinMode(ONBOARD_LED_PIN, OUTPUT);
#if defined(INCLUDE_MOTORS)
    // setup the motors
    ledcSetup(0, 50, 16);
    ledcAttachPin(LEFT_MOTOR_PIN, 0);
    ledcSetup(1, 50, 16);
    ledcAttachPin(RIGHT_MOTOR_PIN, 1);
    ledcWrite(0, calcDuty(1500)); // left
    ledcWrite(1, calcDuty(1500)); // right
#endif

    // allow up to 5 commands to be in flight at once
    m_command_queue_handle = xQueueCreate(5, sizeof(uint16_t));
    if (!m_command_queue_handle)
    {
        Serial.printf("Failed to create command queue\n");
    }
    // kick off the command processor task
    TaskHandle_t command_queue_task_handle;
    xTaskCreate(commandQueueProcessorTask, "Command Queue Processor", 1024, this, 1, &command_queue_task_handle);
}

void CommandProcessor::queueCommand(uint16_t commandIndex, float best_score)
{
    // unsigned long now = millis();
    if (commandIndex != 5 && commandIndex != -1)
    {
        Serial.printf("***** %ld Detected command %s(%f)\n", millis(), words[commandIndex], best_score);
        if (xQueueSendToBack(m_command_queue_handle, &commandIndex, 0) != pdTRUE)
        {
            Serial.printf("No more space for command\n");
        }
    }
}
