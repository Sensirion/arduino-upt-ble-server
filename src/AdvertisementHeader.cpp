#include "AdvertisementHeader.h"

void AdvertisementHeader::writeCompanyId(const uint16_t companyID) {
  write16BitLittleEndian(companyID, 0);
}

void AdvertisementHeader::writeSensirionAdvertisementType(
    const uint8_t advType) {
  writeByte(advType, 2);
}

void AdvertisementHeader::writeSampleType(const uint8_t sampleType) {
  writeByte(sampleType, 3);
}

void AdvertisementHeader::writeDeviceId(const uint8_t deviceIDHigh,
                                        const uint8_t deviceIDLow) {
  writeByte(deviceIDHigh, 4);
  writeByte(deviceIDLow, 5);
}
