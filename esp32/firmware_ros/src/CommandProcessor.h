#ifndef _intent_processor_h_
#define _intent_processor_h_

#include <list>
#include <Arduino.h>
class StepperMotor;
class Servo;

class CommandProcessor
{
private:
    using DetectedWordCallback = void (*)(uint8_t wordId, void *context);

    QueueHandle_t m_command_queue_handle;
    DetectedWordCallback m_detected_word_callback;
    void *m_detected_word_callback_context;
    void processCommand(uint16_t commandIndex);

public:
    CommandProcessor();
    void setDetectedWordCallback(DetectedWordCallback callback, void *context);
    void queueCommand(uint16_t commandIndex, float score);
    friend void commandQueueProcessorTask(void *param);
};

#endif
