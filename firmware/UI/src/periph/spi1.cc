#include "core.h"
#include "spi1.h"

static SPI_HandleTypeDef spi1 = {};

void spi1_init() {
    // PA5 -> SPI1_SCK
    // PA7 -> SPI1_MOSI
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin       = GPIO_PIN_5 | GPIO_PIN_7;
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_NOPULL;
    gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

    spi1.Instance               = SPI1;
    spi1.Init.Mode              = SPI_MODE_MASTER;         // Master mode
    spi1.Init.Direction         = SPI_DIRECTION_2LINES;
    spi1.Init.DataSize          = SPI_DATASIZE_8BIT;       // 8 bit
    spi1.Init.CLKPolarity       = SPI_POLARITY_LOW;        // CPOL = 0
    spi1.Init.CLKPhase          = SPI_PHASE_1EDGE;         // CPHA = 0
    spi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4; // 72 MHz / 4 = 18 MHz
    spi1.Init.FirstBit          = SPI_FIRSTBIT_MSB;        // MSB first
    spi1.Init.NSS               = SPI_NSS_SOFT;
    if (HAL_SPI_Init(&spi1) != HAL_OK) {
        while(1);
    }

    __HAL_SPI_ENABLE(&spi1);
}

void spi1_write(uint8_t* data, uint16_t size) {
    HAL_SPI_Transmit(&spi1, data, size, 100);
}