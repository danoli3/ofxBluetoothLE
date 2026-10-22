# ofxBluetoothLE

MIT Bluetooth Low Energy addon. The public API is a central: list adapters, scan, connect, then read, write, and subscribe to GATT characteristics.

The implementation calls the platform stack directly. On Apple that stack is CoreBluetooth. Windows (WinRT), Linux (BlueZ), Android, and Emscripten (Web Bluetooth) use the same backend interface and currently report that they are not implemented.

Classic Bluetooth (serial, audio, HID) is not part of this addon.

## Use

Add `NSBluetoothAlwaysUsageDescription` to the app Info.plist. iOS also needs Bluetooth permission in the target capabilities.

```cpp
#include "ofxBluetoothLE.h"

void ofApp::setup() {
    auto found = ofxBLEAdapter::adapters();
    if (found.empty()) {
        return;
    }
    adapter = found[0];
    ofAddListener(adapter.deviceDiscovered(), this, &ofApp::onDiscovered);
    ofAddListener(adapter.deviceConnected(), this, &ofApp::onConnected);

    ofxBLEScanSettings scan;
    scan.timeoutSeconds = 8;
    adapter.startScan(scan);
}

void ofApp::onDiscovered(ofxBLEDevice & device) {
    ofLogNotice() << device.name() << " " << device.rssi();
    device.connect();
}

void ofApp::onConnected(ofxBLEDevice & device) {
    auto battery = device.characteristic("180F", "2A19");
    if (battery) {
        ofAddListener(device.valueReceived(), this, &ofApp::onValue);
        battery.read();
        battery.subscribe();
    }
}
```

16-bit UUIDs (`180F`) and full 128-bit UUIDs both work. `startScan` waits if the radio has not reported powered-on yet. `isAvailable()` is false until that callback.

`ofxBLEAdapter` and `ofxBLEDevice` are copyable handles. Copies share the radio session.
