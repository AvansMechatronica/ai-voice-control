#ifndef  __MICRO_ROS_H__   
#define  __MICRO_ROS_H__
#include <Arduino.h>
#include <micro_ros_platformio.h>

#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/init.h>
#include <rclc/executor.h>
#include <std_msgs/msg/u_int8.h>

#define NODE_NAME "VOICE_DETECTION_NODE"

#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){error_loop(__LINE__);}}
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){}}

class MicroROS {
public:
    MicroROS();
    void init();
    void loop();
    void publish_detected_word(uint8_t word_id);
private:
    rcl_publisher_t detected_word_publisher;
    std_msgs__msg__UInt8 detected_word;
    rclc_executor_t executor;
    rclc_support_t support;
    rcl_allocator_t allocator;
    rcl_node_t node;
    rcl_timer_t timer;
};

#endif