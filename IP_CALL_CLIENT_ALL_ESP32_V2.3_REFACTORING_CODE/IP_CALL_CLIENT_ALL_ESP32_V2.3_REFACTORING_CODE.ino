#include <WiFi.h>
#include <AsyncMqttClient.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <HTTPClient.h>
#include <FS.h>
#include <ArduinoJson.h>
#include "BLEDeviceServer.h"
#include "web_pages.h"
#include "Tombol.h"

// CONFIGURATION
//////////////////////////////////////////////

String ssid = "";
String password = "ipcall123";
String id = "";
String DEVICE_TYPE = "";
String NURSESTATION = "";
String STATIC_IP = "";

#define MQTT_HOST "192.168.0.1"
#define MQTT_PORT 1883

///////////////////////////////////////////////


// LOG LEVEL
bool log_wifi = false;
bool log_mqtt = true;
bool log_device = false;
bool log_setup = false;


// GLOBAL VARIABEL
///////////////////////////////////////////////
bool state_wifi = false;
u_long state_wifi_timer = 0;

bool state_mqtt = false;
bool state_mqtt_first = true;
u_long state_mqtt_timer = 0;
bool first_mqtt_connect = false;

AsyncMqttClient mqttClient;

bool mode_ap = false;

u_long timer_ping = 0;
u_long timer_device_activation = 0;

AsyncWebServer server(80);

String hasil_scan_network;
bool trigger_scan_network = false;

u_long reset_periodic_timer = 0;
bool activity = false;
u_long time_reset_setelah_activity = 0;

DynamicJsonDocument doc(1024);

BLEDeviceServer bleServer;

////////////////////////////////////////////////

#define BUZZER    16
#define ANALOGPIN A0
// #define BTN_SETUP D1
#define BTN_SETUP 22

Tombol btn_setup(BTN_SETUP, BUZZER);

//LAMPP
#define LAMPP_LED_merah 	10
#define LAMPP_LED_hijau 	11
#define LAMPP_LED_biru	  12
#define LAMPP_LED_kuning 	13
#define LAMPP_BTN_SETUP 	15


String lampp_activeTopic[20];
char lampp_valTopic[20];
int lampp_countTopic = 0;
bool lampp_newData = false;
u_long lampp_before = 0;
bool lampp_state = false;
int lampp_index_saat_ini = 0;


// BED AND TOILET VARIABLE
#define BED_PIN_ASSIST 21
#define BED_PIN_INFUS 19
#define BED_PIN_EMERGENCY 23
#define BED_PIN_CANCEL 18
#define BED_LED_CANCEL 17

Tombol btn_assist(BED_PIN_ASSIST, BUZZER);
Tombol btn_infus(BED_PIN_INFUS, BUZZER);
Tombol btn_emergency(BED_PIN_EMERGENCY, BUZZER, LOW, 0);
Tombol btn_cancel(BED_PIN_CANCEL, BUZZER);

String MQTT_PUB_EMERGENCY;
String MQTT_PUB_INFUS;
String MQTT_PUB_ASSIST;


int state_tombol = 0;

int clear_dari_server = 0;
u_long timer_setelah_klik = 0;

bool state_led = false;
u_long state_led_timer = 0;

bool waiting_to_setup = false;
u_long waiting_to_setup_timer = 0;
u_long time_buzzer_ap = 0;
bool state_buzzer_ap = false;
u_long timeout_ap = 0;

bool init_bed_toilet = true;
u_long time_init_bed_toilet = 0;
///////////////////////////////////////////////


void connectToWifi() {
  if(!mode_ap) {
    if(log_wifi) {
      Serial.print("[WiFi] Connecting to Wi-Fi ");
      Serial.print(ssid);
      Serial.print(" : ");
      Serial.println(password);
    }
    WiFi.begin(ssid, password);
  }
}


void WiFiEvent(WiFiEvent_t event) {
  if (mode_ap) return;
    Serial.printf("[WiFi-event] event: %d\n", event);
    switch(event) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        if(log_wifi){
          Serial.print("[WiFi] Connected to Wi-Fi.  ");
          Serial.println(WiFi.localIP());
        }

        bleServer.setValue("IP Current", WiFi.localIP().toString());

        state_wifi = true;
        connectToMqtt();
        break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        if(log_wifi) {
          Serial.println("[Wifi] Disconnected from Wi-Fi.");
        }
        state_wifi = false;
        break;
    }
}

void connectToMqtt() {
  if(log_mqtt) {
    Serial.println("[MQTT] Connecting to MQTT...");
  }
  mqttClient.connect();
}

void onMqttConnect(bool sessionPresent) {
  if(log_mqtt) {
    Serial.println("[MQTT] Connected to MQTT.");
    Serial.print("[MQTT] Session present: ");
    Serial.println(sessionPresent);
  }

  state_mqtt = true;
  first_mqtt_connect = true;

  if(DEVICE_TYPE == "BED") {
    mqttClient.subscribe(MQTT_PUB_EMERGENCY.c_str(), 1);
    mqttClient.subscribe(MQTT_PUB_INFUS.c_str(), 1);
    mqttClient.subscribe(MQTT_PUB_ASSIST.c_str(), 1);
  } 
  else if(DEVICE_TYPE == "TOILET") {
    mqttClient.subscribe(MQTT_PUB_EMERGENCY.c_str(), 1);
  }

  mqttClient.subscribe("ping", 1);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {

  if(log_mqtt) {
    Serial.println("[MQTT] Disconnected from MQTT.");
  }

  state_mqtt = false;
  first_mqtt_connect = false;
  init_bed_toilet = true;

  if(state_wifi) {
    connectToMqtt();
  } else {
    connectToWifi();
  }

}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  if(log_mqtt) {
    Serial.println("[MQTT] Subscribe acknowledged.");
    Serial.print("[MQTT] packetId: ");
    Serial.println(packetId);
    Serial.print("[MQTT] qos: ");
    Serial.println(qos);
  }
}

void onMqttUnsubscribe(uint16_t packetId) {
  if(log_mqtt) {
    Serial.println("[MQTT] Unsubscribe acknowledged.");
    Serial.print("[MQTT] packetId: ");
    Serial.println(packetId);
  }
}

void onMqttPublish(uint16_t packetId) {
  if(log_mqtt) {
    Serial.print("[MQTT] Publish acknowledged.");
    Serial.print("[MQTT] packetId: ");
    Serial.println(packetId);
  }
}


void publish(String topic, String payload) {
  uint16_t packetIdPub1 = mqttClient.publish(topic.c_str(), 1, true, payload.c_str());
  if(log_mqtt) {
    Serial.printf("[MQTT] Publishing on topic %s at QoS 1, packetId: %i\n", topic, packetIdPub1);
    Serial.printf("[MQTT] Message: %s \n", payload);
  }
}

void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {

  if(init_bed_toilet) {
    time_init_bed_toilet = millis();
    return;
  }

  if(log_mqtt) {
    Serial.println("[MQTT] Publish received.");
    Serial.print("[MQTT] topic: ");
    Serial.println(topic);
    Serial.print("[MQTT] payload: ");
    Serial.println(payload[0]);
  }

  if(DEVICE_TYPE == "BED") {
    if(millis() - timer_setelah_klik > 6000) {

      // ==== CLEAR (payload 'x' atau 'c') ====
      if (payload[0] == 'x' || payload[0] == 'c') {

        // INFUS clear
        if (topic[0] == 'i') {
          if (state_tombol == 1) state_tombol = 0;
          else if (state_tombol == 4) state_tombol = 2;
          else if (state_tombol == 5) state_tombol = 3;
          else if (state_tombol == 7) state_tombol = 6;
        }

        // CODE BLUE / EMERGENCY clear
        if (topic[0] == 'b') {
          if (state_tombol == 2) state_tombol = 0;
          else if (state_tombol == 4) state_tombol = 1;
          else if (state_tombol == 6) state_tombol = 3;
          else if (state_tombol == 7) state_tombol = 5;
        }

        // ASSIST clear
        if (topic[0] == 'a') {
          if (state_tombol == 3) state_tombol = 0;
          else if (state_tombol == 5) state_tombol = 1;
          else if (state_tombol == 6) state_tombol = 2;
          else if (state_tombol == 7) state_tombol = 4;
        }

        Serial.print("state tombol = ");
        Serial.println(state_tombol);
        writeFile("/state.txt", String(state_tombol).c_str());

      }

    }
  }
  
  else if(DEVICE_TYPE == "TOILET") {
    if(millis() - timer_setelah_klik > 6000) {
      // ==== CLEAR (payload 'x' atau 'c') ====
      if (payload[0] == 'x' || payload[0] == 'c') {

        // CODE BLUE / EMERGENCY clear
        if (topic[0] == 'b') {
          if (state_tombol == 2) state_tombol = 0;
          else if (state_tombol == 4) state_tombol = 1;
          else if (state_tombol == 6) state_tombol = 3;
          else if (state_tombol == 7) state_tombol = 5;
        }

        Serial.print("state tombol = ");
        Serial.println(state_tombol);
        writeFile("/state.txt", String(state_tombol).c_str());

      }
    }
  }

  else if(DEVICE_TYPE == "LAMPP") {
    if(payload[0] == 'c' || payload[0] == 'x'){
      deleteTopic(topic);
    } else {
      if(payload[0] != '1' && payload[0] != 's'){
        if(log_mqtt) {
          Serial.println("=======================");
          Serial.println("NEW DATA");
        }
        if(payload[0] != 'p') {
          pushTopic(topic, payload[0]);
          lampp_newData = true;
        }
      }
    }
  }

  if(topic[0] == 'p' && topic[1] == 'i' && topic[2] == 'n' && topic[3] == 'g') {
    if(log_mqtt) {
      Serial.println("[MQTT] ADA PING MASUK");
    }
    timer_ping = millis();
  }

}


void modeAP() {
  mode_ap = true;
  waiting_to_setup = false;
  timeout_ap = millis();

  bleServer.start();

  // WiFi.disconnect(true);
  // WiFi.softAP("");

  // IPAddress IP = WiFi.softAPIP();
  // if(log_setup) {
  //   Serial.print("[SETUP] AP IP address: ");
  //   Serial.println(IP);
  // }
}

void onEmergencyClicked() {
  if (waiting_to_setup) {
    return;
  }

  if (init_bed_toilet) {
    return;
  }

  if (log_device) {
    Serial.println("[DEVICE] BTN EMERGENCY CLICKED");
  }


  // update kombinasi berdasarkan state sekarang
  if (state_tombol == 0) state_tombol = 2;
  else if (state_tombol == 1) state_tombol = 4;
  else if (state_tombol == 3) state_tombol = 6;
  else if (state_tombol == 5) state_tombol = 7;

  publish(MQTT_PUB_EMERGENCY, "e");
  activity = true;

  Serial.print("state tombol = ");
  Serial.println(state_tombol);
  writeFile("/state.txt", String(state_tombol).c_str());
}

void onAssistClicked() {
  if (init_bed_toilet) {
    return;
  }

  if (log_device) {
    Serial.println("[DEVICE] BTN ASSIST CLICKED");
  }

  if (state_tombol == 0) state_tombol = 3;
  else if (state_tombol == 1) state_tombol = 5;
  else if (state_tombol == 2) state_tombol = 6;
  else if (state_tombol == 4) state_tombol = 7;

  publish(MQTT_PUB_ASSIST, "a");
  activity = true;

  Serial.print("state tombol = ");
  Serial.println(state_tombol);
  writeFile("/state.txt", String(state_tombol).c_str());
}

void onInfusClicked() {
  if (init_bed_toilet) {
    return;
  }

  if (log_device) {
    Serial.println("[DEVICE] BTN INFUS CLICKED");
  }

  if (state_tombol == 0) state_tombol = 1;
  else if (state_tombol == 2) state_tombol = 4;
  else if (state_tombol == 3) state_tombol = 5;
  else if (state_tombol == 6) state_tombol = 7;

  publish(MQTT_PUB_INFUS, "i");
  activity = true;

  Serial.print("state tombol = ");
  Serial.println(state_tombol);
  writeFile("/state.txt", String(state_tombol).c_str());
}

void onCancelClicked() {
  if (init_bed_toilet) {
    return;
  }

  if (waiting_to_setup) {
    return;
  }

  if (log_device) {
    Serial.println("[DEVICE] BTN CANCEL CLICKED");
  }

  // Reset semua topik
  if (DEVICE_TYPE == "BED") {
    publish(MQTT_PUB_INFUS, "c");
    publish(MQTT_PUB_ASSIST, "c");
    publish(MQTT_PUB_EMERGENCY, "c");
  } 
  else if (DEVICE_TYPE == "TOILET") {
    publish(MQTT_PUB_EMERGENCY, "c");
  }

  activity = false;

  // Reset state
  state_tombol = 0;
  Serial.print("state tombol = ");
  Serial.println(state_tombol);
  writeFile("/state.txt", String(state_tombol).c_str());
}
void on30sClicked() {
  waiting_to_setup = true;
  digitalWrite(BUZZER, 1);
  delay(500);
  if(log_setup) {
    Serial.println("[SETUP] waiting to setup true");
  }
  timeout_ap = millis();
}

void on10sClicked() {
  if(waiting_to_setup) {
    digitalWrite(BUZZER, 1);
    delay(500);

    if(log_setup) {
      Serial.println("[SETUP] mode ap true");
    }
    modeAP();
  }
}

String scanNetwork() {
  int n = WiFi.scanNetworks();
  String result = "[";
  for (int i = 0; i < n; ++i) {
    // Print SSID and RSSI for each network found
    result += "{\"SSID\":\"";
    result += WiFi.SSID(i);
    result += "\",\"RSSI\":";
    result += WiFi.RSSI(i);
    result += "}";
    if (i != n - 1) {
      result += ",";
    }
  }
  result += "]";

  return result;
}

void writeFile(const char *path, const char *message) {
  if(log_setup) {
    Serial.printf("[SETUP] Writing file: %s : ", path);
    Serial.println(message);
  }
  File file = LittleFS.open(path, "w");
  if (!file) {
    if(log_setup) {
      Serial.println("[SETUP] Failed to open file for writing");
    }
    return;
  }
  if (file.print(message)) {
    if(log_setup) {
      Serial.println("[SETUP] File written");
    }
  } else {
    if(log_setup) {
      Serial.println("[SETUP] Write failed");
    }
  }
  delay(500);  // Make sure the CREATE and LASTWRITE times are different
  file.close();
}

String readFile(const char *path) {
  if(log_setup) {
    Serial.printf("[SETUP] Reading file: %s", path);
  }

  File file = LittleFS.open(path, "r");
  if (!file) {
    if(log_setup) {
      Serial.println("[SETUP] Failed to open file for reading");
    }
    // return;
  }

  String data = file.readString();
  if(log_setup) {
    Serial.println(data);
  }
  file.close();
  return data;
}


// LAMPP FUNCTION
//////////////////////////////////////////////////////////////

void printTopic(){
  if(log_device){
    Serial.print("[LAMPP] Active Topic : ");
  }
  for(int a = 0; a < lampp_countTopic; a++){
    if(log_device) {
      Serial.print(lampp_activeTopic[a]); Serial.print(lampp_valTopic[a]); Serial.print(", ");
    }
  }
  if(log_device) {
    Serial.println();
    Serial.print("[LAMPP] COUNT : "); Serial.println(lampp_countTopic);
  }
}

void pushTopic(String topic, char val){
  for(int a = 0; a < lampp_countTopic; a++){
    if(topic == lampp_activeTopic[a]){
      return;
    }
  }
  lampp_activeTopic[lampp_countTopic] = topic;
  lampp_valTopic[lampp_countTopic] = val;
  lampp_countTopic++;
  printTopic();
}

void deleteTopic(String topic){
  for(int a = 0; a < lampp_countTopic; a++){
    if(lampp_activeTopic[a] == topic){
      if(lampp_valTopic[a] == 'e') digitalWrite(LAMPP_LED_merah, 1);
      if(lampp_valTopic[a] == 'a') digitalWrite(LAMPP_LED_kuning, 1);
      if(lampp_valTopic[a] == 'i') digitalWrite(LAMPP_LED_hijau, 1);
      if(lampp_valTopic[a] == 'b') digitalWrite(LAMPP_LED_biru, 1);

      for(int j = a; j < lampp_countTopic; j++){
        lampp_activeTopic[a] = lampp_activeTopic[a+1];
        lampp_valTopic[a] = lampp_valTopic[a+1];
      }
      lampp_countTopic--;
      break;
    }
  }
  // printTopic();
}
//////////////////////////////////////////////////////


void setup() {
  // put your setup code here, to run once:
  Serial.begin(112500);

  if(!LittleFS.begin(true)){
      Serial.println("LittleFS Mount Failed");
      return;
  }

  ssid = ssid.isEmpty() ? readFile("/ssid.txt") : ssid;
  password = password.isEmpty() ? readFile("/password.txt") : password;
  id = id.isEmpty() ? readFile("/id.txt") : id;
  DEVICE_TYPE = DEVICE_TYPE.isEmpty() ? readFile("/device.txt") : DEVICE_TYPE;
  STATIC_IP = STATIC_IP.isEmpty() ? readFile("/ip.txt") : STATIC_IP;
  String s = readFile("/state.txt");
  if (s.length() > 0) {
    state_tombol = s.toInt();
  } else {
    state_tombol = state_tombol; // tetap nilai default
  }

  Serial.print("state tombol = ");
  Serial.println(state_tombol);

  // int analogDeviceType = analogRead(ANALOGPIN);

  // if (analogDeviceType < 255){
  //   DEVICE_TYPE = "LAMPP";
  // } else if (analogDeviceType > 750) {
  //   DEVICE_TYPE = "BED";
  // } else {
  //   DEVICE_TYPE = "TOILET";
  // }


  if (DEVICE_TYPE == "BED") {

    pinMode(BED_LED_CANCEL, OUTPUT);

    btn_assist.begin();
    btn_infus.begin();
    btn_emergency.begin();
    btn_cancel.begin();

    btn_assist.onValidClick(onAssistClicked);
    btn_infus.onValidClick(onInfusClicked);
    btn_emergency.onValidClick(onEmergencyClicked);
    btn_cancel.onValidClick(onCancelClicked);
    btn_cancel.onHold30s(on30sClicked);
    btn_emergency.onHold10s(on10sClicked);

    btn_setup.begin();
    btn_setup.onValidClick(modeAP);

    MQTT_PUB_EMERGENCY = "bed/" + id;
    MQTT_PUB_INFUS = "infus/" + id;
    MQTT_PUB_ASSIST = "assist/" + id;

  } else if (DEVICE_TYPE == "TOILET") {

    pinMode(BED_LED_CANCEL, OUTPUT);

    btn_emergency.begin();
    btn_cancel.begin();
    btn_emergency.onValidClick(onEmergencyClicked);
    btn_cancel.onValidClick(onCancelClicked);
    btn_cancel.onHold30s(on30sClicked);
    btn_emergency.onHold10s(on10sClicked);

    btn_setup.begin();
    btn_setup.onValidClick(modeAP);

    MQTT_PUB_EMERGENCY = "toilet/" + id;

  } else if (DEVICE_TYPE == "LAMPP") {

    btn_setup.begin();
    btn_setup.onValidClick(modeAP);

    pinMode(LAMPP_LED_merah, OUTPUT);
    pinMode(LAMPP_LED_kuning, OUTPUT);
    pinMode(LAMPP_LED_hijau, OUTPUT);
    pinMode(LAMPP_LED_biru, OUTPUT);

  }  else {
    
    btn_setup.begin();
    btn_setup.onValidClick(modeAP);

  }

  if(ssid == ""){
    ssid = "tidak boleh kosong";
  }

  // Menghapus konfigurasi WiFi sebelumnya
  WiFi.disconnect(true);

   

  WiFi.onEvent(WiFiEvent);

  if(STATIC_IP != "") {
    if(log_wifi) {
      Serial.print("[WiFi] STATIC CONFIGURE : ");
      Serial.println(STATIC_IP.toInt());
    }
    // Set your Static IP address
    IPAddress local_IP(192, 168, 0, STATIC_IP.toInt());
    // Set your Gateway IP address
    IPAddress gateway(192, 168, 0, 254);

    IPAddress subnet(255, 255, 255, 0);
    IPAddress primaryDNS(8, 8, 8, 8);   //optional
    IPAddress secondaryDNS(8, 8, 4, 4);

    if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
      Serial.println("STA Failed to configure");
    }

  }

  // Memulai koneksi WiFi
  connectToWifi();

  // MQTT
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);



  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", index_html);
  });

  server.on("/restart", HTTP_GET, [](AsyncWebServerRequest *request) {
    ESP.restart();
  });

  // Send a GET request to <IP>/get?message=<message>
  server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", "{\"ssid\": \"" + ssid + "\", \"password\": \"" + password + "\", \"id\": \"" + id + "\",\"device\": \"" + DEVICE_TYPE + "\",\"nursestation\":\""+ NURSESTATION +"\", \"ip\":\""+ STATIC_IP +"\" }");
  });

  server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request) {
    trigger_scan_network = true;
    request->send(200, "application/json", hasil_scan_network);
    // request->send(200, "text/plain", "[]");
  });

  server.on("/wifi", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("ssid", true)) {
      String message = request->getParam("ssid", true)->value();
      ssid = message;
      if(log_setup){
        Serial.print("[SETUP] SSID : ");
        Serial.println(message);
      }
      writeFile("/ssid.txt", message.c_str());
    }
    if (request->hasParam("password", true)) {
      String message = request->getParam("password", true)->value();
      password = message;
      if(log_setup) {
        Serial.print("[SETUP] PASS : ");
        Serial.println(message);
      }
      writeFile("/password.txt", message.c_str());
    }
    if (request->hasParam("id", true)) {
      String message = request->getParam("id", true)->value();
      id = message;
      if(log_setup) {
        Serial.print("[SETUP] ID : ");
        Serial.println(message);
      }
      writeFile("/id.txt", message.c_str());
    }
    if (request->hasParam("device", true)) {
      String message = request->getParam("device", true)->value();
      DEVICE_TYPE = message;
      if(log_setup) {
        Serial.print("[SETUP] DEVICE_TYPE : ");
        Serial.println(message);
      }
      writeFile("/device.txt", message.c_str());
    }
    if (request->hasParam("nursestation", true)) {
      String message = request->getParam("nursestation", true)->value();
      NURSESTATION = message;
      if(log_setup) {
        Serial.print("[SETUP] NURSESTATION : ");
        Serial.println(message);
      }
      writeFile("/nursestation.txt", message.c_str());
    }
    if (request->hasParam("ip", true)) {
      String message = request->getParam("ip", true)->value();
      STATIC_IP = message == "" ? "" : message;
      if(log_setup) {
        Serial.print("[SETUP] STATIC_IP : ");
        Serial.println(message);
      }
      writeFile("/ip.txt", message.c_str());
    }
    request->send(200, "text/html", saved);
  });

  server.begin();
  
  bleServer.setLogging(true);
  bleServer.setValue("SSID", ssid);
  bleServer.setValue("Password", password);
  bleServer.setValue("ID", id);
  bleServer.setValue("IP", STATIC_IP);
  bleServer.setValue("Device Type", DEVICE_TYPE);
  bleServer.setValue("IP Current", WiFi.localIP().toString());
  bleServer.setValue("Mac Addr", WiFi.macAddress());

  // Callback Flutter
  bleServer.onCommand = [](const char* value) {
    Serial.printf("📩 Data dari Flutter: %s\n", value);

    if(strcmp(value, "get") == 0) {
      bleServer.sendDataDevice(true);

    } else if(strcmp(value, "buzzer") == 0) {
      digitalWrite(BUZZER, 0);
      delay(500);
      digitalWrite(BUZZER, 1);

    } else if (strcmp(value, "led") == 0) {
      pinMode(BED_LED_CANCEL, OUTPUT);
      digitalWrite(BED_LED_CANCEL, 1);
      delay(500);
      digitalWrite(BED_LED_CANCEL, 0);

    } else if (strcmp(value, "reboot") == 0) {
      ESP.restart();

    } else if (strcmp(value, "scan-wifi") == 0) {

      WiFi.disconnect(true);  // pastikan tidak sedang terhubung ke AP mana pun
      delay(500);

      Serial.println("Scanning WiFi...");
      int n = WiFi.scanNetworks();
      Serial.println(n);

      // Buat dokumen JSON
      StaticJsonDocument<1024> doc;
      JsonArray wifiArray = doc.createNestedArray("wifi");

      for (int i = 0; i < n; ++i) {
        JsonObject wifiObj = wifiArray.createNestedObject();
        wifiObj[WiFi.SSID(i)] = String(WiFi.RSSI(i)); // { "SSID": "-XX" }
        if(log_setup) Serial.println(WiFi.SSID(i));
      }

      // Serialize ke string
      String jsonStr;
      serializeJson(doc, jsonStr);

      bleServer.sendMessage(jsonStr);

    } else {
      StaticJsonDocument<200> doc;

      // Parse string JSON
      DeserializationError error = deserializeJson(doc, value);

      // Cek error parsing
      if (error) {
        Serial.print("Gagal parse JSON: ");
        Serial.println(error.f_str());
        return;
      }

      const char* key = doc["k"].as<const char*>();

    
      if(strcmp(key, "ID") == 0) {
        if(log_setup){
          Serial.print("[SETUP] ID : ");
          Serial.println(doc["v"].as<const char*>());
        }
        writeFile("/id.txt", doc["v"].as<const char*>());
      }
      if(strcmp(key, "Device Type") == 0) {
        if(log_setup){
          Serial.print("[SETUP] DEVICE_TYPE : ");
          Serial.println(doc["v"].as<const char*>());
        }
        writeFile("/device.txt", doc["v"].as<const char*>());
      }
      if(strcmp(key, "SSID") == 0) {
        if(log_setup){
          Serial.print("[SETUP] SSID : ");
          Serial.println(doc["v"].as<const char*>());
        }
        writeFile("/ssid.txt", doc["v"].as<const char*>());
      }
      if(strcmp(key, "Password") == 0) {
        if(log_setup){
          Serial.print("[SETUP] PASS : ");
          Serial.println(doc["v"].as<const char*>());
        }
        writeFile("/password.txt", doc["v"].as<const char*>());
      }
      if(strcmp(key, "IP") == 0) {
        if(log_setup){
          Serial.print("[SETUP] IP : ");
          Serial.println(doc["v"].as<const char*>());
        }
        writeFile("/ip.txt", doc["v"].as<const char*>());
      }
      

    }

  };

}

void loop() {
  // put your main code here, to run repeatedly:

  if(DEVICE_TYPE == "BED") {
    ONEWAY_DEVICE();
  } 

  else if(DEVICE_TYPE == "TOILET") {
    TOILET_DEVICE();
  }
  
  else if (DEVICE_TYPE == "LAMPP") {
    LAMPP();
  } 

  btn_setup.update();
  

  if (state_mqtt == false) {
    time_init_bed_toilet = millis();
  }

  if (state_mqtt && ((millis() - time_init_bed_toilet) > 3000) && init_bed_toilet) {
    digitalWrite(BED_LED_CANCEL, 1);
    delay(1000);
    digitalWrite(BED_LED_CANCEL, 0);
    init_bed_toilet = false;
  }


  if(trigger_scan_network) {
    hasil_scan_network = scanNetwork();
    trigger_scan_network = false;
  }

  if(mode_ap) {
    if (millis() - time_buzzer_ap > 2000) {
      state_buzzer_ap = !state_buzzer_ap;
      digitalWrite(BUZZER, state_buzzer_ap);
      time_buzzer_ap = millis();

      // update wifi strength
      bleServer.setValue("WiFi Strength", String(WiFi.RSSI()));
    }

    if(millis() - timeout_ap > 300000) {
      ESP.restart();
    }

  }

  if(waiting_to_setup) {
    if(millis() - timeout_ap > 20000){
      digitalWrite(BUZZER, 0);
      delay(500);
      digitalWrite(BUZZER, 1);
      delay(500);
      digitalWrite(BUZZER, 0);
      delay(500);
      digitalWrite(BUZZER, 1);
      mode_ap = false;
      waiting_to_setup = false;
    }
  }


  // RESET BY DISCONNECTED WIFI
  if(state_wifi == false && !mode_ap) {

    if(millis() - state_wifi_timer > 60000) {
      Serial.println("RESET BY DISCONNECTED WIFI");
      delay(2000);
      ESP.restart();
    }

  } else {
    state_wifi_timer = millis();
  }

  // RESET BY DISCONNECTED MQTT
  if(state_mqtt == false && !mode_ap) {

    if(millis() - state_mqtt_timer > 60000) {
      Serial.println("RESET BY DISCONNECTED MQTT");
      delay(2000);
      ESP.restart();
    }

  } else {
    state_mqtt_timer = millis();
  }


  // RESET PERIODIC
  if(millis() - reset_periodic_timer > 86400000) {
    Serial.println("RESET PERIODIC");
    if(activity == false) {
      if(millis() - time_reset_setelah_activity > 15000) {
        WiFi.disconnect(true);
        delay(1000);
        ESP.restart();
      }
    } else {
      time_reset_setelah_activity = millis();
    }
  }

  // RESET BY PING
  if(millis() - timer_ping > 120000) {
    if(!mode_ap) {
      Serial.println("RESET BY PING");
      WiFi.disconnect(true);
      delay(1000);
      ESP.restart();
    }
  }

}

void ONEWAY_DEVICE() {
  btn_assist.update();
  btn_infus.update();
  btn_emergency.update();
  btn_cancel.update();

  if (state_tombol != 0) {
    state_led = true;
  } else {
    state_led = false;
  }

  if(state_led){
    if(millis() - state_led_timer > 500){
      digitalWrite(BED_LED_CANCEL, 1);
      delay(100);
      digitalWrite(BED_LED_CANCEL, 0);
      state_led_timer = millis();
    }
  }

  if(state_mqtt && init_bed_toilet == false) {
    if(millis() - timer_device_activation > 30000 ) {
      // mqttClient.subscribe(MQTT_PUB_EMERGENCY.c_str(), 1);
      // mqttClient.subscribe(MQTT_PUB_ASSIST.c_str(), 1);
      // mqttClient.subscribe(MQTT_PUB_INFUS.c_str(), 1);
      mqttClient.publish("aktif", 0, false, id.c_str());

      
      // Default semua mati
      String payload_infus = "c";
      String payload_emergency = "c";
      String payload_assist = "c";

      // Cek kombinasi aktif
      if (state_tombol == 1) {             // Infus
        payload_infus = "i";
      } else if (state_tombol == 2) {      // Emergency
        payload_emergency = "e";
      } else if (state_tombol == 3) {      // Assist
        payload_assist = "a";
      } else if (state_tombol == 4) {      // Infus + Emergency
        payload_infus = "i";
        payload_emergency = "e";
      } else if (state_tombol == 5) {      // Infus + Assist
        payload_infus = "i";
        payload_assist = "a";
      } else if (state_tombol == 6) {      // Emergency + Assist
        payload_emergency = "e";
        payload_assist = "a";
      } else if (state_tombol == 7) {      // Semua aktif
        payload_infus = "i";
        payload_emergency = "e";
        payload_assist = "a";
      }

      // Kirim payload
      publish(MQTT_PUB_INFUS, payload_infus.c_str());
      publish(MQTT_PUB_EMERGENCY, payload_emergency.c_str());
      publish(MQTT_PUB_ASSIST, payload_assist.c_str());

      timer_device_activation = millis();
    }
  }

  if(state_mqtt && state_mqtt_first){
    digitalWrite(BED_LED_CANCEL, 1);
    delay(100);
    digitalWrite(BED_LED_CANCEL, 0);
    delay(100);
    digitalWrite(BED_LED_CANCEL, 1);
    delay(100);
    digitalWrite(BED_LED_CANCEL, 0);
    state_mqtt_first = false;

    if (id != "") {
      String protocol = "http://";
      String endpoint = "/ip-call/server/bed/set_ip.php?id=";
      String ip = WiFi.localIP().toString();
      String server = protocol + MQTT_HOST + endpoint + id + "&ip=" + ip;
      httpGETRequest(server);
    }

  }
}


void TOILET_DEVICE() {
  btn_emergency.update();
  btn_cancel.update();

   if (state_tombol != 0) {
    state_led = true;
  } else {
    state_led = false;
  }

  if(state_led){
    if(millis() - state_led_timer > 500){
      digitalWrite(BED_LED_CANCEL, 1);
      delay(100);
      digitalWrite(BED_LED_CANCEL, 0);
      state_led_timer = millis();
    }
  }

  if(state_mqtt && init_bed_toilet == false) {
    if(millis() - timer_device_activation > 30000 ) {
      mqttClient.subscribe(MQTT_PUB_EMERGENCY.c_str(), 1);
      mqttClient.publish("aktif", 0, false, id.c_str());

      // Default semua mati
      String payload_emergency = "c";

      // Cek kombinasi aktif
      if (state_tombol == 2) {      // Emergency
        payload_emergency = "e";
      } else if (state_tombol == 4) {      // Infus + Emergency
        payload_emergency = "e";
      } else if (state_tombol == 6) {      // Emergency + Assist
        payload_emergency = "e";
      } else if (state_tombol == 7) {    
        payload_emergency = "e";
      }

      // Kirim payload
      publish(MQTT_PUB_EMERGENCY, payload_emergency.c_str());

      timer_device_activation = millis();
    }
  }

  if(state_mqtt && state_mqtt_first){
    digitalWrite(BED_LED_CANCEL, 1);
    delay(100);
    digitalWrite(BED_LED_CANCEL, 0);
    delay(100);
    digitalWrite(BED_LED_CANCEL, 1);
    delay(100);
    digitalWrite(BED_LED_CANCEL, 0);
    state_mqtt_first = false;

    if (id != "") {
      String protocol = "http://";
      String endpoint = "/ip-call/server/bed/set_ip.php?id=";
      String ip = WiFi.localIP().toString();
      String server = protocol + MQTT_HOST + endpoint + id + "&ip=" + ip;
      httpGETRequest(server);
    }

  }

}


void LAMPP() {

  if(lampp_newData) {
    if(lampp_valTopic[lampp_countTopic - 1] == 'e' && lampp_activeTopic[lampp_countTopic - 1][0] == 'b') {

      String id_temp = lampp_activeTopic[lampp_countTopic - 1].substring(4);

      String endpoint = "/ip-call/server/bed/get_one.php?id=";
      String protocol = "http://";
      String server = protocol + MQTT_HOST + endpoint + id_temp;
      // Serial.println(server);
      String res = httpGETRequest(server);

      deserializeJson(doc, res);

      JsonArray array = doc["data"].as<JsonArray>();
      for(JsonObject v : array) {
          String mode = v["mode"].as<String>();
          
          if(mode == "2") {
            lampp_valTopic[lampp_countTopic - 1] = 'b';
          }
      }
    }

    lampp_newData = false;
  }

  if(lampp_countTopic > 0){
    if(millis() - lampp_before > 300){
      if(lampp_valTopic[lampp_index_saat_ini] == 'e') {
        digitalWrite(LAMPP_LED_merah, lampp_state);
      } else if (lampp_valTopic[lampp_index_saat_ini] == 'b') {
        digitalWrite(LAMPP_LED_biru, lampp_state);
      } else if (lampp_valTopic[lampp_index_saat_ini] == 'i') {
        digitalWrite(LAMPP_LED_kuning, lampp_state);
      } else if (lampp_valTopic[lampp_index_saat_ini] == 'a') {
        digitalWrite(LAMPP_LED_hijau, lampp_state);
      }
      lampp_state = !lampp_state;

      if(!lampp_state) {
        lampp_index_saat_ini ++;
        if (lampp_index_saat_ini >= lampp_countTopic) lampp_index_saat_ini = 0;
      }

      lampp_before = millis();
    }
    activity = true;
  } else {
    digitalWrite(LAMPP_LED_merah, 1);
    digitalWrite(LAMPP_LED_biru, 1);
    digitalWrite(LAMPP_LED_hijau, 1);
    digitalWrite(LAMPP_LED_kuning, 1);
    activity = false;
  }

  if(state_mqtt && state_mqtt_first){
    digitalWrite(LAMPP_LED_merah, 1);
    delay(100);
    digitalWrite(LAMPP_LED_merah, 0);
    delay(100);
    digitalWrite(LAMPP_LED_merah, 1);
    delay(100);
    digitalWrite(LAMPP_LED_merah, 0);
    state_mqtt_first = false;

    if (id != "") {
      String protocol = "http://";
      String endpoint = "/ip-call/server/bed/set_ip.php?id=";
      String ip = WiFi.localIP().toString();
      String server = protocol + MQTT_HOST + endpoint + id + "&ip=" + ip;
      httpGETRequest(server);
    }

  }

  if(millis() - timer_device_activation > 5000) {
    subscribeRoom();
    mqttClient.publish("aktif", 0, false, id.c_str());
    timer_device_activation = millis();
  }
}

String httpGETRequest(String serverName) {
  WiFiClient client;
  HTTPClient http;
    
  http.begin(client, serverName.c_str());
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}

void subscribeRoom() {

  String endpoint = "/ip-call/server/toilet/get_room.php?id=";
  String protocol = "http://";
  String server = protocol + MQTT_HOST + endpoint + id;
  // Serial.println(server);
  String res = httpGETRequest(server);

  deserializeJson(doc, res);

  JsonArray array = doc["data"].as<JsonArray>();
  for(JsonObject v : array) {
      String topic = String("toilet/") + v["id"].as<String>();
      // Serial.println(topic);
      mqttClient.subscribe(topic.c_str(), 1);
  }


  protocol = "http://";
  endpoint = "/ip-call/server/bed/get.php?id=";
  server = protocol + MQTT_HOST + endpoint + id;
  // Serial.println(server);
  res = httpGETRequest(server);

  deserializeJson(doc, res);
  
  array = doc["data"].as<JsonArray>();
  for(JsonObject v : array) {
      String topic = String("bed/") + v["id"].as<String>();
      // Serial.println(topic);
      mqttClient.subscribe(topic.c_str(), 1);
      topic = String("infus/") + v["id"].as<String>();
      // Serial.println(topic);
      mqttClient.subscribe(topic.c_str(), 1);
      topic = String("stop/") + v["id"].as<String>();
      // Serial.println(topic);
      mqttClient.subscribe(topic.c_str(), 1);
      topic = String("call/") + v["id"].as<String>();
      // Serial.println(topic);
      mqttClient.subscribe(topic.c_str(), 1);
      topic = String("assist/") + v["id"].as<String>();
      // Serial.println(topic);
      mqttClient.subscribe(topic.c_str(), 1);
  }
}
