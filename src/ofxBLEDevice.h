#pragma once

#include "ofxBLECharacteristic.h"
#include "ofEvents.h"
#include <memory>
#include <string>
#include <vector>

class ofxBLEDeviceBackend;

class ofxBLEDevice {
public:
	ofxBLEDevice() = default;

	bool isValid() const;
	explicit operator bool() const { return isValid(); }

	std::string identifier() const;
	std::string name() const;
	int rssi() const;
	const ofxBLEAdvertisement & advertisement() const;
	bool isConnected() const;

	void connect();
	void disconnect();

	std::vector<ofxBLECharacteristic> characteristics() const;
	ofxBLECharacteristic characteristic(const std::string & serviceUuid, const std::string & characteristicUuid) const;

	ofEvent<ofxBLEDevice> & connected();
	ofEvent<ofxBLEDevice> & disconnected();
	ofEvent<ofxBLEValue> & valueReceived();

private:
	friend class ofxBLEAdapter;
	struct State;
	explicit ofxBLEDevice(std::shared_ptr<ofxBLEDeviceBackend> backend, ofxBLEAdvertisement advertisement);
	explicit ofxBLEDevice(std::shared_ptr<State> state);
	void updateAdvertisement(ofxBLEAdvertisement advertisement);
	std::shared_ptr<State> state;
};
