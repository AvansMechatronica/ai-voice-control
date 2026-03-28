#include <Arduino.h>
#include <WiFi.h>
#include <driver/i2s.h>
#include <esp_task_wdt.h>
#include <esp_heap_caps.h>
#include "I2SMicSampler.h"
#include "config.h"
#include "CommandDetector.h"
#include "CommandProcessor.h"
#include "micro_ros.h"

MicroROS micro_ros;

static void publishDetectedWordCallback(uint8_t wordId, void *context)
{
  MicroROS *microRos = static_cast<MicroROS *>(context);
  if (microRos)
  {
    microRos->publish_detected_word(wordId);
  }
}

// This task does all the heavy lifting for our application
void applicationTask(void *param)
{
  CommandDetector *commandDetector = static_cast<CommandDetector *>(param);

  const TickType_t xMaxBlockTime = pdMS_TO_TICKS(100);
  while (true)
  {
    // wait for some audio samples to arrive
    uint32_t ulNotificationValue = ulTaskNotifyTake(pdTRUE, xMaxBlockTime);
    if (ulNotificationValue > 0)
    {
      commandDetector->run();
      vTaskDelay(pdMS_TO_TICKS(1));
    }
  }
}

void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting up");

  // make sure we don't get killed for our long running tasks
  esp_task_wdt_init(10, false);

  // start up the I2S input (from either an I2S microphone or Analogue microphone via the ADC)
  // Direct i2s input from INMP441 or the SPH0645
  I2SSampler *i2s_sampler = new I2SMicSampler(pin_config, false);
  // the command processor
  CommandProcessor *command_processor = new CommandProcessor();
  command_processor->setDetectedWordCallback(publishDetectedWordCallback, &micro_ros);

  // create our application
  CommandDetector *commandDetector = new CommandDetector(i2s_sampler, command_processor);

  // set up the i2s sample writer task
  TaskHandle_t applicationTaskHandle = NULL;
  BaseType_t taskCreated = xTaskCreatePinnedToCore(applicationTask, "Command Detect", 8192, commandDetector, 1, &applicationTaskHandle, 0);
  if (taskCreated != pdPASS || applicationTaskHandle == NULL)
  {
    Serial.printf("ERROR: Failed to create applicationTask (free heap=%d, largest internal=%d)\n",
                  esp_get_free_heap_size(),
                  heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    return;
  }

  // start sampling from i2s device - use I2S_NUM_0 as that's the one that supports the internal ADC
  i2s_sampler->start(I2S_NUM_0, i2s_config, applicationTaskHandle);

  // Initialize micro-ROS after the application task is created to keep enough
  // contiguous internal RAM available for task stack allocation.
  micro_ros.init();
}

void loop()
{
  vTaskDelay(pdMS_TO_TICKS(1000));
}