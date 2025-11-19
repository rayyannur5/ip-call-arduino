#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <ArduinoJson.h>

struct DeviceItem {
  int id;
  String key;
  String value;
  bool writable;
};

class BLEDeviceServer {
public:
  // === Callback untuk komunikasi ke luar ===
  std::function<void(const char* value)> onCommand;

  // === Konstruktor ===
  BLEDeviceServer() {}

  // === Fungsi utama ===
  void start();
  void stop();
  void setValue(const String& key, const String& newValue);
  void sendDataDevice(bool writableOnly);
  void handleWrite(const String& value);
  void startSendingLoop();

  // === Logging control ===
  void setLogging(bool enabled);
  bool isLoggingEnabled() const { return loggingEnabled; }
  void sendMessage(const String& message);

private:
  BLEServer* pServer = nullptr;
  BLECharacteristic* pCharacteristic = nullptr;
  bool deviceConnected = false;
  bool sendingActive = false;
  bool loggingEnabled = false; 

  unsigned long lastSendTime = 0;
  bool sendWritableNext = false; // toggle kirim w=false / w=true

  // === Data Dummy Device ===
  DeviceItem deviceItems[8] = {
    {1, "ID", "-", true},
    {2, "Device Type", "", true},
    {3, "IP Current", "", false},
    {4, "Mac Addr", "", false},
    {5, "WiFi Strength", "", false},
    {6, "SSID", "", true},
    {7, "Password", "", true},
    {8, "IP", "", true}
  };

  // === UUID BLE ===
  const char* SERVICE_UUID = "12345678-1234-5678-1234-56789abcdef0";
  const char* CHAR_UUID    = "12345678-1234-5678-1234-56789abcdef1";
};
