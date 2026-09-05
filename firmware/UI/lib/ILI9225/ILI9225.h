#ifndef ILI9225_H
#define	ILI9225_H
#include <stdint.h>

#ifdef	__cplusplus
extern "C" {
#endif

#define ILI9225_WIDTH                   (176)
#define ILI9225_HEIGHT                  (220)

// RGB 16-bit color table definition
#define ILI9225_COLOR_BLACK             (0x0000) //   0,   0,   0
#define ILI9225_COLOR_WHITE             (0xFFFF) // 255, 255, 255
#define ILI9225_COLOR_BLUE              (0x001F) //   0,   0, 255
#define ILI9225_COLOR_GREEN             (0x07E0) //   0, 255,   0
#define ILI9225_COLOR_RED               (0xF800) // 255,   0,   0
#define ILI9225_COLOR_NAVY              (0x000F) //   0,   0, 128
#define ILI9225_COLOR_DARKBLUE          (0x0011) //   0,   0, 139
#define ILI9225_COLOR_DARKGREEN         (0x03E0) //   0, 128,   0
#define ILI9225_COLOR_DARKCYAN          (0x03EF) //   0, 128, 128
#define ILI9225_COLOR_CYAN              (0x07FF) //   0, 255, 255
#define ILI9225_COLOR_TURQUOISE         (0x471A) //  64, 224, 208
#define ILI9225_COLOR_INDIGO            (0x4810) //  75,   0, 130
#define ILI9225_COLOR_DARKRED           (0x8000) // 128,   0,   0
#define ILI9225_COLOR_OLIVE             (0x7BE0) // 128, 128,   0
#define ILI9225_COLOR_GRAY              (0x8410) // 128, 128, 128
#define ILI9225_COLOR_GREY              (0x8410) // 128, 128, 128
#define ILI9225_COLOR_SKYBLUE           (0x867D) // 135, 206, 235
#define ILI9225_COLOR_BLUEVIOLET        (0x895C) // 138,  43, 226
#define ILI9225_COLOR_LIGHTGREEN        (0x9772) // 144, 238, 144
#define ILI9225_COLOR_DARKVIOLET        (0x901A) // 148,   0, 211
#define ILI9225_COLOR_YELLOWGREEN       (0x9E66) // 154, 205,  50
#define ILI9225_COLOR_BROWN             (0xA145) // 165,  42,  42
#define ILI9225_COLOR_DARKGRAY          (0x7BEF) // 128, 128, 128
#define ILI9225_COLOR_DARKGREY          (0x7BEF) // 128, 128, 128
#define ILI9225_COLOR_SIENNA            (0xA285) // 160,  82,  45
#define ILI9225_COLOR_LIGHTBLUE         (0xAEDC) // 172, 216, 230
#define ILI9225_COLOR_GREENYELLOW       (0xAFE5) // 173, 255,  47
#define ILI9225_COLOR_SILVER            (0xC618) // 192, 192, 192
#define ILI9225_COLOR_LIGHTGRAY         (0xC618) // 192, 192, 192
#define ILI9225_COLOR_LIGHTGREY         (0xC618) // 192, 192, 192
#define ILI9225_COLOR_LIGHTCYAN         (0xE7FF) // 224, 255, 255
#define ILI9225_COLOR_VIOLET            (0xEC1D) // 238, 130, 238
#define ILI9225_COLOR_AZUR              (0xF7FF) // 240, 255, 255
#define ILI9225_COLOR_BEIGE             (0xF7BB) // 245, 245, 220
#define ILI9225_COLOR_MAGENTA           (0xF81F) // 255,   0, 255
#define ILI9225_COLOR_TOMATO            (0xFB08) // 255,  99,  71
#define ILI9225_COLOR_GOLD              (0xFEA0) // 255, 215,   0
#define ILI9225_COLOR_ORANGE            (0xFD20) // 255, 165,   0
#define ILI9225_COLOR_SNOW              (0xFFDF) // 255, 250, 250
#define ILI9225_COLOR_YELLOW            (0xFFE0) // 255, 255,   0

typedef struct __attribute__((packed)) {
    uint8_t width;
    uint8_t height;
    uint8_t first_char;
    uint8_t char_count;
    uint8_t bytes_per_row;
    uint8_t spacing_x;
    uint8_t glyph_size;
} ili9225_font_t;

extern const ili9225_font_t* const ili9225_font_8x13;
extern const ili9225_font_t* const ili9225_font_terminal6x8;

/// **************************************************************************
/// @brief  Initialize the ILI9225 display controller
/// **************************************************************************
void ili9225_init(void);

/// **************************************************************************
/// @brief  Clear the full screen to black
/// **************************************************************************
void ili9225_clear(void);

/// **************************************************************************
/// @brief  Draw one pixel on the display
/// @param  [in] x: pixel X coordinate
/// @param  [in] y: pixel Y coordinate
/// @param  [in] color: RGB565 pixel color
/// **************************************************************************
void ili9225_draw_pixel(int x, int y, uint16_t color);

/// **************************************************************************
/// @brief  Fill a rectangular region with a solid color
/// @param  [in] x1: left coordinate
/// @param  [in] y1: top coordinate
/// @param  [in] x2: right coordinate
/// @param  [in] y2: bottom coordinate
/// @param  [in] color: RGB565 fill color
/// **************************************************************************
void ili9225_fill_rectangle(int x1, int y1, int x2, int y2, uint16_t color);

/// **************************************************************************
/// @brief  Draw a string with the specified font
/// @param  [in] x: left coordinate of the first character
/// @param  [in] y: top coordinate of the first character
/// @param  [in] str: null-terminated string to render
/// @param  [in] font: font header view with inline glyph data
/// @param  [in] min_len: minimum number of character cells to draw
/// **************************************************************************
void ili9225_draw_string(int x, int y, uint16_t color, const char* str, uint8_t min_len = 0);

/// **************************************************************************
/// @brief  Set current font for all strings
/// @param  [in] font: font header view with inline glyph data
/// **************************************************************************
void ili9225_set_font(const ili9225_font_t* font);

/// **************************************************************************
/// @brief  Set backgound color for all strings
/// @param  [in] bg_color: background RGB565 color
/// **************************************************************************
void ili9225_set_bg_color(uint16_t bg_color);

/// **************************************************************************
/// @brief  Draw a horizontal line as a filled rectangle
/// @param  [in] x1: left coordinate
/// @param  [in] y1: top coordinate
/// @param  [in] w: line width in pixels
/// @param  [in] color: RGB565 line color
/// **************************************************************************
void ili9225_draw_line(int x1, int y1, int w, uint16_t color);

/// **************************************************************************
/// @brief  Draw a bitmap with integer scaling
/// @param  [in] x: left coordinate
/// @param  [in] y: top coordinate
/// @param  [in] scale: integer scale factor
/// @param  [in] bmp: bitmap array with width and height in the first words
/// **************************************************************************
void ili9225_draw_bitmap(int x, int y, int scale, const unsigned int* bmp);

#ifdef	__cplusplus
}
#endif

#endif	/* ILI9225_H */

