#include "Sample.h"

namespace sensirion::upt::ble_server {

void Sample::writeValue(uint16_t value, size_t position) {
  write16BitLittleEndian(value, position);
}

void Sample::setByte(uint8_t byte, size_t position) { mData[position] = byte; }

} // namespace sensirion::upt::ble_server