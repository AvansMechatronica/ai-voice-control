#include <Arduino.h>
#include "CommandProcessor.h"

#if defined(INCLUDE_PICOROBOT)
#include <WS2812FX.h>
#endif

#if defined(INCLUDE_MOTORS)
#define RIGHT_MOTOR_CHANNEL 1
#define LEFT_MOTOR_CHANNEL 0
#endif

#if defined(INCLUDE_PICOROBOT)
#define LA_CHANNEL 0
#define LB_CHANNEL 1
#define RA_CHANNEL 2
#define RB_CHANNEL 3
//WS2812FX *ws2812fx;
Adafruit_NeoPixel *ws2812fx;
#define NUMBER_OF_RGB_LEDS 8
#endif

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
#if defined(INCLUDE_PICOROBOT)
        //ws2812fx->service();
#endif
    }
}

#if defined(INCLUDE_MOTORS)
int calcDuty(int ms)
{
    // 50Hz = 20ms period
    return (65536 * ms) / 20000;
}
#endif

#if defined(INCLUDE_MOTORS)
const int leftForward = 1600;
const int leftBackward = 1400;
const int leftStop = 1500;
const int rightBackward = 1600;
const int rightForward = 1400;
const int rightStop = 1500;
#endif

#if defined(INCLUDE_PICOROBOT)
#define MOTOR_SPEED 200 //120
#define MOTOR_TURN_SPEED 80
#endif

#if defined(INCLUDE_PICOROBOT)
void turnAllRgbLedsOff()
{
    for (int i = 0; i < NUMBER_OF_RGB_LEDS; i++)    {
        ws2812fx->setPixelColor(i, 0, 0, 0);
    }
    ws2812fx->show();
}
#endif

void CommandProcessor::processCommand(uint16_t commandIndex)
{
    digitalWrite(ONBOARD_LED_PIN, HIGH);
    switch (commandIndex)
    {
    case 0: // forward
#ifdef DEBUG_COMMAND_PROCESSOR
        Serial.printf("Processing forward command\n");
#endif
#if defined(INCLUDE_MOTORS)
        ledcWrite(LEFT_MOTOR_CHANNEL, calcDuty(leftForward));
        ledcWrite(RIGHT_MOTOR_CHANNEL, calcDuty(rightForward));
#endif
#if defined(INCLUDE_PICOROBOT)
        // Move forward
        ledcWrite(LA_CHANNEL, 0); // left forward
        ledcWrite(LB_CHANNEL, MOTOR_SPEED);           // left backward
        ledcWrite(RA_CHANNEL, MOTOR_SPEED); // right forward
        ledcWrite(RB_CHANNEL, 0);           // right backward
        ws2812fx->setPixelColor(2, 0, 0, 255); // blue
        ws2812fx->setPixelColor(3, 0, 0, 255); // blue
        ws2812fx->show();  
#endif
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        break;
    case 1: // backward
#ifdef DEBUG_COMMAND_PROCESSOR
        Serial.printf("Processing backward command\n");
#endif
#if defined(INCLUDE_MOTORS)
        ledcWrite(LEFT_MOTOR_CHANNEL, calcDuty(leftBackward));
        ledcWrite(RIGHT_MOTOR_CHANNEL, calcDuty(rightBackward));
#endif
#if defined(INCLUDE_PICOROBOT)
        // Move backward
        ledcWrite(LA_CHANNEL, MOTOR_SPEED);           // left forward
        ledcWrite(LB_CHANNEL, 0); // left backward
        ledcWrite(RA_CHANNEL, 0);           // right forward
        ledcWrite(RB_CHANNEL, MOTOR_SPEED); // right backward
        ws2812fx->setPixelColor(6, 128, 128, 0); // yellow
        ws2812fx->setPixelColor(7, 128, 128, 0); // yellow
        ws2812fx->show();
#endif  
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        break;
    case 2: // left
#ifdef DEBUG_COMMAND_PROCESSOR
        Serial.printf("Processing left command\n");
#endif
#if defined(INCLUDE_MOTORS)
        ledcWrite(LEFT_MOTOR_CHANNEL, calcDuty(leftBackward));
        ledcWrite(RIGHT_MOTOR_CHANNEL, calcDuty(rightForward));
#endif
#if defined(INCLUDE_PICOROBOT)
        // Turn left
        ledcWrite(LA_CHANNEL, MOTOR_TURN_SPEED);           // left forward
        ledcWrite(LB_CHANNEL, 0); // left backward
        ledcWrite(RA_CHANNEL, MOTOR_TURN_SPEED); // right forward
        ledcWrite(RB_CHANNEL, 0);           // right backward
        ws2812fx->setPixelColor(4, 255, 0, 0); // blue
        ws2812fx->setPixelColor(5, 255, 0, 0); // blue
        ws2812fx->show();

#endif
        vTaskDelay(500 / portTICK_PERIOD_MS);
        break;
    case 3: // right
#ifdef DEBUG_COMMAND_PROCESSOR
        Serial.printf("Processing right command\n");
#endif
#if defined(INCLUDE_MOTORS) 
        ledcWrite(LEFT_MOTOR_CHANNEL, calcDuty(leftForward));
        ledcWrite(RIGHT_MOTOR_CHANNEL, calcDuty(rightBackward));
#endif
#if defined(INCLUDE_PICOROBOT)
        // Turn right
        ledcWrite(LA_CHANNEL, 0); // left forward
        ledcWrite(LB_CHANNEL, MOTOR_TURN_SPEED);           // left backward
        ledcWrite(RA_CHANNEL, 0);           // right forward
        ledcWrite(RB_CHANNEL, MOTOR_TURN_SPEED); // right backward
        ws2812fx->setPixelColor(1, 0, 255, 0); // green
        ws2812fx->setPixelColor(0, 0, 255, 0); // green
//        ws2812fx->setPixelColor(7, 0, 255, 0); // green
        ws2812fx->show();
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
    ledcWrite(LEFT_MOTOR_CHANNEL, calcDuty(leftStop));  // stop
    ledcWrite(RIGHT_MOTOR_CHANNEL, calcDuty(rightStop)); // stop
#endif
#if defined(INCLUDE_PICOROBOT)
    // Stop all motors
    ledcWrite(LA_CHANNEL, 0);
    ledcWrite(LB_CHANNEL, 0);
    ledcWrite(RA_CHANNEL, 0);
    ledcWrite(RB_CHANNEL, 0);
    turnAllRgbLedsOff();
#endif
}

CommandProcessor::CommandProcessor()
{
    pinMode(ONBOARD_LED_PIN, OUTPUT);
#if defined(INCLUDE_MOTORS)
    // setup the motors
    ledcSetup(LEFT_MOTOR_CHANNEL, 50, 16);
    ledcAttachPin(LEFT_MOTOR_PIN, LEFT_MOTOR_CHANNEL);
    ledcSetup(RIGHT_MOTOR_CHANNEL, 50, 16);
    ledcAttachPin(RIGHT_MOTOR_PIN, RIGHT_MOTOR_CHANNEL);
    ledcWrite(LEFT_MOTOR_CHANNEL, calcDuty(leftStop)); // left
    ledcWrite(RIGHT_MOTOR_CHANNEL, calcDuty(rightStop)); // right
#endif

#if defined(INCLUDE_PICOROBOT)
    // setup the motors
    // Setup PWM channels for the motors with 1000Hz frequency and 8-bit resolution
    ledcSetup(LA_CHANNEL, 1000, 8);
    ledcAttachPin(LA_PIN, LA_CHANNEL);
    ledcSetup(LB_CHANNEL, 1000, 8);
    ledcAttachPin(LB_PIN, LB_CHANNEL);
    ledcSetup(RA_CHANNEL, 1000, 8);
    ledcAttachPin(RA_PIN, RA_CHANNEL);
    ledcSetup(RB_CHANNEL, 1000, 8);
    ledcAttachPin(RB_PIN, RB_CHANNEL);
    // Turn off all motors initially
    ledcWrite(LA_CHANNEL, 0); // left forward
    ledcWrite(LB_CHANNEL, 0); // left backward
    ledcWrite(RA_CHANNEL, 0); // right forward
    ledcWrite(RB_CHANNEL, 0); // right backward

    // setup the WS2812FX library for the onboard LED 
    ws2812fx = new WS2812FX(NUMBER_OF_RGB_LEDS, WS2812B_PIN, NEO_GRB + NEO_KHZ800);
    ws2812fx->begin();
    //ws2812fx->init();
    ws2812fx->setBrightness(50);
    //ws2812fx->setColor(0, 255, 0); // green
    ws2812fx->setPixelColor(1, 0, 255, 0); // green
    ws2812fx->show();
#endif

    // allow up to 5 commands to be in flight at once
    m_command_queue_handle = xQueueCreate(5, sizeof(uint16_t));
    if (!m_command_queue_handle)
    {
        Serial.printf("Failed to create command queue\n");
    }
    // kick off the command processor task
    TaskHandle_t command_queue_task_handle;
    xTaskCreate(commandQueueProcessorTask, "Command Queue Processor", 4096, this, 1, &command_queue_task_handle);

    // Flash the onboard LED a few times to indicate we're ready
    for(int i = 0; i < 3; i++)
    {
        digitalWrite(ONBOARD_LED_PIN, HIGH);
        delay(1000);
        digitalWrite(ONBOARD_LED_PIN, LOW);
        delay(1000);
    }
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
