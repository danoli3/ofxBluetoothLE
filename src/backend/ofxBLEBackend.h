#pragma once

#include "ofxBLETypes.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

// Platform backends implement these. The public classes are shared handles.

class ofxBLECharacteristicBackend {
public:
	virtual ~ofxBLECharacteristicBackend() = default;
	virtual std::string uuid() const = 0;
	virtual std::string serviceUuid() const = 0;
	virtual std::uint32_t properties() const = 0;
	virtual ofBuffer value() const = 0;
	virtual void read() = 0;
	virtual void write(const ofBuffer & data, bool withResponse) = 0;
	virtual void setNotify(bool enable) = 0;
};

class ofxBLEDeviceBackend {
public:
	virtual ~ofxBLEDeviceBackend() = default;
	virtual std::string identifier() const = 0;
	virtual std::string name() const = 0;
	virtual int rssi() const = 0;
	virtual bool isConnected() const = 0;
	virtual void connect() = 0;
	virtual void disconnect() = 0;
	virtual std::vector<std::shared_ptr<ofxBLECharacteristicBackend>> characteristics() const = 0;

	std::function<void()> onConnected;
	std::function<void()> onDisconnected;
	std::function<void(ofxBLEValue)> onValue;
};

class ofxBLEAdapterBackend {
public:
	virtual ~ofxBLEAdapterBackend() = default;
	virtual std::string name() const = 0;
	virtual bool isAvailable() const = 0;
	virtual void startScan(const ofxBLEScanSettings & settings, std::function<void(std::shared_ptr<ofxBLEDeviceBackend>, ofxBLEAdvertisement)> onFound) = 0;
	virtual void stopScan() = 0;
};

std::vector<std::unique_ptr<ofxBLEAdapterBackend>> ofxBLECreateAdapters();
