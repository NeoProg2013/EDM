#include "core.h"
#include "i2c1.h"

static I2C_HandleTypeDef hi2c1 = {};


void i2c1_init() {
    // Init I2C
    hi2c1.Instance             = I2C1;
    hi2c1.Init.Timing          = 0x00901A51; // Fast-mode (400 kHz)
    hi2c1.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK)   {
        while (1);
    }

    // Enable analog filter for SDA / SCL
    if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK) {
        while (1);
    }

    // Setup SDA / SCL
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin       = GPIO_PIN_9 | GPIO_PIN_10;
    gpio.Mode      = GPIO_MODE_AF_OD;
    gpio.Pull      = GPIO_NOPULL;
    gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
    gpio.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOA, &gpio);
}

void i2c1_write8(uint8_t addr, uint8_t reg, uint8_t v) {
    HAL_I2C_Mem_Write(&hi2c1, addr, reg, I2C_MEMADD_SIZE_8BIT, &v, 1, 10);
}

uint8_t i2c1_read8(uint8_t addr, uint8_t reg) {
    uint8_t v = 0;
    HAL_I2C_Mem_Read(&hi2c1, addr, reg, I2C_MEMADD_SIZE_8BIT, &v, 1, 10);
    return v;
}
