# Septic Alarm Monitor #

# What is it?

(for more details, see [this](https://belikoff.net/FIXME))

This is an M5StickC Plus based monitor for an old-style seprtic alarm (the one that emits a loud sound). It watches for the sound and posts information about alarm starting/stopping to the Internet, thus allowing remote monitoring for the system that is inherently not remote-friendly.

# What do you need to get one for yourself?

* An [M5StickC Plus](https://shop.m5stack.com/products/m5stickc-plus-esp32-pico-mini-iot-development-kit) controller.
* A computer with [Arduino IDE](https://www.arduino.cc/en/software) (to build and install software)
* A USB-C cable.

# Setup

* Install Arduino software.
  * For Linux, make sure to add your user to group `dialout`
* Add `M5StickC` support to Arduino
  * In _Preferences_ add additional board URL <https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/arduino/package_m5stack_index.json>
  * In _Tools->Board->Boards_ install `M5Stack` board.
* In _Sketch->Manage libraries_ install the following libraries:
  * `M5StickCPlus`
  * `ThingSpeak`
  * `TFT`
* Select board `M5Stick-C-Plus`
* Select port (likely `/dev/ttyUSB0`) and speed (likely 115200)

# Installation and configuration

* Go to <https://thingspeak.com> and create an account. Create a _channel_ and note the Channel ID and the API key you should use.
* Copy `arduino_secrets.h.template` file to `arduino_secrets.h` and change the values in it to reflect your WiFi and ThingSpeak setup.
* Load `septic_alarm_m5stickc` folder in Arduino and make sure the setup above is correct.
* Connect the controller to your PC and make sure Arduino sees it as connected.
* Build and upload the software.

It is recommended to initially uncomment `#define DEBUG` in `config.h` and to build and run the controller while connected to the PC with serial monitor open to observe the operation and to make sure everything works. One you are sure it works, comment `DEBUG` back and re-upload.
