#pragma once

#include "ofxBLEDevice.h"
#include <memory>
#include <string>
#include <vector>

class ofxBLEAdapter {
public:
	static std::vector<ofxBLEAdapter> adapters();

	bool isValid() const;
	explicit operator bool() const { return isValid(); }

	std::string name() const;
	bool isAvailable() const;

	void startScan(const ofxBLEScanSettings & settings = {});
	void stopScan();

	ofEvent<ofxBLEDevice> & deviceDiscovered();
	ofEvent<ofxBLEDevice> & deviceConnected();
	ofEvent<ofxBLEDevice> & deviceDisconnected();

private:
	struct State;
	explicit ofxBLEAdapter(std::unique_ptr<class ofxBLEAdapterBackend> backend);
	std::shared_ptr<State> state;
};
