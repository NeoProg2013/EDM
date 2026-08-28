#include "HX711.h"

void hx711_t::begin(GPIO_TypeDef* dout_port, uint16_t dout_pin, GPIO_TypeDef* sck_port, uint16_t sck_pin, uint8_t gain) {
	m_dout_port = dout_port;
    m_dout_pin  = dout_pin;
    m_sck_port  = sck_port;
    m_sck_pin   = sck_pin;

	GPIO_InitTypeDef dout = {0};
	dout.Pin  = m_dout_pin;
	dout.Mode = GPIO_MODE_INPUT;
	dout.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(m_dout_port, &dout);

	GPIO_InitTypeDef sck = {0};
	sck.Pin   = m_sck_pin;
	sck.Mode  = GPIO_MODE_OUTPUT_PP;
	sck.Pull  = GPIO_NOPULL;
	sck.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(m_sck_port, &sck);
	HAL_GPIO_WritePin(m_sck_port, m_sck_pin, GPIO_PIN_RESET);

	set_gain(gain);
}

bool hx711_t::is_ready() {
	return HAL_GPIO_ReadPin(m_dout_port, m_dout_pin) == GPIO_PIN_RESET;
}

void hx711_t::set_gain(uint8_t gain) {
	switch (gain) {
	case 128: // channel A, gain factor 128
		m_gain = 1;
		break;
	case 64: // channel A, gain factor 64
		m_gain = 3;
		break;
	case 32: // channel B, gain factor 32
		m_gain = 2;
		break;
	}
}

int32_t hx711_t::read() {
	// Protect the read sequence from system interrupts.  If an interrupt occurs during
	// the time the PD_SCK signal is high it will stretch the length of the clock pulse.
	// If the total pulse time exceeds 60 uSec this will cause the HX711 to enter
	// power down mode during the middle of the read sequence.  While the device will
	// wake up when PD_SCK goes low again, the reset starts a new conversion cycle which
	// forces DOUT high until that cycle is completed.
	//
	// The result is that all subsequent bits read will read back as 1,
	// corrupting the value returned by read()
	__disable_irq();

	uint32_t value = 0;
	for (uint8_t i = 0; i < 24; i++) {
		// Min pulse width 0.2 us. 40-50 NOP = ~0.25-0.3 us on 168 Mhz
        HAL_GPIO_WritePin(m_sck_port, m_sck_pin, GPIO_PIN_SET);
        for (volatile uint32_t n = 0; n < 45; n++) {
			__NOP();
		}

		// Read data
        value = value << 1;
        if (HAL_GPIO_ReadPin(m_dout_port, m_dout_pin) == GPIO_PIN_SET) {
            value |= 0x01;
        }

		// Min pulse width 0.2 us. 40-50 NOP = ~0.25-0.3 us on 168 Mhz
        HAL_GPIO_WritePin(m_sck_port, m_sck_pin, GPIO_PIN_RESET);
        for (volatile uint32_t n = 0; n < 45; n++) {
			__NOP();
		}
    }

	// Set the channel and the gain factor for the next reading using the clock pin.
	for (uint8_t i = 0; i < m_gain; i++) {
        HAL_GPIO_WritePin(m_sck_port, m_sck_pin, GPIO_PIN_SET);
        for (volatile uint32_t n = 0; n < 45; n++) {
			__NOP();
		}

        HAL_GPIO_WritePin(m_sck_port, m_sck_pin, GPIO_PIN_RESET);
        for (volatile uint32_t n = 0; n < 45; n++) {
			__NOP();
		}
    }

	__enable_irq();

	// Replicate the most significant bit to pad out a 32-bit signed integer
	if (value & 0x800000) {
        value |= 0xFF000000;
    }

	return static_cast<int32_t>(value) - m_offset;
}

void hx711_t::set_offset(int32_t offset) {
	m_offset = offset;
}