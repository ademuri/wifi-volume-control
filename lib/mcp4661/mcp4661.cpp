#include <mcp4661.h>

Mcp4661::Mcp4661(int sda, int scl) : sda_(sda), scl_(scl){
}

void Mcp4661::begin() {
  wire_.begin(sda_, scl_);
}

uint16_t Mcp4661::read_register(uint8_t register_address) {
  const uint8_t command = 0b1100 | (register_address << 4);

  wire_.beginTransmission(kDefaultAddress);
  wire_.write(command);
  wire_.endTransmission();

  uint16_t msb;
  uint16_t lsb;
  wire_.requestFrom(kDefaultAddress, (uint8_t) 2);
  msb = wire_.read();
  lsb = wire_.read();
  wire_.endTransmission();

  return (msb << 8) | lsb;
}

void Mcp4661::write_register(uint8_t register_address, uint16_t value) {
  // Last 2 bits are the 2 MSB of value
  const uint8_t command = (register_address << 4) | (value & 0x200 >> 10);
  const uint8_t data = value & 0xFF;
  
  wire_.beginTransmission(kDefaultAddress);
  wire_.write(command);
  wire_.write(data);
  wire_.endTransmission();
}
