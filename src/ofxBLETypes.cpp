#include "ofxBLETypes.h"
#include <algorithm>
#include <cctype>

namespace {

std::string upperHex(std::string uuid) {
	uuid.erase(std::remove_if(uuid.begin(), uuid.end(), [](unsigned char c) { return std::isspace(c); }), uuid.end());
	for (char & c : uuid) {
		c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
	}
	return uuid;
}

} // namespace

std::string ofxBLENormalizeUuid(const std::string & uuid) {
	const std::string hex = upperHex(uuid);
	if (hex.size() == 4) {
		return "0000" + hex + "-0000-1000-8000-00805F9B34FB";
	}
	if (hex.size() == 8 && hex.find('-') == std::string::npos) {
		return hex + "-0000-1000-8000-00805F9B34FB";
	}
	return hex;
}

bool ofxBLEUuidEqual(const std::string & a, const std::string & b) {
	return ofxBLENormalizeUuid(a) == ofxBLENormalizeUuid(b);
}
