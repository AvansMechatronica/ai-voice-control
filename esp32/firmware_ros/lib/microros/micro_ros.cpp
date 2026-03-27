#include "micro_ros.h"

static void micro_ros_task(void *param)
{
  MicroROS *microRos = static_cast<MicroROS *>(param);

  while (true)
  {
    microRos->loop();
  }
}

bool errorLedState = false;

int reset_timer_cnt = 0;

void error_loop(int line){
  Serial.printf("Voice Control\nError\nSystem halted\nLine: %d\n", line);
  while(1){

        if(errorLedState){
            digitalWrite(ONBOARD_LED_PIN, HIGH);
            errorLedState = false;
        }
        else{
            //neopixelWrite(RGB_BUILTIN,RGB_BRIGHTNESS,0, 0);
            digitalWrite(ONBOARD_LED_PIN, LOW);
            errorLedState = true;
        }
    delay(100);
    if(reset_timer_cnt++ >= 50){
      ESP.restart(); // Reset after 5 seconds
    }
  }
}


MicroROS::MicroROS() {
    // Constructor implementation (if needed)

}

void MicroROS::init() {
    // Initialization code for micro-ROS (if needed)
#if defined(ONBOARD_LED_PIN)
    pinMode(ONBOARD_LED_PIN, OUTPUT); 
    digitalWrite(ONBOARD_LED_PIN, HIGH);
#endif


#if defined(WIFI)
//#define WIFI_SSID "Wifi_ssid"
//#define WIFI_PASSWORD "Wifi_Password"
  WiFi.setHostname("VoiceControl");
  set_microros_wifi_transports((char*)wifiSSID, (char*)wifiPass, AGENT_IP_ADDRESS, (size_t)PORT);
#else
  set_microros_serial_transports(Serial);
  delay(2000);
#endif

  allocator = rcl_get_default_allocator();

  //create init_options
  RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));

  // create node
  RCCHECK(rclc_node_init_default(&node, NODE_NAME, "", &support));

  // create executor used by loop()
  RCCHECK(rclc_executor_init(&executor, &support.context, 1, &allocator));

    // create detected_word_publisher
  RCCHECK(rclc_publisher_init_default(
    &detected_word_publisher,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, UInt8),
    "detected_word"));

  TaskHandle_t microRosTaskHandle;
  xTaskCreatePinnedToCore(micro_ros_task, "micro-ROS", 4096, this, 1, &microRosTaskHandle, 1);
}

void MicroROS::loop() {
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  RCSOFTCHECK(rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100)));
}

void MicroROS::publish_detected_word(uint8_t word_id) {
    detected_word.data = word_id;
    RCSOFTCHECK(rcl_publish(&detected_word_publisher, &detected_word, NULL));
}