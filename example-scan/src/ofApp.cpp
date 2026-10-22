#include "ofApp.h"

void ofApp::setup() {
	auto found = ofxBLEAdapter::adapters();
	if (found.empty()) {
		lines.push_back("no bluetooth adapter");
		return;
	}
	adapter = found[0];
	lines.push_back(adapter.name() + " (radio state arrives after startScan)");
	ofAddListener(adapter.deviceDiscovered(), this, &ofApp::onDiscovered);
	ofAddListener(adapter.deviceConnected(), this, &ofApp::onConnected);
	ofxBLEScanSettings scan;
	scan.timeoutSeconds = 10;
	adapter.startScan(scan);
}

void ofApp::onDiscovered(ofxBLEDevice & device) {
	lines.push_back(device.name() + "  " + ofToString(device.rssi()) + " dBm");
	if (lines.size() == 2) {
		device.connect();
	}
}

void ofApp::onConnected(ofxBLEDevice & device) {
	lines.push_back("connected " + device.identifier());
	for (const auto & characteristic : device.characteristics()) {
		lines.push_back(characteristic.serviceUuid() + " " + characteristic.uuid());
	}
}

void ofApp::onValue(ofxBLEValue & value) {
	lines.push_back("value " + value.characteristicUuid + " " + ofToString(value.data.size()) + " bytes");
}

void ofApp::draw() {
	ofBackground(20);
	float y = 30;
	for (const auto & line : lines) {
		ofDrawBitmapString(line, 20, y);
		y += 18;
	}
}
