# TinyOLEDdemo - I²C OLED on an ATtiny13
This is just a little demo on how to use an I²C OLED with the limited capabilities of an ATtiny13.

![pic1.jpg](https://github.com/wagiminator/ATtiny13-TinyOLEDdemo/blob/main/documentation/TinyOLEDdemo_pic1.jpg)
![pic2.jpg](https://github.com/wagiminator/ATtiny13-TinyOLEDdemo/blob/main/documentation/TinyOLEDdemo_pic2.jpg)
![pic3.jpg](https://github.com/wagiminator/ATtiny13-TinyOLEDdemo/blob/main/documentation/TinyOLEDdemo_pic3.jpg)

# I²C Implementation
The I²C protocol implementation is based on a crude bitbanging method. It was specifically designed for the limited resources of ATtiny10 and ATtiny13, but should work with some other AVRs as well. To make the code as compact as possible, the following restrictions apply:
- the clock frequency of the MCU must not exceed 1.6 MHz,
- the slave device must support fast mode 400 kbps (is mostly the case),
- the slave device must not stretch the clock (this is usually the case),
- the acknowledge bit sent by the slave device is ignored.

Don't forget the pull-up resistors on the SDA and SCL lines! Many modules, such as the SSD1306 OLED module, have already integrated them.

# I²C Protocol Specification
Refer to: https://i2c.info/i2c-bus-specification

Both signals (SCL and SDA) are bidirectional. They are connected via resistors to a positive power supply voltage. This means that when the bus is free, both lines are high. All devices on the bus must have open-collector or open-drain pins. Activating the line means pulling it down (wired AND).

For each clock pulse one bit of data is transferred. The SDA signal can only change when the SCL signal is low – when the clock is high the data should be stable.

![bit-transfer.gif](https://github.com/wagiminator/ATtiny13-TinyOLEDdemo/blob/main/documentation/i2c-bit-transfer.gif)

Each I2C command initiated by master device starts with a START condition and ends with a STOP condition. For both conditions SCL has to be high. A high to low transition of SDA is considered as START and a low to high transition as STOP.

![start-stop.gif](https://github.com/wagiminator/ATtiny13-TinyOLEDdemo/blob/main/documentation/i2c-start-stop.gif)

After the Start condition the bus is considered as busy and can be used by another master only after a Stop condition is detected. After the Start condition the master can generate a repeated Start. This is equivalent to a normal Start and is usually followed by the slave I2C address.

![data-transfer.gif](https://github.com/wagiminator/ATtiny13-TinyOLEDdemo/blob/main/documentation/i2c-data-transfer.gif)

Data on the I2C bus is transferred in 8-bit packets (bytes). There is no limitation on the number of bytes, however, each byte must be followed by an Acknowledge bit. This bit signals whether the device is ready to proceed with the next byte. For all data bits including the Acknowledge bit, the master must generate clock pulses. If the slave device does not acknowledges transfer this means that there is no more data or the device is not ready for the transfer yet. The master device must either generate Stop or Repeated Start condition.

![acknowlegde.gif](https://github.com/wagiminator/ATtiny13-TinyOLEDdemo/blob/main/documentation/i2c-acknowledge.gif)

Each slave device on the bus should have a unique 7-bit address. The communication starts with the Start condition, followed by the 7-bit slave address and the data direction bit. If this bit is 0 then the master will write to the slave device. Otherwise, if the data direction bit is 1, the master will read from slave device. After the slave address and the data direction is sent, the master can continue with reading or writing. The communication is ended with the Stop condition which also signals that the I2C bus is free. If the master needs to communicate with other slaves it can generate a repeated start with another slave address without generation Stop condition. All the bytes are transferred with the MSB bit shifted first.

![command.gif](https://github.com/wagiminator/ATtiny13-TinyOLEDdemo/blob/main/documentation/i2c-command.gif)

If the master only writes to the slave device then the data transfer direction is not changed.

![address.gif](https://github.com/wagiminator/ATtiny13-TinyOLEDdemo/blob/main/documentation/i2c-7-bit-address-writing.gif)

# SSD1306 OLED
The functions for the OLED are adapted to the SSD1306 128x32 OLED module, but they can easily be modified to be used for other modules. In order to save resources, only the basic functionalities are implemented.  
