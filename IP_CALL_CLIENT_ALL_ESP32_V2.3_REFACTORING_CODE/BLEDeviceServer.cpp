#include "BLEDeviceServer.h"

// === Custom Callbacks ===
class ServerCallbacks : public BLEServerCallbacks {
  BLEDeviceServer* parent;
public:
  ServerCallbacks(BLEDeviceServer* p) : parent(p) {}
  void onConnect(BLEServer* pServer) override {
    if (parent->isLoggingEnabled()) Serial.println("✅ Device connected!");
    parent->startSendingLoop();
  }
  void onDisconnect(BLEServer* pServer) override {
    if (parent->isLoggingEnabled()) Serial.println("❌ Device disconnected!");
  }
};

class CharacteristicCallbacks : public BLECharacteristicCallbacks {
  BLEDeviceServer* parent;
public:
  CharacteristicCallbacks(BLEDeviceServer* p) : parent(p) {}
  void onWrite(BLECharacteristic* pCharacteristic) override {
    String value = pCharacteristic->getValue();
    if (value.length() == 0) return;
    parent->handleWrite(value);
  }
};

// === Start Server ===
void BLEDeviceServer::start() {
  if (loggingEnabled)Serial.println("🚀 Starting BLE Server...");
  BLEDevice::init("ESP32 BLE");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks(this));

  BLEService* pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
    CHAR_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_WRITE |
    BLECharacteristic::PROPERTY_NOTIFY
  );
  pCharacteristic->setCallbacks(new CharacteristicCallbacks(this));
  pService->start();

  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();

  if (loggingEnabled) Serial.println("📡 BLE Server siap dan mengiklankan...");
}

// === Stop Server ===
void BLEDeviceServer::stop() {
  if (pServer) {
    BLEDevice::deinit(true);
    sendingActive = false;
    if (loggingEnabled) Serial.println("🛑 BLE Server dihentikan.");
  }
}

// === Set value device item ===
void BLEDeviceServer::setValue(const String& key, const String& newValue) {
  for (auto& item : deviceItems) {
    if (item.key == key) {
      item.value = newValue;
      if (loggingEnabled) Serial.printf("🔧 %s diubah menjadi %s\n", key.c_str(), newValue.c_str());
      return;
    }
  }
  if (loggingEnabled) Serial.printf("⚠️ Key '%s' tidak ditemukan.\n", key.c_str());
}

// === Kirim data ke Flutter ===
void BLEDeviceServer::sendDataDevice(bool writableOnly) {
  if (!pCharacteristic) return;

  StaticJsonDocument<512> doc;
  for (auto& item : deviceItems) {
    if (item.writable == writableOnly) {
      JsonObject obj = doc.createNestedObject(String(item.id));
      obj["k"] = item.key;
      obj["v"] = item.value;
      obj["w"] = item.writable;
    }
  }

  String output;
  serializeJson(doc, output);
  pCharacteristic->setValue(output.c_str());
  pCharacteristic->notify();
  if (loggingEnabled) Serial.printf("📤 Data terkirim (w=%s): %s\n", writableOnly ? "true" : "false", output.c_str());
}

// === Loop pengiriman otomatis setiap 5s ===
void BLEDeviceServer::startSendingLoop() {
  sendingActive = true;
  xTaskCreate(
    [](void* param) {
      BLEDeviceServer* self = (BLEDeviceServer*)param;
      for (;;) {
        if (self->sendingActive) {
          self->sendDataDevice(false);
          // self->sendWritableNext = !self->sendWritableNext;
        }
        vTaskDelay(500 / portTICK_PERIOD_MS);
      }
    },
    "BLESendTask",
    4096,
    this,
    1,
    NULL
  );
}

// === Handler untuk data dari Flutter ===
void BLEDeviceServer::handleWrite(const String& value) {
  if (loggingEnabled) Serial.print("📥 Data dari Flutter: ");
  if (loggingEnabled) Serial.println(value);
  if (onCommand) {
    onCommand(value.c_str());
  }
}

void BLEDeviceServer::setLogging(bool enabled) {
  loggingEnabled = enabled;
  Serial.printf("🔧 Logging %s\n", enabled ? "diaktifkan" : "dimatikan");
}

// === Kirim notifikasi custom ke Flutter ===
void BLEDeviceServer::sendMessage(const String& message) {
  if (!pCharacteristic) {
    if (loggingEnabled) Serial.println("⚠️ Tidak ada karakteristik BLE untuk mengirim data!");
    return;
  }

  // Set value dan kirim notifikasi ke Flutter
  pCharacteristic->setValue(message.c_str());
  pCharacteristic->notify();

  if (loggingEnabled) {
    Serial.printf("📡 Notifikasi dikirim ke Flutter: %s\n", message.c_str());
  }
}

