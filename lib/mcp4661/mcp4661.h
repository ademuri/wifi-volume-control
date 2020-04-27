#ifndef MCP4661_H
#define MCP4661_H

#include <Wire.h>

class Mcp4661 {
  public:
    // I2C constants
    static const uint8_t kVolatileWiper0 = 0;
    static const uint8_t kVolatileWiper1 = 1;
    static const uint8_t kNonVolatileWiper0 = 2;
    static const uint8_t kNonVolatileWiper1 = 3;

    Mcp4661(int sda, int scl);

    void begin();
    
    uint16_t read_register(uint8_t register_address);
    void write_register(uint8_t register_address, uint16_t value);

  private:
    // Default address: last 3 bits set by the A0-A2 pins, default high
    static const uint8_t kDefaultAddress = 0b0101111;
    const int sda_;
    const int scl_;

    TwoWire wire_;
};

#endif // MCP4661_H
