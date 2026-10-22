#include "ofxBLECharacteristic.h"
#include "backend/ofxBLEBackend.h"

ofxBLECharacteristic::ofxBLECharacteristic(std::shared_ptr<ofxBLECharacteristicBackend> backend)
	: backend(std::move(backend)) {
}

bool ofxBLECharacteristic::isValid() const {
	return backend != nullptr;
}

std::string ofxBLECharacteristic::uuid() const {
	return backend ? backend->uuid() : std::string();
}

std::string ofxBLECharacteristic::serviceUuid() const {
	return backend ? backend->serviceUuid() : std::string();
}

std::uint32_t ofxBLECharacteristic::properties() const {
	return backend ? backend->properties() : 0;
}

bool ofxBLECharacteristic::canRead() const {
	return ofxBLEHas(properties(), ofxBLEProperty::Read);
}

bool ofxBLECharacteristic::canWrite() const {
	return ofxBLEHas(properties(), ofxBLEProperty::Write);
}

bool ofxBLECharacteristic::canWriteWithoutResponse() const {
	return ofxBLEHas(properties(), ofxBLEProperty::WriteWithoutResponse);
}

bool ofxBLECharacteristic::canNotify() const {
	return ofxBLEHas(properties(), ofxBLEProperty::Notify) || ofxBLEHas(properties(), ofxBLEProperty::Indicate);
}

ofBuffer ofxBLECharacteristic::value() const {
	return backend ? backend->value() : ofBuffer();
}

void ofxBLECharacteristic::read() {
	if (backend) {
		backend->read();
	}
}

void ofxBLECharacteristic::write(const ofBuffer & data) {
	if (backend) {
		backend->write(data, true);
	}
}

void ofxBLECharacteristic::write(const std::string & data) {
	ofBuffer buffer;
	buffer.set(data.data(), data.size());
	write(buffer);
}

void ofxBLECharacteristic::writeWithoutResponse(const ofBuffer & data) {
	if (backend) {
		backend->write(data, false);
	}
}

void ofxBLECharacteristic::subscribe() {
	if (backend) {
		backend->setNotify(true);
	}
}

void ofxBLECharacteristic::unsubscribe() {
	if (backend) {
		backend->setNotify(false);
	}
}
