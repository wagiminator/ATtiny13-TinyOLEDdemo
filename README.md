# TinyOLEDdemo - I²C OLED on an ATtiny13
This is just a little demo on how to use an I²C OLED with the limited capabilities of an ATtiny13.

![pic1.jpg](https://github.com/wagiminator/ATtiny13-TinyOLEDdemo/blob/main/documentation/TinyOLEDdemo_pic1.jpg)
![pic2.jpg](https://github.com/wagiminator/ATtiny13-TinyOLEDdemo/blob/main/documentation/TinyOLEDdemo_pic2.jpg)
![pic3.jpg](https://github.com/wagiminator/ATtiny13-TinyOLEDdemo/blob/main/documentation/TinyOLEDdemo_pic3.jpg)

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

# I²C Implementation
The I²C protocol implementation is based on a crude bitbanging method. It was specifically designed for the limited resources of ATtiny10 and ATtiny13, but should work with some other AVRs as well. To make the code as compact as possible, the following restrictions apply:
- the clock frequency of the MCU must not exceed 4.8 MHz,
- the slave device must support fast mode 400 kbps (is mostly the case),
- the slave device must not stretch the clock (this is usually the case),
- the acknowledge bit sent by the slave device is ignored.

If these restrictions are observed, the implementation works almost without delays. An SCL HIGH must be at least 600ns long in Fast Mode. At a maximum clock rate of 4.8 MHz, this is shorter than three clock cycles. An SCL LOW must be at least 1300ns long. Since the SDA signal has to be applied at this point anyway, a total of at least six clock cycles pass. Ignoring the ACK signal and disregarding clock stretching also saves a few bytes of flash. A function for reading from the slave was omitted because it is not necessary here. Overall, the I2C implementation only takes up 56 bytes of flash.

A big thank you at this point goes to Ralph Doncaster (nerdralph) for his optimization tips. He also pointed out that the SSD1306 can be controlled much faster than specified. Therefore an MCU clock rate of 9.6 MHz is also possible in this case.

```c
// I2C definitions
#define I2C_SDA         PB0                   // serial data pin
#define I2C_SCL         PB2                   // serial clock pin
#define I2C_SDA_HIGH()  DDRB &= ~(1<<I2C_SDA) // release SDA   -> pulled HIGH by resistor
#define I2C_SDA_LOW()   DDRB |=  (1<<I2C_SDA) // SDA as output -> pulled LOW  by MCU
#define I2C_SCL_HIGH()  DDRB &= ~(1<<I2C_SCL) // release SCL   -> pulled HIGH by resistor
#define I2C_SCL_LOW()   DDRB |=  (1<<I2C_SCL) // SCL as output -> pulled LOW  by MCU

// I2C init function
void I2C_init(void) {
  DDRB  &= ~((1<<I2C_SDA)|(1<<I2C_SCL));  // pins as input (HIGH-Z) -> lines released
  PORTB &= ~((1<<I2C_SDA)|(1<<I2C_SCL));  // should be LOW when as ouput
}

// I2C start transmission
void I2C_start(uint8_t addr) {
  I2C_SDA_LOW();                          // start condition: SDA goes LOW first
  I2C_SCL_LOW();                          // start condition: SCL goes LOW second
  I2C_write(addr);                        // send slave address
}

// I2C stop transmission
void I2C_stop(void) {
  I2C_SDA_LOW();                          // prepare SDA for LOW to HIGH transition
  I2C_SCL_HIGH();                         // stop condition: SCL goes HIGH first
  I2C_SDA_HIGH();                         // stop condition: SDA goes HIGH second
}

// I2C transmit one data byte to the slave, ignore ACK bit, no clock stretching allowed
void I2C_write(uint8_t data) {
  for(uint8_t i = 8; i; i--) {            // transmit 8 bits, MSB first
    I2C_SDA_LOW();                        // SDA LOW for now (saves some flash this way)
    if (data & 0x80) I2C_SDA_HIGH();      // SDA HIGH if bit is 1
    I2C_SCL_HIGH();                       // clock HIGH -> slave reads the bit
    data<<=1;                             // shift left data byte, acts also as a delay
    I2C_SCL_LOW();                        // clock LOW again
  }
  I2C_SDA_HIGH();                         // release SDA for ACK bit of slave
  I2C_SCL_HIGH();                         // 9th clock pulse is for the ACK bit
  asm("nop");                             // ACK bit is ignored, just a delay
  I2C_SCL_LOW();                          // clock LOW again
}
```

Don't forget the pull-up resistors on the SDA and SCL lines! Many modules, such as the SSD1306 OLED module, have already integrated them.

Here is the result at a clock rate of 4.8 MHz. It doesn't quite meet the specification, but so far it works without any problems.

![I2C@4.8MHz.jpg](https://github.com/wagiminator/ATtiny13-TinyOLEDdemo/blob/main/documentation/I2C@4.8MHz.jpg)

# SSD1306 OLED
The functions for the OLED are adapted to the SSD1306 128x32 OLED module, but they can easily be modified to be used for other modules. In order to save resources, only the basic functionalities are implemented.

```c
// OLED definitions
#define OLED_ADDR       0x78        // OLED write address
#define OLED_CMD_MODE   0x00        // set command mode
#define OLED_DAT_MODE   0x40        // set data mode
#define OLED_INIT_LEN   16          // 16: no screen flip, 18: screen flip

// OLED init settings
const uint8_t OLED_INIT_CMD[] PROGMEM = {
  0xA8, 0x1F,                       // set multiplex (HEIGHT-1): 0x1F for 128x32, 0x3F for 128x64 
  0x22, 0x00, 0x03,                 // set min and max page
  0x20, 0x00,                       // set horizontal memory addressing mode
  0xDA, 0x02,                       // set COM pins hardware configuration to sequential
  0xDB, 0x40,                       // set vcom detect 
  0xD9, 0xF1,                       // set pre-charge period
  0x8D, 0x14,                       // enable charge pump
  0xAF,                             // switch on OLED
  0xA1, 0xC8                        // flip the screen
};

// standard ASCII 5x8 font
const uint8_t OLED_FONT[] PROGMEM = {
  // not shown here
};

// OLED init function
void OLED_init(void) {
  I2C_init();                       // initialize I2C first
  I2C_start(OLED_ADDR);             // start transmission to OLED
  I2C_write(OLED_CMD_MODE);         // set command mode
  for (uint8_t i = 0; i < OLED_INIT_LEN; i++) I2C_write(pgm_read_byte(&OLED_INIT_CMD[i])); // send the command bytes
  I2C_stop();                       // stop transmission
}

// OLED print a character
void OLED_printC(char ch) {
  uint16_t offset = ch - 32;        // calculate position of character in font array
  offset += offset << 2;            // -> offset = (ch - 32) * 5
  I2C_write(0x00);                  // print spacing between characters
  for(uint8_t i=5; i; i--) I2C_write(pgm_read_byte(&OLED_FONT[offset++])); // print character
}

// OLED print a string from program memory
void OLED_printP(const char* p) {
  I2C_start(OLED_ADDR);             // start transmission to OLED
  I2C_write(OLED_DAT_MODE);         // set data mode
  char ch = pgm_read_byte(p);       // read first character from program memory
  while (ch != 0) {                 // repeat until string terminator
    OLED_printC(ch);                // print character on OLED
    ch = pgm_read_byte(++p);        // read next character
  }
  I2C_stop();                       // stop transmission
}

// OLED set the cursor
void OLED_cursor(uint8_t xpos, uint8_t ypos) {
  I2C_start(OLED_ADDR);             // start transmission to OLED
  I2C_write(OLED_CMD_MODE);         // set command mode
  I2C_write(xpos & 0x0F);           // set low nibble of start column
  I2C_write(0x10 | (xpos >> 4));    // set high nibble of start column
  I2C_write(0xB0 | (ypos & 0x07));  // set start page
  I2C_stop();                       // stop transmission
}

// OLED clear screen
void OLED_clear(void) {
  OLED_cursor(0, 0);                // set cursor at upper left corner
  I2C_start(OLED_ADDR);             // start transmission to OLED
  I2C_write(OLED_DAT_MODE);         // set data mode
  for(uint16_t i=512; i; i--) I2C_write(0x00); // clear the screen
  I2C_stop();                       // stop transmission
}
```
