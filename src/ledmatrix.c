/*
 * ledmatrix.c
 *
 * Author: Peter Sutton
 * 
 * See the LED matrix Reference for details of the SPI commands used.
 */ 

#include <avr/io.h>
#include "ledmatrix.h"
#include "spi.h"


#define CMD_UPDATE_COL         0x03
#define CMD_SHIFT_DISPLAY      0x04
#define CMD_CLEAR_SCREEN       0x0F
#define SHIFT_DIR_LEFT         0x02


void ledmatrix_update_column(uint8_t x,  uint8_t pixels[MATRIX_NUM_ROWS])
{
    if(x >= MATRIX_NUM_COLUMNS)
    {
        // x value is too large - we ignore the request
        return;
    }
    (void)spi_send_byte(CMD_UPDATE_COL);
    (void)spi_send_byte(x & 0x0F); // column number
    for (uint8_t y = 0; y < MATRIX_NUM_ROWS; y++)
    {
        (void)spi_send_byte(pixels[y]);
    }
}

void ledmatrix_clear(void)
{
    (void)spi_send_byte(CMD_CLEAR_SCREEN);
}

void ledmatrix_shift_left(void)
{
    // Shift the current display one column left using the matrix command.
    (void)spi_send_byte(CMD_SHIFT_DISPLAY);
    (void)spi_send_byte(SHIFT_DIR_LEFT);
}
