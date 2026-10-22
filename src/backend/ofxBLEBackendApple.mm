#include "ofxBLEBackend.h"
#include "ofLog.h"
#include <CoreBluetooth/CoreBluetooth.h>
#include <Foundation/Foundation.h>
#include <map>
#include <mutex>
#include <memory>

namespace {

std::string ofxString(NSString * value) {
	if (value == nil || value.UTF8String == nullptr) {
		return {};
	}
	return value.UTF8String;
}

std::uint32_t ofxProperties(CBCharacteristicProperties properties) {
	std::uint32_t out = 0;
	if (properties & CBCharacteristicPropertyRead) {
		out |= static_cast<std::uint32_t>(ofxBLEProperty::Read);
	}
	if (properties & CBCharacteristicPropertyWrite) {
		out |= static_cast<std::uint32_t>(ofxBLEProperty::Write);
	}
	if (properties & CBCharacteristicPropertyWriteWithoutResponse) {
		out |= static_cast<std::uint32_t>(ofxBLEProperty::WriteWithoutResponse);
	}
	if (properties & CBCharacteristicPropertyNotify) {
		out |= static_cast<std::uint32_t>(ofxBLEProperty::Notify);
	}
	if (properties & CBCharacteristicPropertyIndicate) {
		out |= static_cast<std::uint32_t>(ofxBLEProperty::Indicate);
	}
	return out;
}

ofBuffer ofxBuffer(NSData * data) {
	ofBuffer buffer;
	if (data != nil && data.bytes != nullptr && data.length > 0) {
		buffer.set(static_cast<const char *>(data.bytes), data.length);
	}
	return buffer;
}

class ofxAppleCharacteristic;
class ofxAppleDevice;

} // namespace

@interface OfxBLECentral : NSObject <CBCentralManagerDelegate, CBPeripheralDelegate> {
@public
	std::function<void(std::shared_ptr<ofxBLEDeviceBackend>, ofxBLEAdvertisement)> onFound;
	ofxBLEScanSettings scanSettings;
	bool scanRequested;
	int scanGeneration;
	std::map<std::string, std::shared_ptr<ofxBLEDeviceBackend>> devices;
}
@property(nonatomic, strong) CBCentralManager * manager;
- (void)beginScanIfPowered;
@end

namespace {

class ofxAppleCharacteristic : public ofxBLECharacteristicBackend {
public:
	ofxAppleCharacteristic(CBPeripheral * peripheral, CBCharacteristic * characteristic, std::string serviceUuid, std::function<void(ofxBLEValue)> onValue)
		: peripheral(peripheral)
		, characteristic(characteristic)
		, service(std::move(serviceUuid))
		, onValue(std::move(onValue)) {
	}

	std::string uuid() const override {
		return ofxString(characteristic.UUID.UUIDString);
	}
	std::string serviceUuid() const override {
		return service;
	}
	std::uint32_t properties() const override {
		return ofxProperties(characteristic.properties);
	}
	ofBuffer value() const override {
		std::lock_guard<std::mutex> lock(mutex);
		return cached;
	}
	void read() override {
		[peripheral readValueForCharacteristic:characteristic];
	}
	void write(const ofBuffer & data, bool withResponse) override {
		NSData * bytes = [NSData dataWithBytes:data.getData() length:data.size()];
		const auto type = withResponse ? CBCharacteristicWriteWithResponse : CBCharacteristicWriteWithoutResponse;
		[peripheral writeValue:bytes forCharacteristic:characteristic type:type];
	}
	void setNotify(bool enable) override {
		[peripheral setNotifyValue:enable forCharacteristic:characteristic];
	}

	void update(NSData * data) {
		ofxBLEValue packet;
		{
			std::lock_guard<std::mutex> lock(mutex);
			cached = ofxBuffer(data);
			packet.data = cached;
		}
		packet.serviceUuid = service;
		packet.characteristicUuid = uuid();
		if (onValue) {
			onValue(packet);
		}
	}

	CBCharacteristic * objc() const {
		return characteristic;
	}

private:
	CBPeripheral * peripheral;
	CBCharacteristic * characteristic;
	std::string service;
	std::function<void(ofxBLEValue)> onValue;
	mutable std::mutex mutex;
	ofBuffer cached;
};

class ofxAppleDevice : public ofxBLEDeviceBackend, public std::enable_shared_from_this<ofxAppleDevice> {
public:
	ofxAppleDevice(OfxBLECentral * central, CBPeripheral * peripheral)
		: central(central)
		, peripheral(peripheral) {
		peripheral.delegate = central;
	}

	void detach() {
		if (peripheral.delegate == central) {
			peripheral.delegate = nil;
		}
		central = nil;
	}

	std::string identifier() const override {
		return ofxString(peripheral.identifier.UUIDString);
	}
	std::string name() const override {
		return ofxString(peripheral.name);
	}
	int rssi() const override {
		return lastRssi;
	}
	bool isConnected() const override {
		return peripheral.state == CBPeripheralStateConnected && discovered;
	}
	void connect() override {
		OfxBLECentral * owner = central;
		if (owner != nil) {
			[owner.manager connectPeripheral:peripheral options:nil];
		}
	}
	void disconnect() override {
		OfxBLECentral * owner = central;
		if (owner != nil) {
			[owner.manager cancelPeripheralConnection:peripheral];
		}
	}
	std::vector<std::shared_ptr<ofxBLECharacteristicBackend>> characteristics() const override {
		std::lock_guard<std::mutex> lock(mutex);
		std::vector<std::shared_ptr<ofxBLECharacteristicBackend>> out;
		out.reserve(chars.size());
		for (const auto & characteristic : chars) {
			out.push_back(characteristic);
		}
		return out;
	}

	void noteRssi(int rssi) {
		lastRssi = rssi;
	}

	void discover() {
		{
			std::lock_guard<std::mutex> lock(mutex);
			chars.clear();
		}
		pendingServices = static_cast<int>(peripheral.services.count);
		if (pendingServices == 0) {
			markReady();
			return;
		}
		for (CBService * service in peripheral.services) {
			[peripheral discoverCharacteristics:nil forService:service];
		}
	}

	void addCharacteristics(CBService * service) {
		const std::string serviceUuid = ofxString(service.UUID.UUIDString);
		std::weak_ptr<ofxAppleDevice> weak = shared_from_this();
		{
			std::lock_guard<std::mutex> lock(mutex);
			for (CBCharacteristic * characteristic in service.characteristics) {
				auto impl = std::make_shared<ofxAppleCharacteristic>(peripheral, characteristic, serviceUuid, [weak](ofxBLEValue value) {
					auto self = weak.lock();
					if (!self) {
						return;
					}
					value.deviceId = self->identifier();
					if (self->onValue) {
						self->onValue(std::move(value));
					}
				});
				chars.push_back(impl);
			}
		}
		if (pendingServices > 0) {
			pendingServices--;
		}
		if (pendingServices == 0) {
			markReady();
		}
	}

	void failCharacteristic(CBCharacteristic * characteristic, NSError * error) {
		ofLogError("ofxBluetoothLE") << ofxString(error.localizedDescription);
		(void)characteristic;
	}

	std::shared_ptr<ofxAppleCharacteristic> find(CBCharacteristic * characteristic) const {
		std::lock_guard<std::mutex> lock(mutex);
		for (const auto & impl : chars) {
			if (impl->objc() == characteristic) {
				return impl;
			}
		}
		return nullptr;
	}

	void markDisconnected() {
		discovered = false;
		if (onDisconnected) {
			onDisconnected();
		}
	}

private:
	void markReady() {
		if (discovered) {
			return;
		}
		discovered = true;
		if (onConnected) {
			onConnected();
		}
	}

	__weak OfxBLECentral * central;
	CBPeripheral * peripheral;
	int lastRssi = 0;
	int pendingServices = 0;
	bool discovered = false;
	mutable std::mutex mutex;
	std::vector<std::shared_ptr<ofxAppleCharacteristic>> chars;
};

ofxBLEAdvertisement ofxAdvertisement(CBPeripheral * peripheral, NSDictionary * data, NSNumber * rssi) {
	ofxBLEAdvertisement advertisement;
	advertisement.identifier = ofxString(peripheral.identifier.UUIDString);
	advertisement.name = ofxString(data[CBAdvertisementDataLocalNameKey]);
	if (advertisement.name.empty()) {
		advertisement.name = ofxString(peripheral.name);
	}
	advertisement.rssi = rssi.intValue;
	NSNumber * connectable = data[CBAdvertisementDataIsConnectable];
	advertisement.connectable = connectable == nil || connectable.boolValue;
	NSArray * services = data[CBAdvertisementDataServiceUUIDsKey];
	for (CBUUID * uuid in services) {
		advertisement.serviceUuids.push_back(ofxString(uuid.UUIDString));
	}
	NSData * manufacturer = data[CBAdvertisementDataManufacturerDataKey];
	if (manufacturer.length >= 2) {
		const auto * bytes = static_cast<const unsigned char *>(manufacturer.bytes);
		const std::uint16_t company = static_cast<std::uint16_t>(bytes[0] | (bytes[1] << 8));
		ofBuffer payload;
		if (manufacturer.length > 2) {
			payload.set(reinterpret_cast<const char *>(bytes + 2), manufacturer.length - 2);
		}
		advertisement.manufacturerData.emplace(company, std::move(payload));
	}
	return advertisement;
}

class ofxAppleAdapter : public ofxBLEAdapterBackend {
public:
	ofxAppleAdapter() {
		central = [[OfxBLECentral alloc] init];
		central.manager = [[CBCentralManager alloc] initWithDelegate:central queue:dispatch_get_main_queue()];
	}

	~ofxAppleAdapter() override {
		for (auto & entry : central->devices) {
			if (auto device = std::dynamic_pointer_cast<ofxAppleDevice>(entry.second)) {
				device->detach();
			}
		}
		[central.manager stopScan];
		central.manager.delegate = nil;
	}

	std::string name() const override {
		return "CoreBluetooth";
	}
	bool isAvailable() const override {
		return central.manager.state == CBManagerStatePoweredOn;
	}
	void startScan(const ofxBLEScanSettings & settings, std::function<void(std::shared_ptr<ofxBLEDeviceBackend>, ofxBLEAdvertisement)> onFound) override {
		central->onFound = std::move(onFound);
		central->scanSettings = settings;
		central->scanRequested = true;
		central->scanGeneration++;
		const int generation = central->scanGeneration;
		[central beginScanIfPowered];
		if (settings.timeoutSeconds > 0) {
			const auto delay = static_cast<int64_t>(settings.timeoutSeconds * NSEC_PER_SEC);
			OfxBLECentral * owner = central;
			dispatch_after(dispatch_time(DISPATCH_TIME_NOW, delay), dispatch_get_main_queue(), ^{
				if (owner->scanGeneration == generation) {
					[owner.manager stopScan];
					owner->scanRequested = false;
				}
			});
		}
	}
	void stopScan() override {
		central->scanRequested = false;
		central->scanGeneration++;
		[central.manager stopScan];
	}

	OfxBLECentral * central = nil;
};

} // namespace

@implementation OfxBLECentral

- (void)beginScanIfPowered {
	if (!scanRequested || self.manager.state != CBManagerStatePoweredOn) {
		return;
	}
	NSMutableArray * uuids = [NSMutableArray array];
	for (const auto & uuid : scanSettings.serviceUuids) {
		const std::string normalized = ofxBLENormalizeUuid(uuid);
		[uuids addObject:[CBUUID UUIDWithString:@(normalized.c_str())]];
	}
	NSDictionary * options = @{ CBCentralManagerScanOptionAllowDuplicatesKey : @(scanSettings.allowDuplicates) };
	[self.manager scanForPeripheralsWithServices:(uuids.count > 0 ? uuids : nil) options:options];
}

- (void)centralManagerDidUpdateState:(CBCentralManager *)manager {
	if (manager.state == CBManagerStateUnauthorized) {
		ofLogError("ofxBluetoothLE") << "Bluetooth permission was denied. Add NSBluetoothAlwaysUsageDescription to Info.plist.";
	} else if (manager.state == CBManagerStateUnsupported) {
		ofLogError("ofxBluetoothLE") << "This Mac or device has no Bluetooth LE radio.";
	} else if (manager.state == CBManagerStatePoweredOff) {
		ofLogNotice("ofxBluetoothLE") << "Bluetooth is powered off.";
	}
	[self beginScanIfPowered];
}

- (void)centralManager:(CBCentralManager *)manager didDiscoverPeripheral:(CBPeripheral *)peripheral advertisementData:(NSDictionary *)advertisementData RSSI:(NSNumber *)RSSI {
	(void)manager;
	const std::string id = ofxString(peripheral.identifier.UUIDString);
	auto found = devices.find(id);
	std::shared_ptr<ofxAppleDevice> device;
	if (found == devices.end()) {
		device = std::make_shared<ofxAppleDevice>(self, peripheral);
		devices.emplace(id, device);
	} else {
		device = std::dynamic_pointer_cast<ofxAppleDevice>(found->second);
	}
	if (device) {
		device->noteRssi(RSSI.intValue);
	}
	if (onFound) {
		onFound(device, ofxAdvertisement(peripheral, advertisementData, RSSI));
	}
}

- (void)centralManager:(CBCentralManager *)manager didConnectPeripheral:(CBPeripheral *)peripheral {
	(void)manager;
	auto found = devices.find(ofxString(peripheral.identifier.UUIDString));
	if (found == devices.end()) {
		return;
	}
	auto device = std::dynamic_pointer_cast<ofxAppleDevice>(found->second);
	if (!device) {
		return;
	}
	[peripheral discoverServices:nil];
}

- (void)centralManager:(CBCentralManager *)manager didFailToConnectPeripheral:(CBPeripheral *)peripheral error:(NSError *)error {
	(void)manager;
	(void)peripheral;
	ofLogError("ofxBluetoothLE") << "connect failed: " << ofxString(error.localizedDescription);
}

- (void)centralManager:(CBCentralManager *)manager didDisconnectPeripheral:(CBPeripheral *)peripheral error:(NSError *)error {
	(void)manager;
	if (error != nil) {
		ofLogNotice("ofxBluetoothLE") << "disconnected: " << ofxString(error.localizedDescription);
	}
	auto found = devices.find(ofxString(peripheral.identifier.UUIDString));
	if (found == devices.end()) {
		return;
	}
	if (auto device = std::dynamic_pointer_cast<ofxAppleDevice>(found->second)) {
		device->markDisconnected();
	}
}

- (void)peripheral:(CBPeripheral *)peripheral didDiscoverServices:(NSError *)error {
	if (error != nil) {
		ofLogError("ofxBluetoothLE") << ofxString(error.localizedDescription);
		return;
	}
	auto found = devices.find(ofxString(peripheral.identifier.UUIDString));
	if (found == devices.end()) {
		return;
	}
	if (auto device = std::dynamic_pointer_cast<ofxAppleDevice>(found->second)) {
		device->discover();
	}
}

- (void)peripheral:(CBPeripheral *)peripheral didDiscoverCharacteristicsForService:(CBService *)service error:(NSError *)error {
	auto found = devices.find(ofxString(peripheral.identifier.UUIDString));
	if (found == devices.end()) {
		return;
	}
	auto device = std::dynamic_pointer_cast<ofxAppleDevice>(found->second);
	if (!device) {
		return;
	}
	if (error != nil) {
		ofLogError("ofxBluetoothLE") << ofxString(error.localizedDescription);
	}
	device->addCharacteristics(service);
}

- (void)peripheral:(CBPeripheral *)peripheral didUpdateValueForCharacteristic:(CBCharacteristic *)characteristic error:(NSError *)error {
	(void)peripheral;
	auto found = devices.find(ofxString(peripheral.identifier.UUIDString));
	if (found == devices.end()) {
		return;
	}
	auto device = std::dynamic_pointer_cast<ofxAppleDevice>(found->second);
	if (!device) {
		return;
	}
	if (error != nil) {
		device->failCharacteristic(characteristic, error);
		return;
	}
	auto impl = device->find(characteristic);
	if (impl) {
		impl->update(characteristic.value);
	}
}

@end

std::vector<std::unique_ptr<ofxBLEAdapterBackend>> ofxBLECreateAdapters() {
	std::vector<std::unique_ptr<ofxBLEAdapterBackend>> adapters;
	adapters.push_back(std::make_unique<ofxAppleAdapter>());
	return adapters;
}
