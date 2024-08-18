#ifndef FLASH_H
#define FLASH_H

#include <Arduino.h>

void logMessage(const char* message);
void logAddress(uint32_t address, uint8_t byte);
void flashFirmwareToInternalMemory();
void writemarker(const char *data, uint32_t startAddress, uint32_t endAddress);
void readmarker(uint32_t startAddress, uint32_t endAddress);
void startApplication(uint32_t APPLICATION_ADDRESS);

#endif