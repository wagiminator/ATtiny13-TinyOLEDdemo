# TinyOLEDdemo - I²C OLED on an ATtiny13
This is just a little demo on how to use an I²C OLED with the limited capabilities of an ATtiny13.

The I²C protocol implementation is based on a crude bitbanging method. It was specifically designed for the limited resources of ATtiny10 and ATtiny13, but should work with some other AVRs as well. To make the code as compact as possible, the following restrictions apply:
- the clock frequency of the MCU must not exceed 1.6 MHz,
- the slave device must support fast mode 400 kbps (is mostly the case),
- the slave device must not stretch the clock (this is usually the case),
- the acknowledge bit sent by the slave device is ignored.

Don't forget the pull-up resistors on the SDA and SCL lines! Many modules, such as the SSD1306 OLED module, have already integrated them.

The functions for the OLED are adapted to the SSD1306 128x32 OLED module, but they can easily be modified to be used for other modules. In order to save resources, only the basic functionalities are implemented.  
