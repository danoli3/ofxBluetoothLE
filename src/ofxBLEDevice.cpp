#include "ofxBLEDevice.h"
#include "backend/ofxBLEBackend.h"

struct ofxBLEDevice::State {
	std::shared_ptr<ofxBLEDeviceBackend> backend;
	ofxBLEAdvertisement advertisement;
	ofEvent<ofxBLEDevice> connectedEvent;
	ofEvent<ofxBLEDevice> disconnectedEvent;
	ofEvent<ofxBLEValue> valueEvent;
};

ofxBLEDevice::ofxBLEDevice(std::shared_ptr<ofxBLEDeviceBackend> backend, ofxBLEAdvertisement advertisement)
	: state(std::make_shared<State>()) {
	state->backend = std::move(backend);
	state->advertisement = std::move(advertisement);
	auto s = state;
	s->backend->onConnected = [s]() {
		ofxBLEDevice device(s);
		ofNotifyEvent(s->connectedEvent, device);
	};
	s->backend->onDisconnected = [s]() {
		ofxBLEDevice device(s);
		ofNotifyEvent(s->disconnectedEvent, device);
	};
	s->backend->onValue = [s](ofxBLEValue value) {
		ofNotifyEvent(s->valueEvent, value);
	};
}

ofxBLEDevice::ofxBLEDevice(std::shared_ptr<State> state)
	: state(std::move(state)) {
}

bool ofxBLEDevice::isValid() const {
	return state && state->backend;
}

std::string ofxBLEDevice::identifier() const {
	return isValid() ? state->backend->identifier() : std::string();
}

std::string ofxBLEDevice::name() const {
	if (!isValid()) {
		return {};
	}
	const std::string advertised = state->advertisement.name;
	if (!advertised.empty()) {
		return advertised;
	}
	return state->backend->name();
}

int ofxBLEDevice::rssi() const {
	if (!state) {
		return 0;
	}
	if (state->advertisement.rssi != 0) {
		return state->advertisement.rssi;
	}
	return state->backend ? state->backend->rssi() : 0;
}

const ofxBLEAdvertisement & ofxBLEDevice::advertisement() const {
	static const ofxBLEAdvertisement empty;
	return state ? state->advertisement : empty;
}

bool ofxBLEDevice::isConnected() const {
	return isValid() && state->backend->isConnected();
}

void ofxBLEDevice::connect() {
	if (isValid()) {
		state->backend->connect();
	}
}

void ofxBLEDevice::disconnect() {
	if (isValid()) {
		state->backend->disconnect();
	}
}

std::vector<ofxBLECharacteristic> ofxBLEDevice::characteristics() const {
	std::vector<ofxBLECharacteristic> out;
	if (!isValid()) {
		return out;
	}
	for (const auto & characteristic : state->backend->characteristics()) {
		out.push_back(ofxBLECharacteristic(characteristic));
	}
	return out;
}

ofxBLECharacteristic ofxBLEDevice::characteristic(const std::string & serviceUuid, const std::string & characteristicUuid) const {
	if (!isValid()) {
		return {};
	}
	for (const auto & characteristic : state->backend->characteristics()) {
		if (ofxBLEUuidEqual(characteristic->serviceUuid(), serviceUuid) && ofxBLEUuidEqual(characteristic->uuid(), characteristicUuid)) {
			return ofxBLECharacteristic(characteristic);
		}
	}
	return {};
}

void ofxBLEDevice::updateAdvertisement(ofxBLEAdvertisement advertisement) {
	if (state) {
		state->advertisement = std::move(advertisement);
	}
}

ofEvent<ofxBLEDevice> & ofxBLEDevice::connected() {
	if (!state) {
		static ofEvent<ofxBLEDevice> empty;
		return empty;
	}
	return state->connectedEvent;
}

ofEvent<ofxBLEDevice> & ofxBLEDevice::disconnected() {
	if (!state) {
		static ofEvent<ofxBLEDevice> empty;
		return empty;
	}
	return state->disconnectedEvent;
}

ofEvent<ofxBLEValue> & ofxBLEDevice::valueReceived() {
	if (!state) {
		static ofEvent<ofxBLEValue> empty;
		return empty;
	}
	return state->valueEvent;
}
