# Voice Command Recognition Firmware (ROS 2 / micro-ROS)

This firmware runs keyword detection on ESP32 and publishes detected word IDs to ROS 2 using micro-ROS.


See details on the [ai-voice-control](https://github.com/AvansMechatronica/ai-voice-control) repository. Here you can find the way to create a custom wake word model that you can use with this firmware.

## Main code entry points

- `src/main.cpp`: app startup, task setup
- `src/CommandDetector.cpp`: audio inference pipeline
- `src/CommandProcessor.cpp`: command handling
- `lib/microros/micro_ros.cpp`: ROS transport and publisher setup

## Supported boards and environments

Configured in `platformio.ini`:

- `esp32-wroom-ros`: serial micro-ROS transport
- `esp32-S3-ros`: Wi-Fi micro-ROS transport (with AP config page support)

Default environment:

```bash
platformio run
```

Or explicitly select one:

```bash
platformio run --environment esp32-wroom-ros
platformio run --environment esp32-S3-ros
```

## Build and flash

Build:

```bash
platformio run --environment esp32-S3-ros
```

Upload firmware:

```bash
platformio run --environment esp32-S3-ros --target upload
```

If using Wi-Fi AP configuration pages (S3 env), upload LittleFS data too:

```bash
platformio run --environment esp32-S3-ros --target buildfs
platformio run --environment esp32-S3-ros --target uploadfs
```

## ROS 2 agent setup

Run micro-ROS agent with Docker.

Serial transport example:

```bash
docker run --rm -it --net=host --privileged -v /dev:/dev \
	microros/micro-ros-agent:humble serial --dev /dev/ttyACM0 -b 115200
```

Wi-Fi transport example (UDP 8888):

```bash
docker run --rm -it --net=host microros/micro-ros-agent:humble udp4 --port 8888
```

## ROS topic

The firmware publishes detected words on topic:

- `detected_word` (`std_msgs/msg/UInt8`)

Check from ROS 2 host:

```bash
ros2 topic list
ros2 topic echo /detected_word
```

## Network and transport configuration

Configure these in `platformio.ini` build flags for Wi-Fi mode:

- `WIFI_SSID`
- `WIFI_PASSWORD`
- `AGENT_IP_ADDRESS` (format: `"192.168.2.150"`)

The firmware parses this string and converts it to an `IPAddress` object internally.
