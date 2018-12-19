# HotSpotPrinter

Serial printer connected to ESP32 WiFi module to and Pro line scale systems from Pro Tournament Scales.  
Prints tickets with weight of scale.  Programable via any web browser including mobile devices
Wifi network is created via  local hotspot.

## Getting Started

In the arduino IDE
Add the following link to the Additional Boards Manager URLs section in File > Preferences > Settings
`https://dl.espressif.com/dl/package_esp32_index.json`
Select the board in tools
Board used is the Heltec_WIFI_kit_32
Upload Speed is 921600
Flash Frequency is 40mhz

### Prerequisites

The included libraries will install automatically from the board and standard set of libraries

### Dependencies

`#include <U8g2lib.h>`  In the library manager search for "U8g" and choose the library by oliver
`https://github.com/olikraus/u8g2`
`#include <sqlite3.h>` Library for the database engine TODO need link
`#include <WiFi.h>` Wi-Fi library TODO need link
`#include <EEPROM.h>` Driver for eeprom TODO need link
`#include <Update.h>` Firmware uploader TODO need link
`#include <Wire.h>` i2C function TODO need link
`#include <LiquidCrystal_I2C.h>` 4x20 lcd display TODO need link
`#include <NetBIOS.h>` Unknown TODO need link
`#include <RTClib.h>` library for  RTC routines TODO need link


## Authors

* **Terry Clarkson** - *Initial work*
* **Adam Clarkson**
