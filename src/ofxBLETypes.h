#pragma once

#include "ofFileUtils.h"
#include <cstdint>
#include <map>
#include <string>
#include <vector>

// Bluetooth base UUID: 0000xxxx-0000-1000-8000-00805F9B34FB
std::string ofxBLENormalizeUuid(const std::string & uuid);
bool ofxBLEUuidEqual(const std::string & a, const std::string & b);

enum class ofxBLEProperty : std::uint32_t {
	Read = 1u << 0,
	Write = 1u << 1,
	WriteWithoutResponse = 1u << 2,
	Notify = 1u << 3,
	Indicate = 1u << 4,
};

inline ofxBLEProperty operator|(ofxBLEProperty a, ofxBLEProperty b) {
	return ofxBLEProperty(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
}

inline bool ofxBLEHas(std::uint32_t properties, ofxBLEProperty flag) {
	return (properties & static_cast<std::uint32_t>(flag)) != 0;
}

struct ofxBLEAdvertisement {
	std::string name;
	std::string identifier;
	int rssi = 0;
	bool connectable = false;
	std::vector<std::string> serviceUuids;
	// Company id from the first two bytes of the manufacturer field.
	std::map<std::uint16_t, ofBuffer> manufacturerData;
};

struct ofxBLEScanSettings {
	// Empty scans for every peripheral. 16-bit, 32-bit, or 128-bit text is accepted.
	std::vector<std::string> serviceUuids;
	bool allowDuplicates = false;
	// 0 scans until stopScan().
	float timeoutSeconds = 0;
};

struct ofxBLEValue {
	std::string deviceId;
	std::string serviceUuid;
	std::string characteristicUuid;
	ofBuffer data;
};
