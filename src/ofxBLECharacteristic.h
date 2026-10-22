#pragma once

#include "ofxBLETypes.h"
#include <memory>

class ofxBLECharacteristicBackend;

class ofxBLECharacteristic {
public:
	ofxBLECharacteristic() = default;

	bool isValid() const;
	explicit operator bool() const { return isValid(); }

	std::string uuid() const;
	std::string serviceUuid() const;
	std::uint32_t properties() const;
	bool canRead() const;
	bool canWrite() const;
	bool canWriteWithoutResponse() const;
	bool canNotify() const;

	ofBuffer value() const;
	void read();
	void write(const ofBuffer & data);
	void write(const std::string & data);
	void writeWithoutResponse(const ofBuffer & data);
	void subscribe();
	void unsubscribe();

private:
	friend class ofxBLEDevice;
	explicit ofxBLECharacteristic(std::shared_ptr<ofxBLECharacteristicBackend> backend);
	std::shared_ptr<ofxBLECharacteristicBackend> backend;
};
