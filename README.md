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
The only issue is `#include <U8g2lib.h>`  In the libray manager search for "U8g" and choose the library by oliver
`https://github.com/olikraus/u8g2`

## Authors

* **Terry Clarkson** - *Initial work*
* **Adam Clarkson**
