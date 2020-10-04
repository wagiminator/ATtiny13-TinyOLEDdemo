// tinyOLEDdemo - controlling an I2C OLED with an ATtiny13
//
// This is just a little demo on how to use an I2C OLED with the limited
// capabilities of an ATtiny13.
//
// The I2C protocol implementation is based on a crude bitbanging method.
// It was specifically designed for the limited resources of ATtiny10 and
// ATtiny13, but should work with some other AVRs as well.
// To make the code as compact as possible, the following restrictions apply:
// - the clock frequency of the MCU must not exceed 1.6 MHz,
// - the slave device must support fast mode 400 kbps (is mostly the case),
// - the slave device must not stretch the clock (this is usually the case),
// - the acknowledge bit sent by the slave device is ignored.
// Don't forget the pull-up resistors on the SDA and SCL lines! Many modules,
// such as the SSD1306 OLED module, have already integrated them.
//
// The functions for the OLED are adapted to the SSD1306 128x32 OLED module,
// but they can easily be modified to be used for other modules. In order to
// save resources, only the basic functionalities are implemented.  
//
//    +-----------------------------+
// ---|SDA +--------------------+   |
// ---|SCL |    SSD1306 OLED    |   |
// ---|VCC |       128x36       |   |
// ---|GND +--------------------+   |
//    +-----------------------------+
//
//                   +-\/-+
// --- A0 (D5) PB5  1|°   |8  Vcc
// --- A3 (D3) PB3  2|    |7  PB2 (D2) A1 --- SCL OLED
// --- A2 (D4) PB4  3|    |6  PB1 (D1) ------ 
//             GND  4|    |5  PB0 (D0) ------ SDA OLED
//                   +----+  
//
// Controller:  ATtiny13
// Clockspeed:  1.2 MHz internal
//
// Font used in this demo was adapted from Neven Boyanov and Stephen Denne.
// (https://github.com/datacute/Tiny4kOLED)
//
// 2020 by Stefan Wagner 
// Project Files (EasyEDA): https://easyeda.com/wagiminator
// Project Files (Github):  https://github.com/wagiminator
// License: http://creativecommons.org/licenses/by-sa/3.0/


#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#define I2C_SDA         PB0                     // serial data pin
#define I2C_SCL         PB2                     // serial clock pin
#define I2C_SDA_HIGH()  (DDRB &= ~(1<<I2C_SDA)) // release SDA   -> pulled HIGH by resistor
#define I2C_SDA_LOW()   (DDRB |=  (1<<I2C_SDA)) // SDA as output -> pulled LOW  by MCU
#define I2C_SCL_HIGH()  (DDRB &= ~(1<<I2C_SCL)) // release SCL   -> pulled HIGH by resistor
#define I2C_SCL_LOW()   (DDRB |=  (1<<I2C_SCL)) // SCL as output -> pulled LOW  by MCU
#define I2C_SDA_READ()  (PINB  &  (1<<I2C_SDA)) // read SDA line
#define I2C_SCL_READ()  (PINB  &  (1<<I2C_SCL)) // read SCL line

#define OLED_ADDR       0x78                    // OLED write address
#define OLED_HEIGHT     32                      // display height (32 or 64 pixels)
#define OLED_INIT_LEN   18                      // 18: no screen flip, 20: screen flip


// OLED init settings
const uint8_t OLED_INIT_CMD[] PROGMEM = {
  0xA8, 0x1F,       // set multiplex (HEIGHT-1): 0x1F for 128x32, 0x3F for 128x64 
  0x22, 0x00, 0x03, // set min and max page
  0x20, 0x00,       // set horizontal memory addressing mode
  0xD3, 0x00,       // set vertical shift to 0
  0xDA, 0x02,       // set COM pins hardware configuration to sequential
  0xDB, 0x40,       // set vcom detect 
  0xD9, 0xF1,       // set pre-charge period
  0x8D, 0x14,       // enable charge pump
  0xAF,             // switch on OLED
  0xA1, 0xC8        // flip the screen
};

// standard ASCII 5x8 font (adapted from Neven Boyanov and Stephen Denne)
const uint8_t OLED_font [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, //   0 
  0x00, 0x00, 0x2f, 0x00, 0x00, // ! 1 
  0x00, 0x07, 0x00, 0x07, 0x00, // " 2 
  0x14, 0x7f, 0x14, 0x7f, 0x14, // # 3 
  0x24, 0x2a, 0x7f, 0x2a, 0x12, // $ 4 
  0x62, 0x64, 0x08, 0x13, 0x23, // % 5 
  0x36, 0x49, 0x55, 0x22, 0x50, // & 6 
  0x00, 0x05, 0x03, 0x00, 0x00, // ' 7 
  0x00, 0x1c, 0x22, 0x41, 0x00, // ( 8 
  0x00, 0x41, 0x22, 0x1c, 0x00, // ) 9 
  0x14, 0x08, 0x3E, 0x08, 0x14, // * 10
  0x08, 0x08, 0x3E, 0x08, 0x08, // + 11
  0x00, 0x00, 0xA0, 0x60, 0x00, // , 12
  0x08, 0x08, 0x08, 0x08, 0x08, // - 13
  0x00, 0x60, 0x60, 0x00, 0x00, // . 14
  0x20, 0x10, 0x08, 0x04, 0x02, // / 15
  0x3E, 0x51, 0x49, 0x45, 0x3E, // 0 16
  0x00, 0x42, 0x7F, 0x40, 0x00, // 1 17
  0x42, 0x61, 0x51, 0x49, 0x46, // 2 18
  0x21, 0x41, 0x45, 0x4B, 0x31, // 3 19
  0x18, 0x14, 0x12, 0x7F, 0x10, // 4 20
  0x27, 0x45, 0x45, 0x45, 0x39, // 5 21
  0x3C, 0x4A, 0x49, 0x49, 0x30, // 6 22
  0x01, 0x71, 0x09, 0x05, 0x03, // 7 23
  0x36, 0x49, 0x49, 0x49, 0x36, // 8 24
  0x06, 0x49, 0x49, 0x29, 0x1E, // 9 25
  0x00, 0x36, 0x36, 0x00, 0x00, // : 26
  0x00, 0x56, 0x36, 0x00, 0x00, // ; 27
  0x08, 0x14, 0x22, 0x41, 0x00, // < 28
  0x14, 0x14, 0x14, 0x14, 0x14, // = 29
  0x00, 0x41, 0x22, 0x14, 0x08, // > 30
  0x02, 0x01, 0x51, 0x09, 0x06, // ? 31
  0x32, 0x49, 0x59, 0x51, 0x3E, // @ 32
  0x7C, 0x12, 0x11, 0x12, 0x7C, // A 33
  0x7F, 0x49, 0x49, 0x49, 0x36, // B 34
  0x3E, 0x41, 0x41, 0x41, 0x22, // C 35
  0x7F, 0x41, 0x41, 0x22, 0x1C, // D 36
  0x7F, 0x49, 0x49, 0x49, 0x41, // E 37
  0x7F, 0x09, 0x09, 0x09, 0x01, // F 38
  0x3E, 0x41, 0x49, 0x49, 0x7A, // G 39
  0x7F, 0x08, 0x08, 0x08, 0x7F, // H 40
  0x00, 0x41, 0x7F, 0x41, 0x00, // I 41
  0x20, 0x40, 0x41, 0x3F, 0x01, // J 42
  0x7F, 0x08, 0x14, 0x22, 0x41, // K 43
  0x7F, 0x40, 0x40, 0x40, 0x40, // L 44
  0x7F, 0x02, 0x0C, 0x02, 0x7F, // M 45
  0x7F, 0x04, 0x08, 0x10, 0x7F, // N 46
  0x3E, 0x41, 0x41, 0x41, 0x3E, // O 47
  0x7F, 0x09, 0x09, 0x09, 0x06, // P 48
  0x3E, 0x41, 0x51, 0x21, 0x5E, // Q 49
  0x7F, 0x09, 0x19, 0x29, 0x46, // R 50
  0x46, 0x49, 0x49, 0x49, 0x31, // S 51
  0x01, 0x01, 0x7F, 0x01, 0x01, // T 52
  0x3F, 0x40, 0x40, 0x40, 0x3F, // U 53
  0x1F, 0x20, 0x40, 0x20, 0x1F, // V 54
  0x3F, 0x40, 0x38, 0x40, 0x3F, // W 55
  0x63, 0x14, 0x08, 0x14, 0x63, // X 56
  0x07, 0x08, 0x70, 0x08, 0x07, // Y 57
  0x61, 0x51, 0x49, 0x45, 0x43, // Z 58
  0x00, 0x7F, 0x41, 0x41, 0x00, // [ 59
  0x02, 0x04, 0x08, 0x10, 0x20, // \ 60
  0x00, 0x41, 0x41, 0x7F, 0x00, // ] 61
  0x04, 0x02, 0x01, 0x02, 0x04, // ^ 62
  0x40, 0x40, 0x40, 0x40, 0x40  // _ 63
};

// messages to print on OLED
const char Message1[] PROGMEM = "HELLO WORLD !";
const char Message2[] PROGMEM = "ATTINY13 GOES OLED !";

// I2C init function
void I2C_init(void) {
  DDRB  &= ~((1<<I2C_SDA)|(1<<I2C_SCL));  // pins as input (HIGH-Z) -> lines released
  PORTB &= ~((1<<I2C_SDA)|(1<<I2C_SCL));  // should be LOW when as ouput
}

// I2C start condition
void I2C_start(void) {
  I2C_SDA_LOW();                          // SDA goes LOW first
  I2C_SCL_LOW();                          // SCL goes LOW second
}

// I2C stop condition
void I2C_stop(){
  I2C_SDA_LOW();                          // prepare SDA for LOW to HIGH transition
  I2C_SCL_HIGH();                         // SCL goes HIGH first
  I2C_SDA_HIGH();                         // SDA goes HIGH second
}

// transmit one data byte to the slave, ignore ACK bit, no clock stretching allowed
void I2C_write(uint8_t data) {
  for(uint8_t i = 8; i; i--, data<<=1) {  // transmit 8 bits
    I2C_SDA_LOW();                        // SDA LOW for now (saves some flash this way)
    if (data & 0x80) I2C_SDA_HIGH();      // SDA HIGH if bit is 1
    I2C_SCL_HIGH();                       // clock HIGH -> slave reads the bit
    I2C_SCL_LOW();                        // clock LOW again
  }
  I2C_SDA_HIGH();                         // release SDA for ACK bit of slave
  I2C_SCL_HIGH();                         // 9th clock pulse is for the ACK bit
  I2C_SCL_LOW();                          // but ACK bit is ignored
}

// receive one data byte from the slave (ack=0 for last byte, ack=1 if more bytes to follow)
uint8_t I2C_read(uint8_t ack) {
  uint8_t data = 0;                       // variable for the received byte
  I2C_SDA_HIGH();                         // release SDA -> will be toggled by slave
  for(uint8_t i =8; i; i--) {             // receive 8 bits
    data<<=1;                             // bits shifted in right (MSB first)
    I2C_SCL_HIGH();                       // clock HIGH
    if(I2C_SDA_READ()) data |=1;          // read bit
    I2C_SCL_LOW();                        // clock LOW -> slave prepares next bit
  }
  if (ack) I2C_SDA_LOW();                 // pull SDA LOW to acknowledge (ACK)
  I2C_SCL_HIGH();                         // clock HIGH -> slave reads ACK bit
  I2C_SCL_LOW();                          // clock LOW again
  I2C_SDA_HIGH();                         // release SDA line (needed????)
  return(data);                           // return the received byte
}

// initialize the OLED
void OLED_init(void) {
  I2C_init();                             // initialize I2C first
  I2C_start();                            // start transmission
  I2C_write(OLED_ADDR);                   // send address of OLED
  I2C_write(0x00);                        // set command mode
  for (uint8_t i = 0; i < OLED_INIT_LEN; i++) I2C_write(pgm_read_byte(&OLED_INIT_CMD[i])); // send the command bytes
  I2C_stop();                             // stop transmission
}

// print a character on the OLED
void OLED_printC(char ch) {
  uint16_t offset = (ch - 32) * 5;        // calculate position of character in font array
  I2C_write(0x00);                        // print spacing between characters
  for(uint8_t i=5; i; i--, offset++) I2C_write(pgm_read_byte(&OLED_font[offset])); // print character
}

// prints a string from program memory on the OLED
void OLED_printP(const char* p) {
  I2C_start();                            // start transmission
  I2C_write(OLED_ADDR);                   // send address of OLED
  I2C_write(0x40);                        // set data mode
  char ch = pgm_read_byte(p);             // read first character from program memory
  while (ch != 0) {                       // repeat until string terminator
    OLED_printC(ch);                      // print character on OLED
    ch = pgm_read_byte(++p);              // read next character
  }
  I2C_stop();                             // stop transmission
}

// sets the cursor on OLED
void OLED_cursor(uint8_t xpos, uint8_t ypos) {
  I2C_start();                            // start transmission
  I2C_write(OLED_ADDR);                   // send address of OLED
  I2C_write(0x00);                        // set command mode
  I2C_write(xpos & 0x0F);                 // set low nibble of start column
  I2C_write(0x10 | (xpos >> 4));          // set high nibble of start column
  I2C_write(0xB0 | (ypos & 0x07));        // set start page
  I2C_stop();                             // stop transmission
}

// clear screen on the OLED
void OLED_clear(void) {
  OLED_cursor(0, 0);                      // set cursor at upper left corner
  I2C_start();                            // start transmission
  I2C_write(OLED_ADDR);                   // send address of OLED
  I2C_write(0x40);                        // set data mode
  for(uint16_t i=512; i; i--) I2C_write(0x00); // clear the screen
  I2C_stop();                             // stop transmission
}

// main function
int main(void) {
  OLED_init();                            // initialize the OLED
  OLED_clear();                           // clear screen

  while(1) {                              // loop until forever
    // print messages
    OLED_cursor(20, 0);                   // set cursor position
    OLED_printP(Message1);                // print message 1
    _delay_ms(1000);                      // wait a second
    OLED_cursor(5, 2);                    // set cursor position
    OLED_printP(Message2);                // print message 2
    _delay_ms(5000);                      // wait 5 seconds

    // print all characters
    OLED_cursor(0, 0);                    // set cursor at upper left corner
    I2C_start();                          // start transmission
    I2C_write(OLED_ADDR);                 // send address of OLED
    I2C_write(0x40);                      // set data mode
    for(uint8_t i=32; i<64+32; i++) OLED_printC(i); // print all characters
    I2C_stop();                           // stop transmission
    _delay_ms(5000);                      // wait 5 seconds

    // scroll out the text
    for (uint8_t i=0; i<32; i++) {
      I2C_start();                        // start transmission
      I2C_write(OLED_ADDR);               // send address of OLED
      I2C_write(0x00);                    // set command mode
      I2C_write(0xD3);                    // vertical shift command
      I2C_write(i);                       // vertical shift value
      I2C_stop();                         // stop transmission
      _delay_ms(100);                     // delay a bit
    }
    OLED_clear();                         // clear screen
    I2C_start();                          // start transmission
    I2C_write(OLED_ADDR);                 // send address of OLED
    I2C_write(0x00);                      // set command mode
    I2C_write(0xD3);                      // vertical shift command
    I2C_write(0x00);                      // reset vertical shift value
    I2C_stop();                           // stop transmission
  }
}
