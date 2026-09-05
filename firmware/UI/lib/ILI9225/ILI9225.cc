#include "stm32f0xx_hal.h"
#include "ILI9225.h"
#include <math.h>
#include <string.h>

#define rcast reinterpret_cast
#define scast static_cast

extern SPI_HandleTypeDef hspi1;

// Display configuration
#define LANDSCAPE                      (0)

// Chip select
#define	CS_PORT	                        GPIOA
#define CS_PIN		                    GPIO_PIN_6

// Reset pin
#define RST_PORT                        GPIOA
#define RST_PIN	                        GPIO_PIN_9

// Command select
#define	RS_PORT                         GPIOB
#define RS_PIN                          GPIO_PIN_1



/* ILI9225 LCD Registers */
#define ILI9225_DRIVER_OUTPUT_CTRL      (0x01u)  // Driver Output Control
#define ILI9225_LCD_AC_DRIVING_CTRL     (0x02u)  // LCD AC Driving Control
#define ILI9225_ENTRY_MODE              (0x03u)  // Entry Mode
#define ILI9225_DISP_CTRL1              (0x07u)  // Display Control 1
#define ILI9225_BLANK_PERIOD_CTRL1      (0x08u)  // Blank Period Control
#define ILI9225_FRAME_CYCLE_CTRL        (0x0Bu)  // Frame Cycle Control
#define ILI9225_INTERFACE_CTRL          (0x0Cu)  // Interface Control
#define ILI9225_OSC_CTRL                (0x0Fu)  // Osc Control
#define ILI9225_POWER_CTRL1             (0x10u)  // Power Control 1
#define ILI9225_POWER_CTRL2             (0x11u)  // Power Control 2
#define ILI9225_POWER_CTRL3             (0x12u)  // Power Control 3
#define ILI9225_POWER_CTRL4             (0x13u)  // Power Control 4
#define ILI9225_POWER_CTRL5             (0x14u)  // Power Control 5
#define ILI9225_VCI_RECYCLING           (0x15u)  // VCI Recycling
#define ILI9225_RAM_ADDR_SET1           (0x20u)  // Horizontal GRAM Address Set
#define ILI9225_RAM_ADDR_SET2           (0x21u)  // Vertical GRAM Address Set
#define ILI9225_GRAM_DATA_REG           (0x22u)  // GRAM Data Register
#define ILI9225_GATE_SCAN_CTRL          (0x30u)  // Gate Scan Control Register
#define ILI9225_VERTICAL_SCROLL_CTRL1   (0x31u)  // Vertical Scroll Control 1 Register
#define ILI9225_VERTICAL_SCROLL_CTRL2   (0x32u)  // Vertical Scroll Control 2 Register
#define ILI9225_VERTICAL_SCROLL_CTRL3   (0x33u)  // Vertical Scroll Control 3 Register
#define ILI9225_PARTIAL_DRIVING_POS1    (0x34u)  // Partial Driving Position 1 Register
#define ILI9225_PARTIAL_DRIVING_POS2    (0x35u)  // Partial Driving Position 2 Register
#define ILI9225_HORIZONTAL_WINDOW_ADDR1 (0x36u)  // Horizontal Address Start Position
#define ILI9225_HORIZONTAL_WINDOW_ADDR2 (0x37u)  // Horizontal Address End Position
#define ILI9225_VERTICAL_WINDOW_ADDR1   (0x38u)  // Vertical Address Start Position
#define ILI9225_VERTICAL_WINDOW_ADDR2   (0x39u)  // Vertical Address End Position
#define ILI9225_GAMMA_CTRL1             (0x50u)  // Gamma Control 1
#define ILI9225_GAMMA_CTRL2             (0x51u)  // Gamma Control 2
#define ILI9225_GAMMA_CTRL3             (0x52u)  // Gamma Control 3
#define ILI9225_GAMMA_CTRL4             (0x53u)  // Gamma Control 4
#define ILI9225_GAMMA_CTRL5             (0x54u)  // Gamma Control 5
#define ILI9225_GAMMA_CTRL6             (0x55u)  // Gamma Control 6
#define ILI9225_GAMMA_CTRL7             (0x56u)  // Gamma Control 7
#define ILI9225_GAMMA_CTRL8             (0x57u)  // Gamma Control 8
#define ILI9225_GAMMA_CTRL9             (0x58u)  // Gamma Control 9
#define ILI9225_GAMMA_CTRL10            (0x59u)  // Gamma Control 10

#define ILI9225C_INVOFF                 (0x20)
#define ILI9225C_INVON                  (0x21)


typedef struct {
    uint8_t width;
    uint8_t data[];
} glyph_t;

// Built-in font binary format.
// Header layout:
//   [0] width         - maximum glyph width in pixels
//   [1] height        - glyph height in pixels
//   [2] first_char    - ASCII code of the first glyph
//   [3] char_count    - number of glyphs stored in the font
//   [4] bytes_per_row - number of bytes used by one bitmap row
//   [5] spacing_x     - blank pixels appended after each glyph
//   [6] glyph_size  - size of one glyph block in bytes
// Glyph layout:
//   [0] glyph_width   - actual glyph width in pixels
//   [1..] bitmap rows - row-major bitmap, MSB-first
// Notes:
//   - glyph blocks are stored sequentially for ASCII codes first_char..(first_char + char_count - 1)
//   - glyph rows are stored bottom-to-top to match lcd_draw_char()
static const uint8_t g_font[] = {
    0x08, 0x0D, 0x20, 0x5F, 0x01, 0x01, 0x0E, // Header
    0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ' ' (0x20)
    0x08, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, // '!' (0x21)
    0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x36, 0x36, 0x36, // '"' (0x22)
    0x08, 0x00, 0x00, 0x00, 0x66, 0x66, 0xff, 0x66, 0x66, 0xff, 0x66, 0x66, 0x00, 0x00, // '#' (0x23)
    0x08, 0x00, 0x00, 0x18, 0x7e, 0xff, 0x1b, 0x1f, 0x7e, 0xf8, 0xd8, 0xff, 0x7e, 0x18, // '$' (0x24)
    0x08, 0x00, 0x00, 0x0e, 0x1b, 0xdb, 0x6e, 0x30, 0x18, 0x0c, 0x76, 0xdb, 0xd8, 0x70, // '%' (0x25)
    0x08, 0x00, 0x00, 0x7f, 0xc6, 0xcf, 0xd8, 0x70, 0x70, 0xd8, 0xcc, 0xcc, 0x6c, 0x38, // '&' (0x26)
    0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x1c, 0x0c, 0x0e, // '\'' (0x27)
    0x08, 0x00, 0x00, 0x0c, 0x18, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x18, 0x0c, // '(' (0x28)
    0x08, 0x00, 0x00, 0x30, 0x18, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x18, 0x30, // ')' (0x29)
    0x08, 0x00, 0x00, 0x00, 0x00, 0x99, 0x5a, 0x3c, 0xff, 0x3c, 0x5a, 0x99, 0x00, 0x00, // '*' (0x2A)
    0x08, 0x00, 0x00, 0x00, 0x18, 0x18, 0x18, 0xff, 0xff, 0x18, 0x18, 0x18, 0x00, 0x00, // '+' (0x2B)
    0x08, 0x00, 0x00, 0x30, 0x18, 0x1c, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ',' (0x2C)
    0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, // '-' (0x2D)
    0x08, 0x00, 0x00, 0x00, 0x38, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // '.' (0x2E)
    0x08, 0x00, 0x60, 0x60, 0x30, 0x30, 0x18, 0x18, 0x0c, 0x0c, 0x06, 0x06, 0x03, 0x03, // '/' (0x2F)
    0x08, 0x00, 0x00, 0x3c, 0x66, 0xc3, 0xe3, 0xf3, 0xdb, 0xcf, 0xc7, 0xc3, 0x66, 0x3c, // '0' (0x30)
    0x08, 0x00, 0x00, 0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x78, 0x38, 0x18, // '1' (0x31)
    0x08, 0x00, 0x00, 0xff, 0xc0, 0xc0, 0x60, 0x30, 0x18, 0x0c, 0x06, 0x03, 0xe7, 0x7e, // '2' (0x32)
    0x08, 0x00, 0x00, 0x7e, 0xe7, 0x03, 0x03, 0x07, 0x7e, 0x07, 0x03, 0x03, 0xe7, 0x7e, // '3' (0x33)
    0x08, 0x00, 0x00, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0xff, 0xcc, 0x6c, 0x3c, 0x1c, 0x0c, // '4' (0x34)
    0x08, 0x00, 0x00, 0x7e, 0xe7, 0x03, 0x03, 0x07, 0xfe, 0xc0, 0xc0, 0xc0, 0xc0, 0xff, // '5' (0x35)
    0x08, 0x00, 0x00, 0x7e, 0xe7, 0xc3, 0xc3, 0xc7, 0xfe, 0xc0, 0xc0, 0xc0, 0xe7, 0x7e, // '6' (0x36)
    0x08, 0x00, 0x00, 0x30, 0x30, 0x30, 0x30, 0x18, 0x0c, 0x06, 0x03, 0x03, 0x03, 0xff, // '7' (0x37)
    0x08, 0x00, 0x00, 0x7e, 0xe7, 0xc3, 0xc3, 0xe7, 0x7e, 0xe7, 0xc3, 0xc3, 0xe7, 0x7e, // '8' (0x38)
    0x08, 0x00, 0x00, 0x7e, 0xe7, 0x03, 0x03, 0x03, 0x7f, 0xe7, 0xc3, 0xc3, 0xe7, 0x7e, // '9' (0x39)
    0x08, 0x00, 0x00, 0x00, 0x38, 0x38, 0x00, 0x00, 0x38, 0x38, 0x00, 0x00, 0x00, 0x00, // ':' (0x3A)
    0x08, 0x00, 0x00, 0x30, 0x18, 0x1c, 0x1c, 0x00, 0x00, 0x1c, 0x1c, 0x00, 0x00, 0x00, // ';' (0x3B)
    0x08, 0x00, 0x00, 0x06, 0x0c, 0x18, 0x30, 0x60, 0xc0, 0x60, 0x30, 0x18, 0x0c, 0x06, // '<' (0x3C)
    0x08, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, // '=' (0x3D)
    0x08, 0x00, 0x00, 0x60, 0x30, 0x18, 0x0c, 0x06, 0x03, 0x06, 0x0c, 0x18, 0x30, 0x60, // '>' (0x3E)
    0x08, 0x00, 0x00, 0x18, 0x00, 0x00, 0x18, 0x18, 0x0c, 0x06, 0x03, 0xc3, 0xc3, 0x7e, // '?' (0x3F)
    0x08, 0x00, 0x00, 0x3f, 0x60, 0xcf, 0xdb, 0xd3, 0xdd, 0xc3, 0x7e, 0x00, 0x00, 0x00, // '@' (0x40)
    0x08, 0x00, 0x00, 0xc3, 0xc3, 0xc3, 0xc3, 0xff, 0xc3, 0xc3, 0xc3, 0x66, 0x3c, 0x18, // 'A' (0x41)
    0x08, 0x00, 0x00, 0xfe, 0xc7, 0xc3, 0xc3, 0xc7, 0xfe, 0xc7, 0xc3, 0xc3, 0xc7, 0xfe, // 'B' (0x42)
    0x08, 0x00, 0x00, 0x7e, 0xe7, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xe7, 0x7e, // 'C' (0x43)
    0x08, 0x00, 0x00, 0xfc, 0xce, 0xc7, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc7, 0xce, 0xfc, // 'D' (0x44)
    0x08, 0x00, 0x00, 0xff, 0xc0, 0xc0, 0xc0, 0xc0, 0xfc, 0xc0, 0xc0, 0xc0, 0xc0, 0xff, // 'E' (0x45)
    0x08, 0x00, 0x00, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xfc, 0xc0, 0xc0, 0xc0, 0xff, // 'F' (0x46)
    0x08, 0x00, 0x00, 0x7e, 0xe7, 0xc3, 0xc3, 0xcf, 0xc0, 0xc0, 0xc0, 0xc0, 0xe7, 0x7e, // 'G' (0x47)
    0x08, 0x00, 0x00, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xff, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, // 'H' (0x48)
    0x08, 0x00, 0x00, 0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7e, // 'I' (0x49)
    0x08, 0x00, 0x00, 0x7c, 0xe6, 0xc6, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x1f, // 'J' (0x4A)
    0x08, 0x00, 0x00, 0xc6, 0xcc, 0xd8, 0xf0, 0xe0, 0xf0, 0xd8, 0xcc, 0xc6, 0xc3, 0xc1, // 'K' (0x4B)
    0x08, 0x00, 0x00, 0xff, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, // 'L' (0x4C)
    0x08, 0x00, 0x00, 0xc3, 0xc3, 0xc3, 0xc3, 0xdb, 0xff, 0xff, 0xe7, 0xc3, 0xc3, 0xc3, // 'M' (0x4D)
    0x08, 0x00, 0x00, 0xc3, 0xc3, 0xc7, 0xcf, 0xdf, 0xfb, 0xf3, 0xe3, 0xc3, 0xc3, 0xc3, // 'N' (0x4E)
    0x08, 0x00, 0x00, 0x3c, 0x66, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0x66, 0x3c, // 'O' (0x4F)
    0x08, 0x00, 0x00, 0xc0, 0xc0, 0xc0, 0xc0, 0xfe, 0xc7, 0xc3, 0xc3, 0xc7, 0xfe, 0xfc, // 'P' (0x50)
    0x08, 0x00, 0x00, 0x3f, 0x66, 0xcf, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0x66, 0x3c, // 'Q' (0x51)
    0x08, 0x00, 0x00, 0xc3, 0xc6, 0xcc, 0xd8, 0xf0, 0xfe, 0xc7, 0xc3, 0xc3, 0xc7, 0xfe, // 'R' (0x52)
    0x08, 0x00, 0x00, 0x7e, 0xe7, 0x03, 0x03, 0x07, 0x7e, 0xe0, 0xc0, 0xc0, 0xe7, 0x7e, // 'S' (0x53)
    0x08, 0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0xff, // 'T' (0x54)
    0x08, 0x00, 0x00, 0x7e, 0xe7, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, // 'U' (0x55)
    0x08, 0x00, 0x00, 0x18, 0x3c, 0x66, 0x66, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, // 'V' (0x56)
    0x08, 0x00, 0x00, 0xc3, 0xe7, 0xff, 0xff, 0xdb, 0xdb, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, // 'W' (0x57)
    0x08, 0x00, 0x00, 0xc3, 0x66, 0x66, 0x3c, 0x3c, 0x18, 0x3c, 0x3c, 0x66, 0x66, 0xc3, // 'X' (0x58)
    0x08, 0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3c, 0x3c, 0x66, 0x66, 0xc3, // 'Y' (0x59)
    0x08, 0x00, 0x00, 0xff, 0xc0, 0xc0, 0x60, 0x30, 0x7e, 0x0c, 0x06, 0x03, 0x03, 0xff, // 'Z' (0x5A)
    0x08, 0x00, 0x00, 0x3c, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3c, // '[' (0x5B)
    0x08, 0x00, 0x03, 0x03, 0x06, 0x06, 0x0c, 0x0c, 0x18, 0x18, 0x30, 0x30, 0x60, 0x60, // '\' (0x5C)
    0x08, 0x00, 0x00, 0x3c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x3c, // ']' (0x5D)
    0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc3, 0x66, 0x3c, 0x18, // '^' (0x5E)
    0x08, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // '_' (0x5F)
    0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x38, 0x30, 0x70, // '`' (0x60)
    0x08, 0x00, 0x00, 0x7f, 0xc3, 0xc3, 0x7f, 0x03, 0xc3, 0x7e, 0x00, 0x00, 0x00, 0x00, // 'a' (0x61)
    0x08, 0x00, 0x00, 0xfe, 0xc3, 0xc3, 0xc3, 0xc3, 0xfe, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, // 'b' (0x62)
    0x08, 0x00, 0x00, 0x7e, 0xc3, 0xc0, 0xc0, 0xc0, 0xc3, 0x7e, 0x00, 0x00, 0x00, 0x00, // 'c' (0x63)
    0x08, 0x00, 0x00, 0x7f, 0xc3, 0xc3, 0xc3, 0xc3, 0x7f, 0x03, 0x03, 0x03, 0x03, 0x03, // 'd' (0x64)
    0x08, 0x00, 0x00, 0x7f, 0xc0, 0xc0, 0xfe, 0xc3, 0xc3, 0x7e, 0x00, 0x00, 0x00, 0x00, // 'e' (0x65)
    0x08, 0x00, 0x00, 0x30, 0x30, 0x30, 0x30, 0x30, 0xfc, 0x30, 0x30, 0x30, 0x33, 0x1e, // 'f' (0x66)
    0x08, 0x7e, 0xc3, 0x03, 0x03, 0x7f, 0xc3, 0xc3, 0xc3, 0x7e, 0x00, 0x00, 0x00, 0x00, // 'g' (0x67)
    0x08, 0x00, 0x00, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xfe, 0xc0, 0xc0, 0xc0, 0xc0, // 'h' (0x68)
    0x08, 0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x18, 0x00, // 'i' (0x69)
    0x08, 0x38, 0x6c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x00, 0x00, 0x0c, 0x00, // 'j' (0x6A)
    0x08, 0x00, 0x00, 0xc6, 0xcc, 0xf8, 0xf0, 0xd8, 0xcc, 0xc6, 0xc0, 0xc0, 0xc0, 0xc0, // 'k' (0x6B)
    0x08, 0x00, 0x00, 0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x78, // 'l' (0x6C)
    0x08, 0x00, 0x00, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xfe, 0x00, 0x00, 0x00, 0x00, // 'm' (0x6D)
    0x08, 0x00, 0x00, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0xfc, 0x00, 0x00, 0x00, 0x00, // 'n' (0x6E)
    0x08, 0x00, 0x00, 0x7c, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0x7c, 0x00, 0x00, 0x00, 0x00, // 'o' (0x6F)
    0x08, 0xc0, 0xc0, 0xc0, 0xfe, 0xc3, 0xc3, 0xc3, 0xc3, 0xfe, 0x00, 0x00, 0x00, 0x00, // 'p' (0x70)
    0x08, 0x03, 0x03, 0x03, 0x7f, 0xc3, 0xc3, 0xc3, 0xc3, 0x7f, 0x00, 0x00, 0x00, 0x00, // 'q' (0x71)
    0x08, 0x00, 0x00, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xe0, 0xfe, 0x00, 0x00, 0x00, 0x00, // 'r' (0x72)
    0x08, 0x00, 0x00, 0xfe, 0x03, 0x03, 0x7e, 0xc0, 0xc0, 0x7f, 0x00, 0x00, 0x00, 0x00, // 's' (0x73)
    0x08, 0x00, 0x00, 0x1c, 0x36, 0x30, 0x30, 0x30, 0x30, 0xfc, 0x30, 0x30, 0x30, 0x00, // 't' (0x74)
    0x08, 0x00, 0x00, 0x7e, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0x00, 0x00, 0x00, 0x00, // 'u' (0x75)
    0x08, 0x00, 0x00, 0x18, 0x3c, 0x3c, 0x66, 0x66, 0xc3, 0xc3, 0x00, 0x00, 0x00, 0x00, // 'v' (0x76)
    0x08, 0x00, 0x00, 0xc3, 0xe7, 0xff, 0xdb, 0xc3, 0xc3, 0xc3, 0x00, 0x00, 0x00, 0x00, // 'w' (0x77)
    0x08, 0x00, 0x00, 0xc3, 0x66, 0x3c, 0x18, 0x3c, 0x66, 0xc3, 0x00, 0x00, 0x00, 0x00, // 'x' (0x78)
    0x08, 0xc0, 0x60, 0x60, 0x30, 0x18, 0x3c, 0x66, 0x66, 0xc3, 0x00, 0x00, 0x00, 0x00, // 'y' (0x79)
    0x08, 0x00, 0x00, 0xff, 0x60, 0x30, 0x18, 0x0c, 0x06, 0xff, 0x00, 0x00, 0x00, 0x00, // 'z' (0x7A)
    0x08, 0x00, 0x00, 0x0f, 0x18, 0x18, 0x18, 0x38, 0xf0, 0x38, 0x18, 0x18, 0x18, 0x0f, // '{' (0x7B)
    0x08, 0x40, 0x60, 0x70, 0x78, 0x7C, 0x7E, 0x7F, 0x7E, 0x7C, 0x78, 0x70, 0x60, 0x40, // '|' (0x7C)
    0x08, 0x00, 0x18, 0x3C, 0x7E, 0xFF, 0x00, 0x00, 0x18, 0x3C, 0x7E, 0xFF, 0x00, 0x00, // '}' (0x7D)
    0x08, 0x00, 0x18, 0x3C, 0x7E, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x7E, 0x3C, 0x18, 0x00  // '~' (0x7E)
};

const ili9225_font_t* const ili9225_font_8x13 = reinterpret_cast<const ili9225_font_t*>(g_font);



static const glyph_t* lcd_get_glyph(const ili9225_font_t* font, uint8_t glyph_index) {
    const uint8_t* font_data = rcast<const uint8_t*>(font);
    return rcast<const glyph_t*>(font_data + sizeof(ili9225_font_t) + (scast<uint16_t>(glyph_index) * font->glyph_size));
}

/// **************************************************************************
/// @brief  Write one byte to SPI without changing chip select state
/// @param  [in] data: byte value to transmit
/// **************************************************************************
static void spi_write(uint8_t data) {
	HAL_SPI_Transmit(&hspi1, &data, 1, 100);
}

/// **************************************************************************
/// @brief  Write one data byte to the display controller
/// @param  [in] data: byte value to transmit
/// **************************************************************************
static void lcd_write_data(uint8_t data) {
    HAL_GPIO_WritePin(RS_PORT, RS_PIN, GPIO_PIN_SET);   // DC HIGH
    spi_write(data);                                    // Send data to the SPI register
}

/// **************************************************************************
/// @brief  Write one command byte to the display controller
/// @param  [in] data: command byte to transmit
/// **************************************************************************
static void lcd_write_command(uint8_t data) {
    HAL_GPIO_WritePin(RS_PORT, RS_PIN, GPIO_PIN_RESET); // Pull the command AND chip select lines LOW
    spi_write(data);                                    // Send data to the SPI register
}

/// **************************************************************************
/// @brief  Write a 16-bit value to a controller register
/// @param  [in] reg: target register address
/// @param  [in] data: 16-bit register value
/// **************************************************************************
static void lcd_write_register(unsigned int reg, unsigned int data) {
    // Write each register byte, and each data byte seperately.
    lcd_write_command(reg >> 8);   // regH
    lcd_write_command(reg & 0xFF); // regL
    lcd_write_data(data >> 8);     // dataH
    lcd_write_data(data & 0xFF);   // dataL
}

/// **************************************************************************
/// @brief  Swap two integer values in place
/// @param  [in,out] num1: pointer to first value
/// @param  [in,out] num2: pointer to second value
/// **************************************************************************
static void _swap(int *num1, int *num2) {
    int temp = *num2;
    *num2 = *num1;
    *num1 = temp;
}



/// **************************************************************************
/// @brief  Set the active LCD drawing window
/// @param  [in] x1: left coordinate
/// @param  [in] y1: top coordinate
/// @param  [in] x2: right coordinate
/// @param  [in] y2: bottom coordinate
/// **************************************************************************
static void lcd_set_draw_window(int x1, int y1, int x2, int y2) {
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);
    lcd_write_register(ILI9225_HORIZONTAL_WINDOW_ADDR1, x2);
    lcd_write_register(ILI9225_HORIZONTAL_WINDOW_ADDR2, x1);
    lcd_write_register(ILI9225_VERTICAL_WINDOW_ADDR1, y2);
    lcd_write_register(ILI9225_VERTICAL_WINDOW_ADDR2, y1);
    lcd_write_register(ILI9225_RAM_ADDR_SET1, x1);
    lcd_write_register(ILI9225_RAM_ADDR_SET2, y1);
    lcd_write_command(0x00);
    lcd_write_command(0x22);
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);
}

/// **************************************************************************
/// @brief  Send the controller initialization command sequence
/// **************************************************************************
static void lcd_init_command_list() {
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);
    lcd_write_register(ILI9225_POWER_CTRL1, 0x0000); // Set SAP,DSTB,STB
    lcd_write_register(ILI9225_POWER_CTRL2, 0x0000); // Set APON,PON,AON,VCI1EN,VC
    lcd_write_register(ILI9225_POWER_CTRL3, 0x0000); // Set BT,DC1,DC2,DC3
    lcd_write_register(ILI9225_POWER_CTRL4, 0x0000); // Set GVDD
    lcd_write_register(ILI9225_POWER_CTRL5, 0x0000); // Set VCOMH/VCOML voltage
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);
    HAL_Delay(10);
    
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);
    lcd_write_register(ILI9225_POWER_CTRL2, 0xFFFF); // EVERYTHING ON
    lcd_write_register(ILI9225_POWER_CTRL3, 0x7000); // Set BT,DC1,DC2,DC3
    lcd_write_register(ILI9225_POWER_CTRL4, 0x006F); // Set GVDD   /*007F 0088 */
    lcd_write_register(ILI9225_POWER_CTRL5, 0x495F); // Set VCOMH/VCOML voltage
    lcd_write_register(ILI9225_POWER_CTRL1, 0x0F00); // Set SAP,DSTB,STB
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);
    HAL_Delay(10);
    
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);
    lcd_write_register(ILI9225_POWER_CTRL2, 0xFFFF); // Set APON,PON,AON,VCI1EN,VC
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);
    HAL_Delay(50);

    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);
    lcd_write_register(ILI9225_DRIVER_OUTPUT_CTRL, 0x011C);  // Set the display line number and display direction
    lcd_write_register(ILI9225_LCD_AC_DRIVING_CTRL, 0x0000); // Set frame inversion
    lcd_write_register(ILI9225_ENTRY_MODE, 0x1030);          // Set GRAM write direction and BGR=1.
    lcd_write_register(ILI9225_DISP_CTRL1, 0x0000);          // Display off
    lcd_write_register(ILI9225_BLANK_PERIOD_CTRL1, 0x0202);  // Set the back porch and front porch (2 lines, minimum)
    lcd_write_register(ILI9225_FRAME_CYCLE_CTRL, 0x0000);    // Set the clocks number per line
    lcd_write_register(ILI9225_INTERFACE_CTRL, 0x0000);      // CPU interface
    lcd_write_register(ILI9225_OSC_CTRL, 0x0F01);            // Set Osc
    lcd_write_register(ILI9225_VCI_RECYCLING, 0x0000);       // Set VCI recycling
    lcd_write_register(ILI9225_RAM_ADDR_SET1, 0x0000);       // RAM Address
    lcd_write_register(ILI9225_RAM_ADDR_SET2, 0x0000);       // RAM Address

    // Set GRAM area
    lcd_write_register(ILI9225_GATE_SCAN_CTRL, 0x0000); 
    lcd_write_register(ILI9225_VERTICAL_SCROLL_CTRL1, 0x00DB); 
    lcd_write_register(ILI9225_VERTICAL_SCROLL_CTRL2, 0x0000); 
    lcd_write_register(ILI9225_VERTICAL_SCROLL_CTRL3, 0x0000); 
    lcd_write_register(ILI9225_PARTIAL_DRIVING_POS1, 0x00DB); 
    lcd_write_register(ILI9225_PARTIAL_DRIVING_POS2, 0x0000); 
    lcd_write_register(ILI9225_HORIZONTAL_WINDOW_ADDR1, 0x00AF); 
    lcd_write_register(ILI9225_HORIZONTAL_WINDOW_ADDR2, 0x0000); 
    lcd_write_register(ILI9225_VERTICAL_WINDOW_ADDR1, 0x00DB); 
    lcd_write_register(ILI9225_VERTICAL_WINDOW_ADDR2, 0x0000); 

    // Set GAMMA curve
    lcd_write_register(ILI9225_GAMMA_CTRL1, 0x0000); 
    lcd_write_register(ILI9225_GAMMA_CTRL2, 0x0808); 
    lcd_write_register(ILI9225_GAMMA_CTRL3, 0x080A); 
    lcd_write_register(ILI9225_GAMMA_CTRL4, 0x000A); 
    lcd_write_register(ILI9225_GAMMA_CTRL5, 0x0A08); 
    lcd_write_register(ILI9225_GAMMA_CTRL6, 0x0808); 
    lcd_write_register(ILI9225_GAMMA_CTRL7, 0x0000); 
    lcd_write_register(ILI9225_GAMMA_CTRL8, 0x0A00); 
    lcd_write_register(ILI9225_GAMMA_CTRL9, 0x0710); 
    lcd_write_register(ILI9225_GAMMA_CTRL10, 0x0710); 

    lcd_write_register(ILI9225_DISP_CTRL1, 0x0012); 
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);
    HAL_Delay(50);
    
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);
    lcd_write_register(ILI9225_DISP_CTRL1, 0x1017);
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);
}

/// **************************************************************************
/// @brief  Draw one character using the specified font
/// @param  [in] x: left coordinate of the character cell
/// @param  [in] y: top coordinate of the character cell
/// @param  [in] c: character to render
/// @param  [in] color: foreground RGB565 color
/// @param  [in] bg_color: background RGB565 color
/// @param  [in] font: font header view with inline glyph data
/// @note   Preserves the background by writing all pixels of the glyph cell
/// **************************************************************************
static void lcd_draw_char(int x, int y, const glyph_t* glyph, const ili9225_font_t* font, uint16_t color, uint16_t bg_color) {
    // Gathering glyph info
    const uint8_t* bitmap      = glyph->data;
    const uint8_t  glyph_width = glyph->width;
    const uint8_t  char_w      = glyph_width + font->spacing_x;
    const uint8_t  char_h      = font->height;

    // Set draw window
    lcd_set_draw_window(x + 1, y, x + char_w, y + char_h - 1);

    // Glyph RAM buffer
    uint8_t  glyph_buffer[char_w * char_h * 2];
    uint32_t buf_idx = 0;

    // Prepare colors
    uint8_t color_h    = color >> 8;
    uint8_t color_l    = color & 0xFF;
    uint8_t bg_color_h = bg_color >> 8;
    uint8_t bg_color_l = bg_color & 0xFF;

    // Get the line of pixels from the font file
    for (uint8_t i = 0; i < char_h; ++i) {
        const uint8_t* row = bitmap + ((char_h - 1 - i) * font->bytes_per_row);
        
        // Draw the pixels to screen
        for (uint8_t x_pos = 0; x_pos < glyph_width; ++x_pos) {
            uint8_t byte_index = x_pos / 8;
            uint8_t bit_index = 7 - (x_pos % 8);
            if (row[byte_index] & (0x01 << bit_index)) { // Char pixel
                glyph_buffer[buf_idx++] = color_h;
                glyph_buffer[buf_idx++] = color_l;
            } else { // Background pixel
                glyph_buffer[buf_idx++] = bg_color_h;
                glyph_buffer[buf_idx++] = bg_color_l;
            }
        }

        // Draw space after char
        for (uint8_t spacing = 0; spacing < font->spacing_x; ++spacing) {
            glyph_buffer[buf_idx++] = bg_color_h;
            glyph_buffer[buf_idx++] = bg_color_l;
        }
    }

    // Send data
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(RS_PORT, RS_PIN, GPIO_PIN_SET);
    HAL_SPI_Transmit(&hspi1, glyph_buffer, sizeof(glyph_buffer), 100);
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);
}

/// **************************************************************************
/// @brief  Initialize the ILI9225 display controller
/// **************************************************************************
void ili9225_init() {
    // SET control pins for the LCD HIGH (they are active LOW)
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);   // Chip select
    HAL_GPIO_WritePin(RS_PORT, RS_PIN, GPIO_PIN_RESET); // Data / command select
    HAL_GPIO_WritePin(RST_PORT, RST_PIN, GPIO_PIN_SET); // RESET pin HIGH
    
    // Cycle reset pin
    HAL_GPIO_WritePin(RST_PORT, RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(500);
    HAL_GPIO_WritePin(RST_PORT, RST_PIN, GPIO_PIN_SET);
    HAL_Delay(500);
    
    lcd_init_command_list();
}

/// **************************************************************************
/// @brief  Clear the full screen to black
/// **************************************************************************
void ili9225_clear() {
    ili9225_fill_rectangle(0, 0, ILI9225_WIDTH - 1, ILI9225_HEIGHT - 1, ILI9225_COLOR_BLACK);
}

/// **************************************************************************
/// @brief  Draw one pixel on the display
/// @param  [in] x: pixel X coordinate
/// @param  [in] y: pixel Y coordinate
/// @param  [in] color: RGB565 pixel color
/// **************************************************************************
void ili9225_draw_pixel(int x, int y, uint16_t color) {
    // If we are in landscape view then translate -90 degrees
    if (LANDSCAPE) {
        _swap(&x, &y);
        y = ILI9225_WIDTH - y;
    }
    
    // Set the x, y position that we want to write to
    lcd_set_draw_window(x, y, x + 1, y + 1);
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);
    lcd_write_data(color >> 8);
    lcd_write_data(color & 0xFF);
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);
}

/// **************************************************************************
/// @brief  Fill a rectangular region with a solid color
/// @param  [in] x1: left coordinate
/// @param  [in] y1: top coordinate
/// @param  [in] x2: right coordinate
/// @param  [in] y2: bottom coordinate
/// @param  [in] color: RGB565 fill color
/// **************************************************************************
void ili9225_fill_rectangle(int x1, int y1, int x2, int y2, uint16_t color) {
    // If landscape view then translate everyting -90 degrees
    if (LANDSCAPE) {
        _swap(&x1, &y1);
        _swap(&x2, &y2);
        y1 = ILI9225_WIDTH - y1;
        y2 = ILI9225_WIDTH - y2;
        _swap(&y2, &y1);
    }
    
    // Set the drawing region
    lcd_set_draw_window(x1, y1, x2, y2);

    const uint8_t CHUNK_SIZE = 64;
    static uint16_t pixel_buffer[CHUNK_SIZE];

    uint16_t prepared_color = __REV16(color); // Swap color bytes
    for (int i = 0; i < CHUNK_SIZE; ++i) {
        pixel_buffer[i] = prepared_color;
    }
    
    // Write color by chunks
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(RS_PORT, RS_PIN, GPIO_PIN_SET); 
    uint32_t total_pixels = (x2 - x1 + 1) * (y2 - y1 + 1);
    while (total_pixels > 0) {
        uint16_t to_send = (total_pixels > CHUNK_SIZE) ? CHUNK_SIZE : total_pixels;
        HAL_SPI_Transmit(&hspi1, (uint8_t*)pixel_buffer, to_send * 2, 100);
        total_pixels -= to_send;
    }
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);
}

/// **************************************************************************
/// @brief  Draw a string with the built-in font
/// @param  [in] x: left coordinate of the first character
/// @param  [in] y: top coordinate of the first character
/// @param  [in] color: foreground RGB565 color
/// @param  [in] bg_color: background RGB565 color
/// @param  [in] str: null-terminated string to render
/// @param  [in] min_len: minimum number of character cells to draw
/// **************************************************************************
void ili9225_draw_string(int x, int y, uint16_t color, uint16_t bg_color, const char* str, uint8_t min_len) {
    ili9225_draw_string_with_font(x, y, color, bg_color, str, ili9225_font_8x13, min_len);
}

/// **************************************************************************
/// @brief  Draw a string with the specified font
/// @param  [in] x: left coordinate of the first character
/// @param  [in] y: top coordinate of the first character
/// @param  [in] color: foreground RGB565 color
/// @param  [in] bg_color: background RGB565 color
/// @param  [in] str: null-terminated string to render
/// @param  [in] font: font header view with inline glyph data
/// @param  [in] min_len: minimum number of character cells to draw
/// **************************************************************************
void ili9225_draw_string_with_font(int x, int y, uint16_t color, uint16_t bg_color, const char* str, const ili9225_font_t* font, uint8_t min_len) {
    if (!str || !font) {
        return;
    }

    int current_x = x;
    uint8_t str_size = strlen(str);
    
    // Draw glyphs
    for (int i = 0; i < str_size; i++) {
        uint8_t char_code = static_cast<uint8_t>(str[i]);
        if (char_code < font->first_char || char_code >= (font->first_char + font->char_count)) {
            return;
        }

        const glyph_t* glyph = lcd_get_glyph(font, char_code - font->first_char);
        lcd_draw_char(current_x, y, glyph, font, color, bg_color);
        current_x += glyph->width + font->spacing_x; 
    }

    // Fill tail (spaces)
    for (int i = str_size; i < min_len; i++) {
        uint8_t char_code = ' ';
        if (char_code < font->first_char || char_code >= (font->first_char + font->char_count)) {
            return;
        }

        const glyph_t* glyph = lcd_get_glyph(font, char_code - font->first_char);
        lcd_draw_char(current_x, y, glyph, font, color, bg_color);
        current_x += glyph->width + font->spacing_x; 
    }
}

/// **************************************************************************
/// @brief  Draw a horizontal line as a filled rectangle
/// @param  [in] x1: left coordinate
/// @param  [in] y1: top coordinate
/// @param  [in] w: line width in pixels
/// @param  [in] color: RGB565 line color
/// **************************************************************************
void ili9225_draw_line(int x1, int y1, int w, uint16_t color) {
    ili9225_fill_rectangle(x1, y1, x1 + w, y1 + 1, color);
}

/// **************************************************************************
/// @brief  Draw a bitmap with integer scaling
/// @param  [in] x1: left coordinate
/// @param  [in] y1: top coordinate
/// @param  [in] scale: integer scale factor
/// @param  [in] bmp: bitmap array with width and height in the first words
/// **************************************************************************
void ili9225_draw_bitmap(int x1, int y1, int scale, const unsigned int* bmp) {
	int width = bmp[0];
	int height = bmp[1];
	unsigned int this_byte;
	int x2 = x1 + (width * scale);
	int y2 = y1 + (height * scale);

	// If landscape view then translate everyting -90 degrees
	if (LANDSCAPE) {
		_swap(&x1, &y1);
		_swap(&x2, &y2);
		y1 = ILI9225_WIDTH - y1;
		y2 = ILI9225_WIDTH - y2;
		_swap(&y2, &y1);
		_swap(&width, &height);
	}

	// Set the drawing region
	lcd_set_draw_window(x1, y1, x2 + scale - 1, y2);

	// Write color to each pixel
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);
	for (int i = 0; i < height; ++i) {
		// Yhis loop does the vertical axis scaling (two of each line)
		for (int sv = 0; sv < scale; ++sv) {
			for (int j = 0; j <= width; ++j) {
				// Choose which byte to display depending on the screen orientation
				// NOTE: We add a byte because of the first two bytes being dimension data in the array
				if (LANDSCAPE) {
					this_byte = bmp[(height * (j + 1)) - i + 1];
                } else {
					this_byte = bmp[(width * i) + j + 1];
                }

				// And this loop does the horizontal axis scale (two of each pixels on the line))
				for (int sh = 0; sh < scale; ++sh) {
					lcd_write_data(this_byte >> 8);
					lcd_write_data(this_byte & 0xFF);
				}
			}
		}
	}
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);
}

