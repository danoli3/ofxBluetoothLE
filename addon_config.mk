meta:
	ADDON_NAME = ofxBluetoothLE
	ADDON_DESCRIPTION = Bluetooth Low Energy central for scan, connect, and GATT
	ADDON_AUTHOR = OF Team
	ADDON_TAGS = "bluetooth" "bluetoothle" "ble"
	ADDON_URL = http://github.com/openframeworks/openFrameworks

common:
	ADDON_SOURCES_EXCLUDE = src/backend/ofxBLEBackendApple.mm

osx:
	ADDON_SOURCES_EXCLUDE = src/backend/ofxBLEBackendUnavailable.cpp
	ADDON_FRAMEWORKS = CoreBluetooth Foundation
	ADDON_CFLAGS = -fobjc-arc

macos:
	ADDON_SOURCES_EXCLUDE = src/backend/ofxBLEBackendUnavailable.cpp
	ADDON_FRAMEWORKS = CoreBluetooth Foundation
	ADDON_CFLAGS = -fobjc-arc

ios:
	ADDON_SOURCES_EXCLUDE = src/backend/ofxBLEBackendUnavailable.cpp
	ADDON_FRAMEWORKS = CoreBluetooth Foundation
	ADDON_CFLAGS = -fobjc-arc

tvos:
	ADDON_SOURCES_EXCLUDE = src/backend/ofxBLEBackendUnavailable.cpp
	ADDON_FRAMEWORKS = CoreBluetooth Foundation
	ADDON_CFLAGS = -fobjc-arc

catos:
	ADDON_SOURCES_EXCLUDE = src/backend/ofxBLEBackendUnavailable.cpp
	ADDON_FRAMEWORKS = CoreBluetooth Foundation
	ADDON_CFLAGS = -fobjc-arc

xros:
	ADDON_SOURCES_EXCLUDE = src/backend/ofxBLEBackendUnavailable.cpp
	ADDON_FRAMEWORKS = CoreBluetooth Foundation
	ADDON_CFLAGS = -fobjc-arc
