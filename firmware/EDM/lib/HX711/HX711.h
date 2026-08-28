#ifndef _HX711_H_
#define _HX711_H_
#include "stm32f4xx_hal.h"

class hx711_t {
protected:
    // Serial Data Output Pin
    GPIO_TypeDef* m_dout_port   { };
    uint16_t m_dout_pin         { };

    // Power Down and Serial Clock Input Pin
    GPIO_TypeDef* m_sck_port    { };
    uint16_t m_sck_pin          { };

    uint8_t m_gain              { }; // Amplification factor
    int32_t m_offset            { }; // Used for tare weight

public:
    // Initialize library with data output pin, clock input pin and gain factor.
    // Channel selection is made by passing the appropriate gain:
    // - With a gain factor of 64 or 128, channel A is selected
    // - With a gain factor of 32, channel B is selected
    // The library default is "128" (Channel A).
    void begin(GPIO_TypeDef* dout_port, uint16_t dout_pin, GPIO_TypeDef* sck_port, uint16_t sck_pin, uint8_t gain = 128);

    // Check if HX711 is ready
    // from the datasheet: When output data is not ready for retrieval, digital output pin DOUT is high. Serial clock
    // input PD_SCK should be low. When DOUT goes to low, it indicates data is ready for retrieval.
    bool is_ready();
    
    // set the gain factor; takes effect only after a call to read()
    // channel A can be set for a 128 or 64 gain; channel B has a fixed 32 gain
    // depending on the parameter, the channel is also set to either A or B
    void set_gain(uint8_t gain);

    // waits for the chip to be ready and returns a reading
    int32_t read();

    // set OFFSET, the value that's subtracted from the actual reading (tare weight)
    void set_offset(int32_t offset);
};

#endif // _HX711_H_
