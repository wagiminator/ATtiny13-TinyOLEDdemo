// tinyOLEDdemo - controlling an I²C OLED with an ATtiny202
//
// This is just a little demo on how to use an I²C OLED with the limited
// capabilities of an ATtiny202.
//
// The I²C protocol implementation is based on the hardware TWI (two wire
// interface) of the tinyAVR. In order to make it as small as possible,
// most status flags including the acknowledge bit are ignored. A function
// for reading from the slave was omitted because it is not necessary here.
// Overall, the I2C implementation only takes up 60 bytes of flash.
//
// The functions for the OLED are adapted to the SSD1306 128x32 OLED module,
// but they can easily be modified to be used for other modules. In order to
// save resources, only the basic functionalities are implemented.  
//
// Ralph Doncaster (nerdralph) pointed out that the SSD1306 can be controlled
// much faster than specified. Therefore an I²C clock rate of 800 kHz is also
// possible in this case.
//
// Don't forget the pull-up resistors on the SDA and SCL lines! Many modules,
// such as the SSD1306 OLED module, have already integrated them.
//
//
//    +-----------------------------+
// ---|SDA +--------------------+   |
// ---|SCL |    SSD1306 OLED    |   |
// ---|VCC |       128x36       |   |
// ---|GND +--------------------+   |
//    +-----------------------------+
//
//                         +-\/-+
//                   Vcc  1|°   |8  GND
//              (D0) PA6  2|    |7  PA3 (D4)
//              (D1) PA7  3|    |6  PA0 (D5) --- UPDI
// SDA OLED --- (D2) PA1  4|    |5  PA2 (D3) --- SCL OLED
//                         +----+  
//
// Core:    megaTinyCore (https://github.com/SpenceKonde/megaTinyCore)
// Board:   ATtiny412/402/212/202
// Chip:    ATtiny202
// Clock:   10 Mhz internal
// Leave the rest on default settings. Don't forget to "Burn bootloader"!
// No Arduino core functions or libraries are used.
//
// Font used in this demo was adapted from Neven Boyanov and Stephen Denne.
// ( https://github.com/datacute/Tiny4kOLED )
//
// A big thank you to Ralph Doncaster (nerdralph) for his optimization tips.
// ( https://nerdralph.blogspot.com/ , https://github.com/nerdralph )
//
// AVR toolchain for the new tinyAVR microcontrollers:
// https://www.microchip.com/mplab/avr-support/avr-and-arm-toolchains-c-compilers
//
// 2021 by Stefan Wagner 
// Project Files (EasyEDA): https://easyeda.com/wagiminator
// Project Files (Github):  https://github.com/wagiminator
// License: http://creativecommons.org/licenses/by-sa/3.0/


#include <avr/io.h>
#include <util/delay.h>

// -----------------------------------------------------------------------------
// I2C Master Implementation (Write only)
// -----------------------------------------------------------------------------

#define I2C_FREQ  800000L                         // I2C clock frequency in Hz
#define I2C_BAUD  ((F_CPU / I2C_FREQ) - 10) / 2;  // simplified BAUD calculation

// I2C init function
void I2C_init(void) {
  TWI0.MBAUD   = I2C_BAUD;                        // set BAUD rate
  TWI0.MCTRLA  = TWI_ENABLE_bm;                   // enable TWI
  TWI0.MSTATUS = TWI_BUSSTATE_IDLE_gc;            // set bus state to idle
}

// I2C start transmission
void I2C_start(uint8_t addr) {
  TWI0.MADDR = addr;                              // start sending address
}

// I2C stop transmission
void I2C_stop(void) {
  while (~TWI0.MSTATUS & TWI_WIF_bm);             // wait for last transfer to complete
  TWI0.MCTRLB |= TWI_MCMD_STOP_gc;                // send stop condition
}

// I2C transmit one data byte to the slave, ignore ACK bit
void I2C_write(uint8_t data) {
  while (~TWI0.MSTATUS & TWI_WIF_bm);             // wait for last transfer to complete
  TWI0.MDATA = data;                              // start sending data byte 
}

// -----------------------------------------------------------------------------
// OLED Implementation
// -----------------------------------------------------------------------------

// OLED definitions
#define OLED_ADDR       0x78                  // OLED write address
#define OLED_CMD_MODE   0x00                  // set command mode
#define OLED_DAT_MODE   0x40                  // set data mode
#define OLED_INIT_LEN   12                    // 12: no screen flip, 14: screen flip

// OLED init settings
const uint8_t OLED_INIT_CMD[] = {
  0xA8, 0x1F,       // set multiplex (HEIGHT-1): 0x1F for 128x32, 0x3F for 128x64 
  0x22, 0x00, 0x03, // set min and max page
  0x20, 0x00,       // set horizontal memory addressing mode
  0xDA, 0x02,       // set COM pins hardware configuration to sequential
  0x8D, 0x14,       // enable charge pump
  0xAF,             // switch on OLED
  0xA1, 0xC8        // flip the screen
};

// standard ASCII 5x8 font (adapted from Neven Boyanov and Stephen Denne)
const uint8_t OLED_FONT[] = {
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

// OLED init function
void OLED_init(void) {
  I2C_init();                             // initialize I2C first
  I2C_start(OLED_ADDR);                   // start transmission to OLED
  I2C_write(OLED_CMD_MODE);               // set command mode
  for (uint8_t i = 0; i < OLED_INIT_LEN; i++) I2C_write(OLED_INIT_CMD[i]); // send the command bytes
  I2C_stop();                             // stop transmission
}

// OLED set vertical shift
void OLED_shift(uint8_t ypos) {
  I2C_start(OLED_ADDR);                   // start transmission to OLED
  I2C_write(OLED_CMD_MODE);               // set command mode
  I2C_write(0xD3);                        // vertical shift command
  I2C_write(ypos);                        // set vertical shift value
  I2C_stop();                             // stop transmission
}

// OLED print a character
void OLED_printC(char ch) {
  uint16_t offset = ch - 32;              // calculate position of character in font array
  offset += offset << 2;                  // -> offset = (ch - 32) * 5
  I2C_write(0x00);                        // print spacing between characters
  for(uint8_t i=5; i; i--) I2C_write(OLED_FONT[offset++]); // print character
}

// OLED print a string (by default from program memory)
void OLED_printS(const char* p) {
  I2C_start(OLED_ADDR);                   // start transmission to OLED
  I2C_write(OLED_DAT_MODE);               // set data mode
  while(*p) OLED_printC(*p++);            // print each character of the string
  I2C_stop();                             // stop transmission
}

// OLED set the cursor
void OLED_cursor(uint8_t xpos, uint8_t ypos) {
  I2C_start(OLED_ADDR);                   // start transmission to OLED
  I2C_write(OLED_CMD_MODE);               // set command mode
  I2C_write(xpos & 0x0F);                 // set low nibble of start column
  I2C_write(0x10 | (xpos >> 4));          // set high nibble of start column
  I2C_write(0xB0 | (ypos & 0x07));        // set start page
  I2C_stop();                             // stop transmission
}

// OLED clear screen
void OLED_clear(void) {
  OLED_cursor(0, 0);                      // set cursor at upper left corner
  I2C_start(OLED_ADDR);                   // start transmission to OLED
  I2C_write(OLED_DAT_MODE);               // set data mode
  for(uint16_t i=512; i; i--) I2C_write(0x00); // clear the screen
  I2C_stop();                             // stop transmission
  OLED_shift(0);                          // reset vertical shift
}

// -----------------------------------------------------------------------------
// Main Function
// -----------------------------------------------------------------------------

int main(void) {
  // Setup
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, 1); // set clock frequency to 10MHz
  OLED_init();                            // setup I2C OLED

  // Loop
  while(1) {                              // loop until forever                         
    // print messages
    OLED_clear();                         // clear screen
    OLED_cursor(20, 0);                   // set cursor position
    OLED_printS("HELLO WORLD !");         // print string
    _delay_ms(1000);                      // wait a second
    OLED_cursor(5, 2);                    // set cursor position
    OLED_printS("ATTINY202 GOES OLED!");  // print string
    _delay_ms(4000);                      // wait 4 seconds
    OLED_clear();
    OLED_printS("THE QUICK BROWN FOX");   // print message 3
    OLED_cursor(0, 1);                    // set cursor next line
    OLED_printS("JUMPS OVER THE LAZY");   // print message 4
    OLED_cursor(0, 2);                    // set cursor next line
    OLED_printS("DOG  - (0123456789)");   // print message 5
    _delay_ms(4000);                      // wait 4 seconds

    // print all characters
    OLED_cursor(0, 0);                    // set cursor at upper left corner
    I2C_start(OLED_ADDR);                 // start transmission to OLED
    I2C_write(OLED_DAT_MODE);             // set data mode
    for(uint8_t i=32; i<64+32; i++) OLED_printC(i); // print all characters
    I2C_stop();                           // stop transmission
    _delay_ms(3000);                      // wait 3 seconds

    // scroll out the text
    for (uint8_t i=0; i<32; i++) {        // shift 32 pixels upwards
      OLED_shift(i);                      // set vertical shift value
      _delay_ms(100);                     // delay a bit
    }
  }
}
