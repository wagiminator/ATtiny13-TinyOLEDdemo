// tinyOLEDsegment - using an I²C OLED as an 8-digit 7-segment display
//                   with an ATtiny13
//
// This is just a little demo on how to use an I²C OLED as an 8-digit
// 7-segment display with the limited capabilities of an ATtiny13. It
// implements a 24-bit hexadecimal counter.
//
// The I²C protocol implementation is based on a crude bitbanging method.
// It was specifically designed for the limited resources of ATtiny10 and
// ATtiny13, but should work with some other AVRs as well.
// To make the code as compact as possible, the following restrictions apply:
// - the clock frequency of the MCU must not exceed 4.8 MHz,
// - the slave device must support fast mode 400 kbps (is mostly the case),
// - the slave device must not stretch the clock (this is usually the case),
// - the acknowledge bit sent by the slave device is ignored.
// If these restrictions are observed, the implementation works almost without
// delays. An SCL HIGH must be at least 600ns long in Fast Mode. At a maximum
// clock rate of 4.8 MHz, this is shorter than three clock cycles. An SCL LOW
// must be at least 1300ns long. Since the SDA signal has to be applied anyway,
// a total of at least six clock cycles pass. Ignoring the ACK signal and
// disregarding clock stretching also saves a few bytes of flash. A function
// for reading from the slave was omitted because it is not necessary here.
// Overall, the I2C implementation only takes up 56 bytes of flash.
//
// Ralph Doncaster (nerdralph) pointed out that the SSD1306 can be controlled
// much faster than specified. Therefore an MCU clock rate of 9.6 MHz is also
// possible in this case.
//
// Don't forget the pull-up resistors on the SDA and SCL lines! Many modules,
// such as the SSD1306 OLED module, have already integrated them.
//
// The functions for the OLED are adapted to the SSD1306 128x32 or 128x64 OLED module,
// but they can easily be modified to be used for other modules. In order to
// save resources, only the basic functionalities are implemented.  
//
//    +-----------------------------+
// ---|SDA +--------------------+   |
// ---|SCL |    SSD1306 OLED    |   |
// ---|VCC |   128x32 (or 64)   |   |
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
// Controller: ATtiny13
// Core:       MicroCore (https://github.com/MCUdude/MicroCore)
// Clockspeed: 4.8 (or 9.6) MHz internal
// BOD:        BOD disabled
// Timing:     Micros disabled
// Leave the rest on default settings. Don't forget to "Burn bootloader"!
// No Arduino core functions or libraries are used. Use the makefile to
// compile without Arduino IDE.
//
// A big thank you to Ralph Doncaster (nerdralph) for his optimization tips.
// ( https://nerdralph.blogspot.com/ , https://github.com/nerdralph )
//
// 2020 by Stefan Wagner 
// Project Files (EasyEDA): https://easyeda.com/wagiminator
// Project Files (Github):  https://github.com/wagiminator
// License: http://creativecommons.org/licenses/by-sa/3.0/


// Select the screen size
#define SCREEN_128x32
//#define SCREEN_128x64


#if defined(SCREEN_128x32) and defined(SCREEN_128x64)
#error "Please define either SCREEN_128x32 or SCREEN_128x64 but not both!"
#endif
#if !defined(SCREEN_128x32) and !defined(SCREEN_128x64)
#error "Please define one of SCREEN_128x32 or SCREEN_128x64!"
#endif

// Libraries
#include <avr/io.h>
#include <avr/pgmspace.h>

// Pin definitions
#define I2C_SDA         PB0                   // serial data pin
#define I2C_SCL         PB2                   // serial clock pin

// -----------------------------------------------------------------------------
// I2C Master Implementation (Write only)
// -----------------------------------------------------------------------------

// I2C macros
#define I2C_SDA_HIGH()  DDRB &= ~(1<<I2C_SDA) // release SDA   -> pulled HIGH by resistor
#define I2C_SDA_LOW()   DDRB |=  (1<<I2C_SDA) // SDA as output -> pulled LOW  by MCU
#define I2C_SCL_HIGH()  DDRB &= ~(1<<I2C_SCL) // release SCL   -> pulled HIGH by resistor
#define I2C_SCL_LOW()   DDRB |=  (1<<I2C_SCL) // SCL as output -> pulled LOW  by MCU

// I2C init function
void I2C_init(void) {
  DDRB  &= ~((1<<I2C_SDA)|(1<<I2C_SCL));  // pins as input (HIGH-Z) -> lines released
  PORTB &= ~((1<<I2C_SDA)|(1<<I2C_SCL));  // should be LOW when as ouput
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

// -----------------------------------------------------------------------------
// OLED Implementation
// -----------------------------------------------------------------------------

// OLED definitions
#define OLED_ADDR       0x78                  // OLED write address
#define OLED_CMD_MODE   0x00                  // set command mode
#define OLED_DAT_MODE   0x40                  // set data mode
#if defined(SCREEN_128x32)
#define OLED_INIT_LEN   12                    // 12: no screen flip, 14: screen flip
#define MULTIPLEX 0x1F
#else
#define OLED_INIT_LEN   7                     // 7: no screen flip, 9: screen flip
#define MULTIPLEX 0x3F
#endif

// OLED init settings
const uint8_t OLED_INIT_CMD[] PROGMEM = {
  0xA8, MULTIPLEX,  // set multiplex (HEIGHT-1): 0x1F for 128x32, 0x3F for 128x64
  0x20, 0x01,       // set vertical memory addressing mode
//  0x22, 0x00, 0x03, // set min and max page => set in OLED_init
#if defined(SCREEN_128x32)
  0xDA, 0x02,       // set COM pins hardware configuration to sequential
#endif
  0x8D, 0x14,       // enable charge pump
  0xAF,             // switch on OLED
#if defined(SCREEN_128x32)
  0x00, 0x10, 0xB0, // set cursor at home position
#endif
  0xA1, 0xC8        // flip the screen
};

// Simple reduced 3x8 font
const uint8_t OLED_FONT[] PROGMEM = {
  0x7F, 0x41, 0x7F, // 0  0
  0x00, 0x00, 0x7F, // 1  1
  0x79, 0x49, 0x4F, // 2  2
  0x41, 0x49, 0x7F, // 3  3
  0x0F, 0x08, 0x7E, // 4  4
  0x4F, 0x49, 0x79, // 5  5
  0x7F, 0x49, 0x79, // 6  6
  0x03, 0x01, 0x7F, // 7  7
  0x7F, 0x49, 0x7F, // 8  8
  0x4F, 0x49, 0x7F, // 9  9
  0x7F, 0x09, 0x7F, // A 10
  0x7F, 0x48, 0x78, // b 11
  0x7F, 0x41, 0x63, // C 12
  0x78, 0x48, 0x7F, // d 13
  0x7F, 0x49, 0x41, // E 14
  0x7F, 0x09, 0x01, // F 15
  0x00, 0x60, 0x00, // . 16
  0x00, 0x36, 0x00, // : 17
  0x08, 0x08, 0x08, // - 18
  0x00, 0x00, 0x00  //   19
};

// OLED init function
void OLED_init(void) {
  I2C_init();                             // initialize I2C first
  I2C_start(OLED_ADDR);                   // start transmission to OLED
  I2C_write(OLED_CMD_MODE);               // set command mode
  for (uint8_t i = 0; i < OLED_INIT_LEN; i++) I2C_write(pgm_read_byte(&OLED_INIT_CMD[i])); // send the command bytes
  I2C_stop();                             // stop transmission

  // For the 128x64 screen, we'll only use half the height (vertically centered), so clear the whole screen
#if defined(SCREEN_128x64)
  I2C_start(OLED_ADDR);                   // start transmission to OLED
  I2C_write(OLED_DAT_MODE);               // set data mode
  for(uint16_t i=1024; i; i--) I2C_write(0x00);
  I2C_stop();                             // stop transmission
#endif

  // Set min and max pages. Probably not required for 128x32 since we
  // use the whole height.
  I2C_start(OLED_ADDR);
  I2C_write(OLED_CMD_MODE);
  I2C_write(0x22);
#if defined(SCREEN_128x32)
  I2C_write(0x00);
  I2C_write(0x03);
#else // Center vertically for 128x64 screen
  I2C_write(0x02);
  I2C_write(0x05);
#endif
  I2C_stop();
}

// OLED stretch a part of a byte
uint8_t OLED_stretch(uint8_t b) {
  b  = ((b & 2) << 3) | (b & 1);          // split 2 LSB into the nibbles
  b |= b << 1;                            // double the bits
  b |= b << 2;                            // double them again = 4 times
  return b;                               // return the value
}

// OLED print a big digit
void OLED_printD(uint8_t ch) {
  uint8_t i, j, k, b;                     // loop variables
  uint8_t sb[4];                          // stretched character bytes
  ch += ch << 1;                          // calculate position of character in font array
  for(i=8; i; i--) I2C_write(0x00);       // print spacing between characters
  for(i=3; i; i--) {                      // font has 3 bytes per character
    b = pgm_read_byte(&OLED_FONT[ch++]);  // read character byte
    for(j=0; j<4; j++, b >>= 2) sb[j] = OLED_stretch(b);  // stretch 4 times
    j=4; if(i==2) j=6;                    // calculate x-stretch value
    while(j--) {                       // write several times (x-direction)
      for(k=0; k<4; k++) I2C_write(sb[k]);// the 4 stretched bytes (y-direction)
    }
  } 
}

// OLED print buffer
void OLED_printB(uint8_t *buffer) {
  I2C_start(OLED_ADDR);                   // start transmission to OLED
  I2C_write(OLED_DAT_MODE);               // set data mode
  for(uint8_t i=0; i<8; i++) OLED_printD(buffer[i]);  // print buffer
  I2C_stop();                             // stop transmission
}

// -----------------------------------------------------------------------------
// Main Function
// -----------------------------------------------------------------------------

int main(void) {
  // Variables
  uint8_t buffer[8] = {0, 0, 17, 0, 0, 16, 0, 0};       // screen buffer
  uint8_t counter_a = 0, counter_b = 0, counter_c = 0;  // 8-bit counter variables
  
  // Setup
  OLED_init();                            // initialize the OLED

  // Loop
  while(1) {                              // loop until forever                         
    OLED_printB(buffer);                  // print screen buffer
    counter_a++;                          // increase counter a
    if(!counter_a) {                      // if counter a overflows:
      counter_b++;                        // increase counter b
      if (!counter_b) counter_c++;        // if counter b overflows increase counter c
    }
    buffer[7] = counter_a & 0x0F;         // low nibble of counter a
    buffer[6] = counter_a >> 4;           // high nibble of counter a
    buffer[4] = counter_b & 0x0F;         // low nibble of counter b
    buffer[3] = counter_b >> 4;           // high nibble of counter b
    buffer[1] = counter_c & 0x0F;         // low nibble of counter c
    buffer[0] = counter_c >> 4;           // high nibble of counter c
    buffer[2] = 19;                       // set space between c and b for now
    if(counter_a & 0x20) buffer[2] = 17;  // toggle ':' at this position
  }
}
