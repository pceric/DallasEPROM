// Maxim/Dallas 1-Wire EPROM & EEPROM library for Arduino
// Copyright (C) 2011-2017 Eric Hokanson
// https://github.com/pceric

// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

/** @mainpage Quick Start Guide
 *
 * @section req_sec Requirements
 * Arduino v1.0.0+ and OneWire Library v2.2
 *
 * @section install_sec Installation
 * Extract the DallasEPROM directory into the arduino/libraries directory.
 *
 * @section usage_sec Usage
 * Click <a href="../examples/simple/simple.pde">here</a> to see a
 * simple example of how to use this library.
 *
 * You can also find this example by selecting File->Examples->DallasEPROM
 * from the Arduino software menu.
 */

#ifndef DallasEPROM_h
#define DallasEPROM_h

#define DALLASEPROMVERSION "1.3.0"

#include <inttypes.h>
#include <OneWire.h>

// OneWire commands
#define READSTATUS      0xAA  // Read the status fields [EPROM] or the Scratchpad [EEPROM]
#define VERIFYRESUME    0xA5  // Either verifies or resumes depending on EEPROM
#define WRITESTATUS     0x55  // Write to the status fields [EPROM] or commit Scratchpad [EEPROM]
#define READMEMORY      0xF0  // Read memory
#define READMEMORYCRC   0xC3  // Read memory w CRC
#define WRITEMEMORY     0x0F  // Write to EPROM or the Scratchpad
/**
 * @defgroup ERROR_GROUP Returned Error Codes
 *
 * @{
 */
#define CRC_MISMATCH -1 //!< CRC mismatch
#define INVALID_PAGE -2 //!< Requested page is invalid
#define PAGE_LOCKED -3 //!< Page is currently locked
#define BAD_INTEGRITY -4 //!< Failed scratchpad integrity check
#define COPY_FAILURE -5 //!< Copy scratchpad to memory has failed
#define UNSUPPORTED_DEVICE -64 //!< Chip is unsupported
#define DEVICE_DISCONNECTED -127 //!< Device has disconnected
/** @} */

/**
 * Stores our supported chip types.
 *
 * @param id 1 byte chip id.
 * @param name Name/model number of chip.
 * @param pages Total number of 32 byte pages supported by chip.
 * @param isEPROM Is this device an EPROM and not an EEPROM.
 */
typedef struct {
	const uint8_t id;
	const char* name;
	const int pages;
	const bool isEPROM;
} model_type;

/**
 * A class that reads and writes to Dallas/Maxim EPROM and EEPROM devices.
 *
 * @author Eric Hokanson
 */
class DallasEPROM {
public:
	/**
	 * Creates a new DallasEPROM instance using the first EPROM/EEPROM
	 * device found on the bus.
	 *
	 * @param rWire Reference to a OneWire v2.2 instance.
	 */
	DallasEPROM(OneWire* rWire);

	/**
	 * Creates a new DallasEPROM instance using the first EPROM/EEPROM
	 * device found on the bus.  In addition it will trigger a 500us
	 * pulse on the provided Arduino pin for EPROM programming.
	 *
	 * @param rWire Reference to a OneWire v2.2 instance.
	 * @param progPin Arduino pin number to pulse if writing EPROMs
	 */
	DallasEPROM(OneWire* rWire, int progPin);

	/**
	 * Static helper function to check if an address has a valid checksum.
	 *
	 * @param pAddress Pointer to an 8 byte 1-Wire address.
	 * @return True if the address has a valid checksum.
	 */
	static bool validAddress(uint8_t* pAddress);

	/**
	 * Static helper function to check if the supplied address is from
	 * a chip that the library supports.
	 *
	 * @param pAddress Pointer to an 8 byte 1-Wire address.
	 * @return True if the chip is supported.
	 */
	static bool isSupported(uint8_t* pAddress);

	/**
	 * Finds the first supported device on the bus and returns true on success
	 */
	bool search();

	/**
	 * Gets the device address of the current instance.
	 *
	 * @return Pointer to the currently configured address.
	 */
	uint8_t* getAddress();

	/**
	 * Sets the address of the current instance.
	 *
	 * @param pAddress Pointer to an 8 byte 1-Wire address.
	 */
	void setAddress(uint8_t* pAddress);

	/**
	 * Gets the device name based on the current address.
	 *
	 * @return Pointer to the current device string.
	 */
	const char* getDeviceName();

	/**
	 * Scans the bus and checks if the device is still connected.
	 *
	 * @return True if the device is still connected.
	 */
	bool isConnected();

	/**
	 * Reads a page from the device's memory.
	 *
	 * @param pData Pointer to a 32 byte buffer to store the data.
	 * @param page Page number to read (0-indexed).
	 * @return 0 on success or @ref ERROR_GROUP.
	 */
	int readPage(uint8_t* pData, int page);

	/**
	 * Writes a page to the device's memory.
	 *
	 * @param pData Pointer to a 32 byte buffer containing the data to store.
	 * @param page Page number to write (0-indexed).
	 * @return 0 on success or @ref ERROR_GROUP.
	 */
	int writePage(uint8_t* pData, int page);

	/**
	 * Lock a page and prevent further writes.
	 *
	 * @param page Page to lock (0-indexed).
	 * @return 0 on success or @ref ERROR_GROUP.
	 */
	int lockPage(int page);

	/**
	 * Checks to see if a page is locked.
	 *
	 * @param page Page to lock (0-indexed).
	 * @return True if locked.
	 */
	bool isPageLocked(int page);

private:
	OneWire* _wire;  // Pointer to OneWire v2.2 instance

	uint8_t _addr[8];  // 1-Wire address of memory device stored LSB first

	int _progPin;  // Arduino pin number to pulse when programming EPROMs

	char _curModelIndex;  // Currently selected device from device table

	/**
	 *  EEPROMs must use a scratch space to write data
	 */
	int scratchWrite(uint8_t* pdata, int length, unsigned int address);

	/**
	 * Checks to see if the provided page is valid.
	 */
	bool isPageValid(int page);

	/**
	 * Returns true if the current device is an EPROM and not an EEPROM.
	 */
	bool isEPROMDevice();
};
#endif

/** @file */
