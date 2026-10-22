#include "ofxBLEBackend.h"
#include "ofLog.h"

namespace {

class ofxBLEUnavailableAdapter : public ofxBLEAdapterBackend {
public:
	std::string name() const override {
		return "Bluetooth LE";
	}
	bool isAvailable() const override {
		return false;
	}
	void startScan(const ofxBLEScanSettings &, std::function<void(std::shared_ptr<ofxBLEDeviceBackend>, ofxBLEAdvertisement)>) override {
		ofLogError("ofxBluetoothLE") << "This build has no Bluetooth LE backend. Apple uses CoreBluetooth. Windows (WinRT), Linux (BlueZ), Android, and Emscripten are not implemented yet.";
	}
	void stopScan() override {
	}
};

} // namespace

std::vector<std::unique_ptr<ofxBLEAdapterBackend>> ofxBLECreateAdapters() {
	std::vector<std::unique_ptr<ofxBLEAdapterBackend>> adapters;
	adapters.push_back(std::make_unique<ofxBLEUnavailableAdapter>());
	return adapters;
}
