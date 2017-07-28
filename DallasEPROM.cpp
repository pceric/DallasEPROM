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

#include "DallasEPROM.h"

/** Supported chips. */
model_type _chip_model_list[] = {
	// EPROMs
	{ 0x09, "DS2502", 4, true },
	{ 0x0B, "DS2505", 64, true },
	// EEPROMs
	{ 0x14, "DS2430", 1, false },
	{ 0x2D, "DS2431", 4, false },
	{ 0x23,	"DS2433", 16, false },
	{ 0, 0, 0, 0 }
};

DallasEPROM::DallasEPROM(OneWire* rWire) {
	_wire = rWire;
	_progPin = -1;
}

DallasEPROM::DallasEPROM(OneWire* rWire, int progPin) {
	_wire = rWire;
	_progPin = progPin;
	pinMode(progPin, OUTPUT);
	digitalWrite(progPin, LOW);
}

/*******************
 * Static methods
 *******************/

bool DallasEPROM::validAddress(uint8_t* deviceAddress) {
	return (OneWire::crc8(deviceAddress, 7) == deviceAddress[7]);
}

bool DallasEPROM::isSupported(uint8_t* deviceAddress) {
	int i = 0;
	while (_chip_model_list[i].id) {
		if (deviceAddress[0] == _chip_model_list[i].id)
			return true;
		++i;
	}
	return false;
}

/*******************
 * Public methods
 *******************/

bool DallasEPROM::search() {
	int i;
	_curModelIndex = -1;
	if (!_wire->reset())
		return false;
	_wire->reset_search();
	while (_wire->search(_addr)) {
		i = 0;
		while (_chip_model_list[i].id) {
			if (_addr[0] == _chip_model_list[i].id) {
				_curModelIndex = i;
				return true;
			}
			++i;
		}
	}
	return false;
}

uint8_t* DallasEPROM::getAddress() {
	return _addr;
}

void DallasEPROM::setAddress(uint8_t* pAddress) {
	int i = 0;
	_curModelIndex = -1;

	memcpy(_addr, pAddress, 8);

	while (_chip_model_list[i].id) {
		if (_addr[0] == _chip_model_list[i].id) {
			_curModelIndex = i;
			return;
		}
		++i;
	}
}

const char* DallasEPROM::getDeviceName() {
	if (_curModelIndex >= 0)
		return _chip_model_list[_curModelIndex].name;
	else
		return NULL;
}

bool DallasEPROM::isConnected() {
	uint8_t tmpAddress[8];

	if (!_wire->reset())
		return false;
	_wire->reset_search();
	while (_wire->search(tmpAddress)) {
		if (memcmp(_addr, tmpAddress, 8)==0)
			return true;
	}
	return false;
}

int DallasEPROM::readPage(uint8_t* data, int page) {
	unsigned int address = page * 32;

	if (!isPageValid(page))
		return INVALID_PAGE;
	if (!isConnected())
		return DEVICE_DISCONNECTED;

	// check for page redirection
	if (isEPROMDevice()) {
		byte command[] = { READSTATUS, (byte)(page+1), 0x00 };
		byte new_addr;

		_wire->reset();
		_wire->select(_addr);
		_wire->write(command[0]);
		_wire->write(command[1]);
		_wire->write(command[2]);

		if (OneWire::crc8(command, 3) != _wire->read())
			return CRC_MISMATCH;

		if ((new_addr = _wire->read()) != 0xFF)
			address = new_addr;
	}

	byte command[] = { READMEMORY, (byte) address, (byte)(address >> 8) };

	// send the command and starting address
	_wire->reset();
	_wire->select(_addr);
	_wire->write(command[0]);
	_wire->write(command[1]);
	_wire->write(command[2]);

	// Check CRC on EPROM devices
	if (isEPROMDevice() && OneWire::crc8(command, 3) != _wire->read())
		return CRC_MISMATCH;

	// Read the entire page
	for (int i = 0; i < 32; i++) {
		data[i] = _wire->read();
	}

	// TODO: On EPROM device you can check the CRC post read

	return 0;
}

int DallasEPROM::writePage(uint8_t* data, int page) {
	unsigned int address = page * 32;

	if (!isPageValid(page))
		return INVALID_PAGE;
	if (!isConnected())
		return DEVICE_DISCONNECTED;

	// EEPROMS have a difference write method than EPROMS
	if (!isEPROMDevice()) {
		int status;

		// a page is 4 8-byte scratch writes
		for (int i = 0; i < 32; i += 8) {
			if ((status = scratchWrite(&data[i], 8, address + i)))
				return status;
		}
		return 0;
	}

	byte command[] = { WRITEMEMORY, (byte) address, (byte)(address >> 8),
			data[0] };

	// send the command, address, and the first byte
	_wire->reset();
	_wire->select(_addr);
	_wire->write(command[0]);
	_wire->write(command[1]);
	_wire->write(command[2]);
	_wire->write(command[3]);

	// Check CRC
	if (OneWire::crc8(command, 4) != _wire->read())
		return CRC_MISMATCH;

	// Issue programming pulse for the first byte
	if (_progPin >= 0) {
		digitalWrite(_progPin, HIGH);
		delayMicroseconds(500);
		digitalWrite(_progPin, LOW);
	}
	delayMicroseconds(500);

	// Check the first byte for proper burn
	if (command[3] != _wire->read())
		return COPY_FAILURE;

    // write out the rest of the page
	for (int i = 1; i < 32; i++) {
		// Write byte
		_wire->write(data[i]);
		// Check CRC
		_wire->read();  // FIXME: The EPROM calculates a CRC based on 9 bits, we can't do that with OneWire
		//byte crc[] = { (byte)((address+i) & 0x01), data[i] };
		//if (OneWire::crc8(crc, 2) != _wire->read())
		//	return CRC_MISMATCH;
		// Issue programming pulse
		if (_progPin >= 0) {
			digitalWrite(_progPin, HIGH);
			delayMicroseconds(500);
			digitalWrite(_progPin, LOW);
		}
		delayMicroseconds(500);
		// Check for proper burn
		if (data[i] != _wire->read())
			return COPY_FAILURE;
	}

	return 0;
}

int DallasEPROM::lockPage(int page) {
	if (!isPageValid(page))
		return INVALID_PAGE;
	if (!isConnected())
		return DEVICE_DISCONNECTED;

	_wire->reset();
	_wire->select(_addr);

	if (isEPROMDevice()) {
		byte command[] = { WRITESTATUS, 0x00, 0x00, (1 << page) };

		_wire->write(command[0]);
		_wire->write(command[1]);
		_wire->write(command[2]);
		_wire->write(command[3]);

		// Check CRC
		if (OneWire::crc8(command, 4) != _wire->read())
			return CRC_MISMATCH;

		// Issue programming pulse
		if (_progPin >= 0) {
			digitalWrite(_progPin, HIGH);
			delayMicroseconds(500);
			digitalWrite(_progPin, LOW);
		}
		delayMicroseconds(500);

		// TODO: Verify data
	} else {
		unsigned int start;
		byte data[] = { 0x55 };  // write protect

		start = _chip_model_list[_curModelIndex].pages * 32 + page;
		scratchWrite(data, 1, start);
	}

	return 0;
}

bool DallasEPROM::isPageLocked(int page) {
	byte status;

	if (!isPageValid(page))
		return INVALID_PAGE;
	if (!isConnected())
		return DEVICE_DISCONNECTED;

	_wire->reset();
	_wire->select(_addr);

	if (isEPROMDevice()) {
		byte command[] = { READSTATUS, 0x00, 0x00 };
		_wire->write(command[0]);
		_wire->write(command[1]);
		_wire->write(command[2]);

		// Check CRC on EPROM devices
		if (OneWire::crc8(command, 3) != _wire->read())
			return CRC_MISMATCH;

		status = _wire->read();

		_wire->reset();

		return 1 & (status >> page);
	} else {
		unsigned int start;

		start = _chip_model_list[_curModelIndex].pages * 32 + page;

		_wire->write(READMEMORY);
		_wire->write((byte)start);
		_wire->write((byte)(start >> 8));

		if (_wire->read() == 0x55)
			return true;
		else
			return false;
	}
}

/*******************
 * Private methods
 *******************/

int DallasEPROM::scratchWrite(uint8_t* data, int length, unsigned int address) {
	byte auth[3];

	// send the command and address
	_wire->reset();
	_wire->select(_addr);
	_wire->write(WRITEMEMORY);
	_wire->write((byte) address);
	_wire->write((byte)(address >> 8));

	// write "length" bytes to the scratchpad
	for (int i = 0; i < length; i++)
		_wire->write(data[i]);

	if (_chip_model_list[_curModelIndex].name == "DS2430") {
		// Issue copy scratchpad with DS2430 auth byte
		_wire->reset();
		_wire->select(_addr);
		_wire->write(WRITESTATUS);
		_wire->write(VERIFYRESUME);
	} else {
		// Read the auth code from the scratchpad and verify integrity
		_wire->reset();
		_wire->select(_addr);
		_wire->write(READSTATUS);
		_wire->read_bytes(auth, 3);
		for (int i = 0; i < length; i++) {
			if (_wire->read() != data[i])
				return BAD_INTEGRITY;
		}

		// Issue copy scratchpad with auth bytes
		_wire->reset();
		_wire->select(_addr);
		_wire->write(WRITESTATUS);
		_wire->write(auth[0]);
		_wire->write(auth[1]);
		_wire->write(auth[2], 1);
	}

	// Need 10ms prog delay
	delay(10);
	_wire->depower();

	// Check for success
	if (_chip_model_list[_curModelIndex].name != "DS2430" && _wire->read() != 0xAA)
		return COPY_FAILURE;

	return 0;
}

bool DallasEPROM::isPageValid(int page) {
	if (_curModelIndex >= 0 && page < _chip_model_list[_curModelIndex].pages)
		return true;
	return false;
}

bool DallasEPROM::isEPROMDevice() {
	if (_curModelIndex >= 0 && _chip_model_list[_curModelIndex].isEPROM == true)
		return true;
	return false;
}

/** @file */
