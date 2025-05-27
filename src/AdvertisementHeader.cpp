#include "AdvertisementHeader.h"

void AdvertisementHeader::writeCompanyId(uint16_t companyID) {
  write16BitLittleEndian(companyID, 0);
}

void AdvertisementHeader::writeSensirionAdvertisementType(uint8_t advType) {
  writeByte(advType, 2);
}

void AdvertisementHeader::writeSampleType(uint8_t sampleType) {
  writeByte(sampleType, 3);
}

void AdvertisementHeader::writeDeviceId(uint8_t deviceIDHigh,
                                        uint8_t deviceIDLow) {
  writeByte(deviceIDHigh, 4);
  writeByte(deviceIDLow, 5);
}
