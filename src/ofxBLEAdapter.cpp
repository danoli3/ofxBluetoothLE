#include "ofxBLEAdapter.h"
#include "backend/ofxBLEBackend.h"
#include "ofEventUtils.h"
#include <map>

struct ofxBLEAdapter::State {
	std::unique_ptr<ofxBLEAdapterBackend> backend;
	ofEvent<ofxBLEDevice> discoveredEvent;
	ofEvent<ofxBLEDevice> connectedEvent;
	ofEvent<ofxBLEDevice> disconnectedEvent;
	std::map<std::string, ofxBLEDevice> devices;

	void onDeviceConnected(ofxBLEDevice & device) {
		ofNotifyEvent(connectedEvent, device);
	}
	void onDeviceDisconnected(ofxBLEDevice & device) {
		ofNotifyEvent(disconnectedEvent, device);
	}
};

ofxBLEAdapter::ofxBLEAdapter(std::unique_ptr<ofxBLEAdapterBackend> backend)
	: state(std::make_shared<State>()) {
	state->backend = std::move(backend);
}

std::vector<ofxBLEAdapter> ofxBLEAdapter::adapters() {
	std::vector<ofxBLEAdapter> out;
	for (auto & backend : ofxBLECreateAdapters()) {
		out.push_back(ofxBLEAdapter(std::move(backend)));
	}
	return out;
}

bool ofxBLEAdapter::isValid() const {
	return state && state->backend;
}

std::string ofxBLEAdapter::name() const {
	return isValid() ? state->backend->name() : std::string();
}

bool ofxBLEAdapter::isAvailable() const {
	return isValid() && state->backend->isAvailable();
}

void ofxBLEAdapter::startScan(const ofxBLEScanSettings & settings) {
	if (!isValid()) {
		return;
	}
	auto s = state;
	s->backend->startScan(settings, [s](std::shared_ptr<ofxBLEDeviceBackend> backend, ofxBLEAdvertisement advertisement) {
		const std::string id = backend->identifier();
		auto found = s->devices.find(id);
		if (found == s->devices.end()) {
			ofxBLEDevice device(std::move(backend), std::move(advertisement));
			ofAddListener(device.connected(), s.get(), &ofxBLEAdapter::State::onDeviceConnected);
			ofAddListener(device.disconnected(), s.get(), &ofxBLEAdapter::State::onDeviceDisconnected);
			s->devices.emplace(id, device);
		} else {
			found->second.updateAdvertisement(std::move(advertisement));
		}
		ofxBLEDevice device = s->devices.at(id);
		ofNotifyEvent(s->discoveredEvent, device);
	});
}

void ofxBLEAdapter::stopScan() {
	if (isValid()) {
		state->backend->stopScan();
	}
}

ofEvent<ofxBLEDevice> & ofxBLEAdapter::deviceDiscovered() {
	return state->discoveredEvent;
}

ofEvent<ofxBLEDevice> & ofxBLEAdapter::deviceConnected() {
	return state->connectedEvent;
}

ofEvent<ofxBLEDevice> & ofxBLEAdapter::deviceDisconnected() {
	return state->disconnectedEvent;
}
