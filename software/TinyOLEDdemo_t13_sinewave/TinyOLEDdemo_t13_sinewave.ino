// ===================================================================================
// Project:   TinyOLED Demo - Sine Wave Animation
// Version:   v1.0
// Year:      2021
// Author:    Stefan Wagner
// Github:    https://github.com/wagiminator
// EasyEDA:   https://easyeda.com/wagiminator
// License:   http://creativecommons.org/licenses/by-sa/3.0/
// ===================================================================================
//
// Description:
// ------------
// Retro-style sine wave text animation.
//
// References:
// -----------
// Sine look up table generator calculator:
// https://www.daycounter.com/Calculators/Sine-Generator-Calculator.phtml
//
// OLED font was adapted from Neven Boyanov and Stephen Denne
// https://github.com/datacute/Tiny4kOLED
//
// Wiring:
// -------
//                         +-\/-+
//      --- RST ADC0 PB5  1|°   |8  Vcc
//      ------- ADC3 PB3  2|    |7  PB2 ADC1 -------- OLED SCL
//      ------- ADC2 PB4  3|    |6  PB1 AIN1 OC0B --- 
//                   GND  4|    |5  PB0 AIN0 OC0A --- OLED SDA
//                         +----+
//
// Compilation Settings:
// ---------------------
// Controller:  ATtiny13A
// Core:        MicroCore (https://github.com/MCUdude/MicroCore)
// Clockspeed:  9.6 MHz internal
// BOD:         BOD disabled
// Timing:      Micros disabled
//
// Leave the rest on default settings. Don't forget to "Burn bootloader"!
// No Arduino core functions or libraries are used. Use the makefile if 
// you want to compile without Arduino IDE.
//
// Fuse settings: -U lfuse:w:0x3a:m -U hfuse:w:0xff:m


// ===================================================================================
// Libraries and Definitions
// ===================================================================================

// Libraries
#include <avr/io.h>
#include <avr/pgmspace.h>

// Pin definitions
#define I2C_SDA         PB0                   // serial data pin
#define I2C_SCL         PB2                   // serial clock pin

// Message to print on OLED (21 characters)
const char Message[] PROGMEM = "ATTINY13 LOVES OLED !";

// ===================================================================================
// Sine Wave Look Up Table
// ===================================================================================

// Global variables
uint8_t sine_ptr;                             // sine wave table pointer

// Sine wave table (quarter wave, amplitude 23, 32 points as nibbles in 16 bytes)
const uint8_t SINE24[] PROGMEM = {
  0xCB, 0xAA, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44,
  0x33, 0x32, 0x22, 0x11, 0x11, 0x00, 0x00, 0x00
};

// ===================================================================================
// OLED Font
// ===================================================================================

// Standard ASCII 5x8 font (adapted from Neven Boyanov and Stephen Denne)
const uint8_t OLED_FONT[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2F, 0x00, 0x00, 0x00, 0x07, 0x00, 0x07, 0x00,
  0x14, 0x7F, 0x14, 0x7F, 0x14, 0x24, 0x2A, 0x7F, 0x2A, 0x12, 0x23, 0x13, 0x08, 0x64, 0x62,
  0x36, 0x49, 0x55, 0x22, 0x50, 0x00, 0x05, 0x03, 0x00, 0x00, 0x00, 0x1C, 0x22, 0x41, 0x00,
  0x00, 0x41, 0x22, 0x1C, 0x00, 0x14, 0x08, 0x3E, 0x08, 0x14, 0x08, 0x08, 0x3E, 0x08, 0x08,
  0x00, 0x00, 0xA0, 0x60, 0x00, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 0x60, 0x60, 0x00, 0x00,
  0x20, 0x10, 0x08, 0x04, 0x02, 0x3E, 0x51, 0x49, 0x45, 0x3E, 0x00, 0x42, 0x7F, 0x40, 0x00,
  0x42, 0x61, 0x51, 0x49, 0x46, 0x21, 0x41, 0x45, 0x4B, 0x31, 0x18, 0x14, 0x12, 0x7F, 0x10,
  0x27, 0x45, 0x45, 0x45, 0x39, 0x3C, 0x4A, 0x49, 0x49, 0x30, 0x01, 0x71, 0x09, 0x05, 0x03,
  0x36, 0x49, 0x49, 0x49, 0x36, 0x06, 0x49, 0x49, 0x29, 0x1E, 0x00, 0x36, 0x36, 0x00, 0x00,
  0x00, 0x56, 0x36, 0x00, 0x00, 0x08, 0x14, 0x22, 0x41, 0x00, 0x14, 0x14, 0x14, 0x14, 0x14,
  0x00, 0x41, 0x22, 0x14, 0x08, 0x02, 0x01, 0x51, 0x09, 0x06, 0x32, 0x49, 0x59, 0x51, 0x3E,
  0x7C, 0x12, 0x11, 0x12, 0x7C, 0x7F, 0x49, 0x49, 0x49, 0x36, 0x3E, 0x41, 0x41, 0x41, 0x22,
  0x7F, 0x41, 0x41, 0x22, 0x1C, 0x7F, 0x49, 0x49, 0x49, 0x41, 0x7F, 0x09, 0x09, 0x09, 0x01,
  0x3E, 0x41, 0x49, 0x49, 0x7A, 0x7F, 0x08, 0x08, 0x08, 0x7F, 0x00, 0x41, 0x7F, 0x41, 0x00,
  0x20, 0x40, 0x41, 0x3F, 0x01, 0x7F, 0x08, 0x14, 0x22, 0x41, 0x7F, 0x40, 0x40, 0x40, 0x40,
  0x7F, 0x02, 0x0C, 0x02, 0x7F, 0x7F, 0x04, 0x08, 0x10, 0x7F, 0x3E, 0x41, 0x41, 0x41, 0x3E,
  0x7F, 0x09, 0x09, 0x09, 0x06, 0x3E, 0x41, 0x51, 0x21, 0x5E, 0x7F, 0x09, 0x19, 0x29, 0x46,
  0x46, 0x49, 0x49, 0x49, 0x31, 0x01, 0x01, 0x7F, 0x01, 0x01, 0x3F, 0x40, 0x40, 0x40, 0x3F,
  0x1F, 0x20, 0x40, 0x20, 0x1F, 0x3F, 0x40, 0x38, 0x40, 0x3F, 0x63, 0x14, 0x08, 0x14, 0x63,
  0x07, 0x08, 0x70, 0x08, 0x07, 0x61, 0x51, 0x49, 0x45, 0x43, 0x00, 0x7F, 0x41, 0x41, 0x00,
  0x02, 0x04, 0x08, 0x10, 0x20, 0x00, 0x41, 0x41, 0x7F, 0x00, 0x04, 0x02, 0x01, 0x02, 0x04,
  0x40, 0x40, 0x40, 0x40, 0x40
};

// ===================================================================================
// I2C Master Implementation (Write only)
// ===================================================================================

// I2C macros
#define I2C_SDA_HIGH()  DDRB &= ~(1<<I2C_SDA) // release SDA   -> pulled HIGH by resistor
#define I2C_SDA_LOW()   DDRB |=  (1<<I2C_SDA) // SDA as output -> pulled LOW  by MCU
#define I2C_SCL_HIGH()  DDRB &= ~(1<<I2C_SCL) // release SCL   -> pulled HIGH by resistor
#define I2C_SCL_LOW()   DDRB |=  (1<<I2C_SCL) // SCL as output -> pulled LOW  by MCU

// I2C transmit one data byte to the slave, ignore ACK bit, no clock stretching allowed
void I2C_write(uint8_t data) {
  for(uint8_t i = 8; i; i--) {                // transmit 8 bits, MSB first
    I2C_SDA_LOW();                            // SDA LOW for now (saves some flash this way)
    if (data & 0x80) I2C_SDA_HIGH();          // SDA HIGH if bit is 1
    I2C_SCL_HIGH();                           // clock HIGH -> slave reads the bit
    data<<=1;                                 // shift left data byte, acts also as a delay
    I2C_SCL_LOW();                            // clock LOW again
  }
  I2C_SDA_HIGH();                             // release SDA for ACK bit of slave
  I2C_SCL_HIGH();                             // 9th clock pulse is for the ACK bit
  asm("nop");                                 // ACK bit is ignored, just a delay
  I2C_SCL_LOW();                              // clock LOW again
}

// I2C start transmission
void I2C_start(uint8_t addr) {
  I2C_SDA_LOW();                              // start condition: SDA goes LOW first
  I2C_SCL_LOW();                              // start condition: SCL goes LOW second
  I2C_write(addr);                            // send slave address
}

// I2C stop transmission
void I2C_stop(void) {
  I2C_SDA_LOW();                              // prepare SDA for LOW to HIGH transition
  I2C_SCL_HIGH();                             // stop condition: SCL goes HIGH first
  I2C_SDA_HIGH();                             // stop condition: SDA goes HIGH second
}

// ===================================================================================
// OLED Implementation
// ===================================================================================

// OLED definitions
#define OLED_ADDR       0x78                  // OLED write address
#define OLED_CMD_MODE   0x00                  // set command mode
#define OLED_DAT_MODE   0x40                  // set data mode
#define OLED_INIT_LEN   12                    // 12: no screen flip, 14: screen flip

// OLED init settings
const uint8_t OLED_INIT_CMD[] PROGMEM = {
  0xA8, 0x1F,         // set multiplex (HEIGHT-1): 0x1F for 128x32, 0x3F for 128x64 
  0x22, 0x00, 0x03,   // set min and max page
  0x20, 0x01,         // set vertical memory addressing mode
  0xDA, 0x02,         // set COM pins hardware configuration to sequential
  0x8D, 0x14,         // enable charge pump
  0xAF,               // switch on OLED
  0xA1, 0xC8          // flip the screen
};

// OLED init function
void OLED_init(void) {
  I2C_start(OLED_ADDR);                       // start transmission to OLED
  I2C_write(OLED_CMD_MODE);                   // set command mode and send command bytes ...
  for (uint8_t i = 0; i < OLED_INIT_LEN; i++) I2C_write(pgm_read_byte(&OLED_INIT_CMD[i]));
  I2C_stop();                                 // stop transmission
}

// OLED plot a character
void OLED_plotChar(char c) {
  uint16_t offset = c - 32;                   // calculate position of character in font array
  offset += offset << 2;                      // -> offset = (c - 32) * 5
  for(uint8_t i=4; i; i--) I2C_write(0x00);   // print spacing between characters
  sine_ptr++;                                 // increase sine table pointer
  for(uint8_t i=5; i; i--) {                  // character consists of 5 lines
    uint32_t ch = pgm_read_byte(&OLED_FONT[offset++]); // read line of character
    uint8_t  pt = sine_ptr & 0x1F;            // get quarter part of pointer
    if(sine_ptr & 0x20) pt = 0x1F - pt;       // mirror on the y-axis, if necessary
    uint8_t sh = pgm_read_byte(&SINE24[pt>>1]);  // read sine value
    (pt & 1) ? (sh &= 0x0F) : (sh >>= 4);     // get correct nibble
    if(sine_ptr & 0x40) sh = 0x17 - sh;       // mirror on the x-axis, if necessary
    ch <<= sh;                                // shift char according to sine table value
    sine_ptr++;                               // increase sine table pointer
    for(uint8_t i=4; i; i--) {                // write the shifted line on the OLED ...
      I2C_write(ch);
      ch >>= 8;
    }
  }
}

// OLED print a string from program memory
void OLED_print(const char* p) {
  I2C_start(OLED_ADDR);                       // start transmission to OLED
  I2C_write(OLED_DAT_MODE);                   // set data mode
  char ch = pgm_read_byte(p);                 // read first character from program memory
  while (ch != 0) {                           // repeat until string terminator
    OLED_plotChar(ch);                          // print character on OLED
    ch = pgm_read_byte(++p);                  // read next character
  }
  I2C_stop();                                 // stop transmission
}

// OLED set the cursor
void OLED_cursor(uint8_t xpos, uint8_t ypos) {
  I2C_start(OLED_ADDR);                       // start transmission to OLED
  I2C_write(OLED_CMD_MODE);                   // set command mode
  I2C_write(xpos & 0x0F);                     // set low nibble of start column
  I2C_write(0x10 | (xpos >> 4));              // set high nibble of start column
  I2C_write(0xB0 | (ypos & 0x07));            // set start page
  I2C_stop();                                 // stop transmission
}

// OLED clear screen
void OLED_clear(void) {
  OLED_cursor(0, 0);                          // set cursor at upper left corner
  I2C_start(OLED_ADDR);                       // start transmission to OLED
  I2C_write(OLED_DAT_MODE);                   // set data mode
  for(uint16_t i=512; i; i--) I2C_write(0x00);// clear the screen
  I2C_stop();                                 // stop transmission
}

// ===================================================================================
// Main Function
// ===================================================================================

int main(void) {
  // Setup
  OLED_init();                                // initialize the OLED
  OLED_clear();                               // clear screen

  // Loop
  while(1) {                                  // loop until forever                         
    // Animate messages
    OLED_cursor(0, 0);                        // set cursor position
    OLED_print(Message);                      // print message
    sine_ptr--;                               // shift whole wave to the right
  }
}
