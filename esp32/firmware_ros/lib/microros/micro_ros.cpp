#include "micro_ros.h"
#if defined(WIFI)
#include "wifi_network_config.h"
#include <esp_wifi.h>
#define NODE_NAME "voice_control_node"
#endif


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
  m_initialized = false;
}

/**
 * Convert snake_case to camelCase
 */
char* convertToCamelCase(const char *input) {
  static char output[64];
  if (input == nullptr) {
    output[0] = '\0';
    return output;
  }

  size_t out_index = 0;
  bool upper_next = false;
  for (size_t in_index = 0; input[in_index] != '\0' && out_index < (sizeof(output) - 1); ++in_index) {
    char current = input[in_index];
    if (current == '_') {
      upper_next = true;
      continue;
    }

    if (upper_next && current >= 'a' && current <= 'z') {
      current = static_cast<char>(current - ('a' - 'A'));
    }
    upper_next = false;
    output[out_index++] = current;
  }

  output[out_index] = '\0';
  return output;
}

void MicroROS::init() {
  m_initialized = false;
    // Initialization code for micro-ROS (if needed)
#if defined(ONBOARD_LED_PIN)
    pinMode(ONBOARD_LED_PIN, OUTPUT); 
    digitalWrite(ONBOARD_LED_PIN, HIGH);
#endif


#if defined(WIFI)
  const char *host_name = convertToCamelCase(NODE_NAME);
  Serial.printf("hostname :%s\n", host_name);
  WiFi.setHostname(host_name);
#if 0
  
  // UCP_MOTORS_PIN low to force WiFi configuration mode
  bool force_configure_wifi = false;//digitalRead(UCP_MOTORS_PIN) == LOW;
  

  NETWORK_CONFIG networkConfig;
  bool wifiUp = configureNetwork(force_configure_wifi, &networkConfig);
  if(!wifiUp){
    Serial.printf("Config mode:\nconnect to AP\nand set WiFi\n");
    return;
  }

  esp_wifi_set_max_tx_power(WIFI_POWER_19_5dBm); // Set max WiFi transmit power to 19.5 dBm (max for ESP32-S3)

  set_microros_wifi_transports(const_cast<char*>(networkConfig.ssid.c_str()), 
                               const_cast<char*>(networkConfig.password.c_str()), 
                               networkConfig.microros_agent_ip_address,
                               networkConfig.microros_agent_port);
#else
#define WIFI_SSID "VRV9517724283"
#define WIFI_PASSWORD "@AYCwXhz976C"
#define AGENT_IP_ADDRESS IPAddress(192, 168, 2, 150)
#define PORT 8888

  set_microros_wifi_transports((char*)WIFI_SSID, (char*)WIFI_PASSWORD, AGENT_IP_ADDRESS, (size_t)PORT);
#endif
  Serial.printf("WiFi Connected\n");
  delay(2000);

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
  BaseType_t taskCreated = xTaskCreatePinnedToCore(micro_ros_task, "micro-ROS", 4096, this, 1, &microRosTaskHandle, 1);
  if (taskCreated != pdPASS || microRosTaskHandle == NULL)
  {
    Serial.printf("ERROR: Failed to create micro-ROS task\n");
    return;
  }

  m_initialized = true;
}

void MicroROS::loop() {
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  RCSOFTCHECK(rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100)));
}

void MicroROS::publish_detected_word(uint8_t word_id) {
    if (!m_initialized)
    {
      return;
    }
    detected_word.data = word_id;
    RCSOFTCHECK(rcl_publish(&detected_word_publisher, &detected_word, NULL));
}