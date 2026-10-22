#pragma once

#include "ofMain.h"
#include "ofxBluetoothLE.h"

class ofApp : public ofBaseApp {
public:
	void setup() override;
	void draw() override;
	void onDiscovered(ofxBLEDevice & device);
	void onConnected(ofxBLEDevice & device);
	void onValue(ofxBLEValue & value);

	ofxBLEAdapter adapter;
	std::vector<std::string> lines;
};
