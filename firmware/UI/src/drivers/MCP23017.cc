#include "core.h"
#include "i2c1.h"
#include "MCP23017.h"

#define MCP23017_ADDRESS  (0x20 << 1)
#define MCP_IODIRA        (0x00)
#define MCP_IODIRB        (0x01)
#define MCP_GPPUA         (0x0C)
#define MCP_GPPUB         (0x0D)
#define MCP_GPIOA         (0x12)
#define MCP_GPIOB         (0x13)


void mcp23017_init() {
    i2c1_init();

    // Setup GPIOА as input with pullup
    i2c1_write8(MCP23017_ADDRESS, MCP_IODIRA, 0xFF);
    i2c1_write8(MCP23017_ADDRESS, MCP_GPPUA, 0xFF);
}

uint8_t mcp23017_read_porta() {
    return i2c1_read8(MCP23017_ADDRESS, MCP_GPIOA);
}
