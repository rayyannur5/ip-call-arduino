#include <ESP8266WiFi.h>
#include <AsyncMqttClient.h>
#include <ESPAsyncWebSrv.h>
#include <LittleFS.h>
#include <ESP8266HTTPClient.h>
#include <FS.h>
#include <ArduinoJson.h>

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


// BUTTON CLASS
typedef void (*FungsiAksi)();

class Tombol {
  private:
    uint8_t _pinTombol;
    uint8_t _pinBuzzer;
    long _debounceDelay;
    
    int _activeState;
    int _inactiveState;

    int _stateTerakhir;
    unsigned long _waktuTekan;
    bool _sedangDitekan;
    bool _buzzerAktif;

    // BARU: Flag untuk memastikan aksi tahan lama hanya terpicu sekali per tekanan
    bool _aksi10sTerpicu;
    bool _aksi30sTerpicu;

    // Kumpulan fungsi callback untuk setiap event
    FungsiAksi _fungsiAksiOnClick = nullptr;
    FungsiAksi _fungsiAksiTahan10s = nullptr; // BARU
    FungsiAksi _fungsiAksiTahan30s = nullptr; // BARU

  public:
    Tombol(uint8_t pinTombol, uint8_t pinBuzzer, int activeState = LOW, long delay = 200) {
      _pinTombol = pinTombol;
      _pinBuzzer = pinBuzzer;
      _debounceDelay = delay;
      _activeState = activeState;
      _inactiveState = (_activeState == HIGH) ? LOW : HIGH;
      _stateTerakhir = _inactiveState;
    }

    void begin() {
      if (_activeState == LOW) pinMode(_pinTombol, INPUT_PULLUP);
      else pinMode(_pinTombol, INPUT);
      pinMode(_pinBuzzer, OUTPUT);
      digitalWrite(_pinBuzzer, HIGH);
    }
    
    // --- Metode untuk Mengaitkan Aksi ---
    void onValidClick(FungsiAksi fungsi) { _fungsiAksiOnClick = fungsi; }
    void onHold10s(FungsiAksi fungsi) { _fungsiAksiTahan10s = fungsi; } // BARU
    void onHold30s(FungsiAksi fungsi) { _fungsiAksiTahan30s = fungsi; } // BARU

    void update() {
      int stateSekarang = digitalRead(_pinTombol);

      // 1. Deteksi saat tombol baru ditekan
      if (_stateTerakhir == _inactiveState && stateSekarang == _activeState) {
        _sedangDitekan = true;
        _waktuTekan = millis();
        // Reset semua status saat tekanan baru dimulai
        _buzzerAktif = false;
        _aksi10sTerpicu = false;
        _aksi30sTerpicu = false;
      }

      // 2. Logika saat tombol sedang ditahan
      if (_sedangDitekan && stateSekarang == _activeState) {
        unsigned long durasiTahan = millis() - _waktuTekan;

        // Aktifkan buzzer setelah debounce delay
        if (!_buzzerAktif && durasiTahan >= _debounceDelay) {
          digitalWrite(_pinBuzzer, LOW);
          _buzzerAktif = true;
        }

        // Cek untuk aksi tahan 10 detik
        if (!_aksi10sTerpicu && durasiTahan >= 7000) {
          if (_fungsiAksiTahan10s != nullptr) {
            // Serial.print("[Tombol di Pin "); Serial.print(_pinTombol); Serial.println("]: Aksi TAHAN 10 DETIK terpicu!");
            _fungsiAksiTahan10s();
          }
          _aksi10sTerpicu = true; // Set flag agar tidak terpicu lagi
        }
        
        // Cek untuk aksi tahan 30 detik
        if (!_aksi30sTerpicu && durasiTahan >= 15000) {
          if (_fungsiAksiTahan30s != nullptr) {
            // Serial.print("[Tombol di Pin "); Serial.print(_pinTombol); Serial.println("]: Aksi TAHAN 30 DETIK terpicu!");
            _fungsiAksiTahan30s();
          }
          _aksi30sTerpicu = true; // Set flag agar tidak terpicu lagi
        }
      }

      // 3. Deteksi saat tombol dilepaskan
      if (_stateTerakhir == _activeState && stateSekarang == _inactiveState) {
        if (_sedangDitekan) { // Pastikan ini adalah akhir dari sebuah tekanan
          long durasiTekan = millis() - _waktuTekan;
          
          // Aksi klik singkat HANYA berjalan jika tidak ada aksi tahan lama yang terpicu
          if (!_aksi10sTerpicu && !_aksi30sTerpicu) {
            if (durasiTekan >= _debounceDelay) {
              // Serial.print("[Tombol di Pin "); Serial.print(_pinTombol); Serial.println("]: Aksi KLIK SINGKAT valid.");
              if (_fungsiAksiOnClick != nullptr) {
                _fungsiAksiOnClick();
              }
            } else {
              // Serial.print("[Tombol di Pin "); Serial.print(_pinTombol); Serial.println("]: Aksi diabaikan (terlalu singkat).");
            }
          } else {
            // Serial.print("[Tombol di Pin "); Serial.print(_pinTombol); Serial.println("]: Aksi tahan lama selesai.");
          }

          // Selalu reset saat tombol dilepas
          _sedangDitekan = false;
          digitalWrite(_pinBuzzer, HIGH);
        }
      }

      _stateTerakhir = stateSekarang;
    }
};


// LOG LEVEL
bool log_wifi = true;
bool log_mqtt = true;
bool log_device = true;
bool log_setup = true;


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

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;

DynamicJsonDocument doc(1024);

////////////////////////////////////////////////

#define BUZZER    2
#define ANALOGPIN A0
#define BTN_SETUP D1

Tombol btn_setup(BTN_SETUP, BUZZER);

// LAMPP VARIABLE
////////////////////////////////////////////////

#define LAMPP_LED_merah 	D6 
#define LAMPP_LED_hijau 	D3
#define LAMPP_LED_biru	  D2
#define LAMPP_LED_kuning 	D7
#define LAMPP_BTN_SETUP 	14 // D5

// // LAMPP DEMO 1
// #define LAMPP_LED_merah   D5
// #define LAMPP_LED_hijau   D7
// #define LAMPP_LED_biru    D2
// #define LAMPP_LED_kuning  D7
// #define LAMPP_BTN_SETUP   D5

// LAMPP DEMO 2
// #define LAMPP_LED_merah   D5
// #define LAMPP_LED_hijau   D5
// #define LAMPP_LED_biru    D5
// #define LAMPP_LED_kuning  D5
// #define LAMPP_BTN_SETUP   D5


String lampp_activeTopic[20];
char lampp_valTopic[20];
int lampp_countTopic = 0;
bool lampp_newData = false;
u_long lampp_before = 0;
bool lampp_state = false;
int lampp_index_saat_ini = 0;


// BED AND TOILET VARIABLE
///////////////////////////////////////////////


// // BED DEMO
// #define BED_PIN_ASSIST D2
// #define BED_PIN_INFUS D6
// #define BED_PIN_EMERGENCY D7
// #define BED_PIN_CANCEL D5
// #define BED_LED_CANCEL D3

#define BED_PIN_ASSIST 4 // D2
#define BED_PIN_INFUS 12 // D6
#define BED_PIN_EMERGENCY 13 // D7
#define BED_PIN_CANCEL 14 // D5
#define BED_LED_CANCEL 0 // D3

Tombol btn_assist(BED_PIN_ASSIST, BUZZER);
Tombol btn_infus(BED_PIN_INFUS, BUZZER);
Tombol btn_emergency(BED_PIN_EMERGENCY, BUZZER, LOW, 0);
Tombol btn_cancel(BED_PIN_CANCEL, BUZZER);

String MQTT_PUB_EMERGENCY;
String MQTT_PUB_INFUS;
String MQTT_PUB_ASSIST;

int x_server_topic_emergency = 0;
int x_server_topic_infus = 0;
int x_server_topic_assist = 0;

int clear_dari_server = 0;
u_long timer_setelah_klik = 0;

bool state_led = false;
u_long state_led_timer = 0;

bool waiting_to_setup = false;
u_long waiting_to_setup_timer = 0;
u_long time_buzzer_ap = 0;
bool state_buzzer_ap = false;
u_long timeout_ap = 0;
///////////////////////////////////////////////


const char saved[] PROGMEM = R"rawliteral(
  <!DOCTYPE HTML><html>
  <head>
    <title>ESP Web Server</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
  </head>
  <body>
    <h2>SAVED</h2>
  <script>
    setTimeout(() => {
      window.location.replace("/");
    }, 3000);
  </script>
  </body>
  </html>
  )rawliteral";

  const char index_html[] PROGMEM = R"rawliteral(
  <!DOCTYPE HTML><html>
  <head>
    <title>ESP Web Server</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
  </head>
  <body>
    <h2 style="text-align: center; color: #2c3e50;">ESP Web Server</h2>

    <div id="realtime" style="margin: 10px auto; padding: 10px; text-align: center; font-size: 18px;"></div>

    <div style="display: flex; justify-content: center; gap: 10px; margin-bottom: 20px;">
      <!-- <button id="reset" style="padding: 10px 20px; border: none; background-color: #3498db; color: white; border-radius: 5px; cursor: pointer;">Reset value</button> -->
      <button id="btn-scan" style="padding: 10px 20px; border: none; background-color: #27ae60; color: white; border-radius: 5px; cursor: pointer;">Scan</button>
    </div>

    <div id="element-scan" style="margin: 10px auto; padding: 10px; text-align: center;"></div>

    <form action="/wifi" method="post" style="display: flex; flex-direction: column; padding: 20px; max-width: 400px; margin: auto; background-color: #f7f7f7; border-radius: 10px; box-shadow: 0 2px 5px rgba(0,0,0,0.1);">
      <label for="device" style="margin-bottom: 5px; font-weight: bold;">Device:</label>
      <select name="device" id="device" style="margin-bottom: 15px; padding: 8px; border-radius: 5px; border: 1px solid #ccc;">
        <option value="">NOT REGISTERED</option>
        <option value="BED">BED</option>
        <option value="TOILET">TOILET</option>
        <option value="LAMPP">LAMPP</option>
      </select>
      

    <!--
      <label for="nursestation" style="margin-bottom: 5px; font-weight: bold;">Nurse Station:</label>
      <select name="nursestation" id="nursestation" style="margin-bottom: 15px; padding: 8px; border-radius: 5px; border: 1px solid #ccc;">
        <option value="">NOT AUTOCONNECT MODE</option>
        <option value="1">1</option>
        <option value="2">2</option>
        <option value="3">3</option>
      </select>
    -->

      <input type="number" id="ip" name="ip" placeholder="STATIC IP" style="margin-bottom: 15px; padding: 10px; border-radius: 5px; border: 1px solid #ccc;">

      <input type="text" id="ssid" name="ssid" placeholder="SSID" style="margin-bottom: 15px; padding: 10px; border-radius: 5px; border: 1px solid #ccc;">
    
    <!--
      <input type="text" name="password" id="password" placeholder="Password" style="margin-bottom: 15px; padding: 10px; border-radius: 5px; border: 1px solid #ccc;">
    -->

      <input type="text" name="id" id="id" placeholder="ID" style="margin-bottom: 15px; padding: 10px; border-radius: 5px; border: 1px solid #ccc;">

      <button type="submit" style="padding: 10px; background-color: #e67e22; color: white; border: none; border-radius: 5px; cursor: pointer;">Save</button>
    </form>

    <div style="text-align: center; margin-top: 20px;">
      <button id="restart" style="padding: 10px 20px; background-color: #c0392b; color: white; border: none; border-radius: 5px; cursor: pointer;">Restart</button>
    </div>

  <script>

    function onStart() {
      const xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function() {
          if (this.readyState == 4 && this.status == 200) {
            // Typical action to be performed when the document is ready:
            const dataForm = JSON.parse(xhttp.responseText);
            document.getElementById("ssid").value = dataForm.ssid;
            // document.getElementById("password").value = dataForm.password;
            document.getElementById("id").value = dataForm.id;
            document.getElementById("device").value = dataForm.device;
            // document.getElementById("nursestation").value = dataForm.nursestation;
            document.getElementById("ip").value = dataForm.ip;
          }
      };
      xhttp.open("GET", "/get", true);
      xhttp.send();
    }

    onStart();

    document.getElementById("btn-scan").onclick = () => {
      const xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function() {
          if (this.readyState == 4 && this.status == 200) {
            // Typical action to be performed when the document is ready:
            const scanNetwork = JSON.parse(xhttp.responseText);
            let html = "";
            scanNetwork.forEach((val) => {
              html += `<a href='#' onclick="setSSID('${val.SSID}')" >${val.SSID} : ${val.RSSI}</a><br>`
            })
            document.getElementById("element-scan").innerHTML = html;
          }
      };
      xhttp.open("GET", "/scan", true);
      xhttp.send();
    }

    document.getElementById("restart").onclick = () => {
      const xhttp = new XMLHttpRequest();
      xhttp.open("GET", "/restart", true);
      xhttp.send();
    }

    document.getElementById("reset").onclick = () => {
      document.getElementById("ssid").value = "";
      // document.getElementById("password").value = "";
      document.getElementById("id").value = "";
      document.getElementById("device").value = "";
      // document.getElementById("nursestation").value = "";
    }

    function setSSID(val) {
      document.getElementById("ssid").value = val;
      document.getElementById("password").focus();
    }
  </script>
  </body>
  </html>
  
)rawliteral";


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

void onWifiConnect(const WiFiEventStationModeGotIP &event) {
  if(log_wifi){
    Serial.print("[WiFi] Connected to Wi-Fi.  ");
    Serial.println(WiFi.localIP());
  }

  state_wifi = true;
  connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected &event) {
  if(log_wifi) {
    Serial.println("[Wifi] Disconnected from Wi-Fi.");
  }
  state_wifi = false;
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
  if(log_mqtt) {
    Serial.println("[MQTT] Publish received.");
    Serial.print("[MQTT] topic: ");
    Serial.println(topic);
    Serial.print("[MQTT] payload: ");
    Serial.println(payload[0]);
  }

  if(DEVICE_TYPE == "BED") {
    if(millis() - timer_setelah_klik > 6000) {

      if(topic[0] == 'a' && (payload[0] == 'x' || payload[0] == 'c')){//assist
        x_server_topic_assist = 0;
      }
      if(topic[0] == 'a' && (payload[0] == 'a')){//assist
        x_server_topic_assist = 1;
      }
      
      if(topic[0] == 'i' && (payload[0] == 'x' || payload[0] == 'c')){//infus
        x_server_topic_infus = 0;
      }
      if(topic[0] == 'i' && (payload[0] == 'i')){//infus
        x_server_topic_infus = 1;
      }

      if(topic[0] == 'b' && (payload[0] == 'x' || payload[0] == 'c')){//emergency
        x_server_topic_emergency = 0;
      }
      if(topic[0] == 'b' && (payload[0] == 'e')){//emergency
        x_server_topic_emergency = 1;
      }

    }

    if(log_mqtt) {
      Serial.println("==================================================");
      Serial.println("BED");
      Serial.print(x_server_topic_emergency);
      Serial.print("\t");
      Serial.print(x_server_topic_infus);
      Serial.print("\t");
      Serial.println(x_server_topic_assist);
      Serial.println("==================================================");
    }
  }
  
  else if(DEVICE_TYPE == "TOILET") {
    if(millis() - timer_setelah_klik > 6000) {
      if(topic[0] == 't' && (payload[0] == 'x' || payload[0] == 'c')){//emergency
        x_server_topic_emergency = 0;
      }
      if(topic[0] == 't' && payload[0] == 'e'){//emergency
        x_server_topic_emergency = 1;
      }
    }

    if(log_mqtt) {
      Serial.println("==================================================");
      Serial.println("TOILET");
      Serial.println(x_server_topic_emergency);
      Serial.println("==================================================");
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
    Serial.println("ADA PING MASUK");
    timer_ping = millis();
  }
}


void modeAP() {
  mode_ap = true;
  waiting_to_setup = false;
  timeout_ap = millis();

  WiFi.disconnect(true);
  WiFi.softAP("");

  IPAddress IP = WiFi.softAPIP();
  if(log_setup) {
    Serial.print("[SETUP] AP IP address: ");
    Serial.println(IP);
  }
}

void onEmergencyClicked() {
  if(log_device) {
    Serial.println("[DEVICE] BTN EMERGENCY CLICKED");
  }
  x_server_topic_emergency = 1;
  publish(MQTT_PUB_EMERGENCY, "e");
  activity = true;
}

void onAssistClicked() {
  if(log_device) {
    Serial.println("[DEVICE] BTN ASSIST CLICKED");
  }
  x_server_topic_assist = 1;
  publish(MQTT_PUB_ASSIST, "a");
  activity = true;
}

void onInfusClicked() {
  if(log_device) {
    Serial.println("[DEVICE] BTN INFUS CLICKED");
  }
  x_server_topic_infus = 1;
  publish(MQTT_PUB_INFUS, "i");
  activity = true;
}

void onCancelClicked() {
  if(log_device) {
    Serial.println("[DEVICE] BTN CANCEL CLICKED");
  }
  if(DEVICE_TYPE == "BED") {
    
    publish(MQTT_PUB_INFUS, "c");
    publish(MQTT_PUB_ASSIST, "c");
    publish(MQTT_PUB_EMERGENCY, "c");

  } else if (DEVICE_TYPE == "TOILET") {
    
    publish(MQTT_PUB_EMERGENCY, "c");

  }

  x_server_topic_emergency = 0;
  x_server_topic_infus = 0;
  x_server_topic_assist = 0;

  activity = false;
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
    Serial.printf("[SETUP] Reading file: %s\n", path);
  }

  File file = LittleFS.open(path, "r");
  if (!file) {
    if(log_setup) {
      Serial.println("[SETUP] Failed to open file for reading");
    }
    // return;
  }

  String data = file.readString();
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

  if (!LittleFS.begin()) {
    Serial.println("LittleFS Mount Failed");
    return;
  }

  ssid = ssid.isEmpty() ? readFile("/ssid.txt") : ssid;
  password = password.isEmpty() ? readFile("/password.txt") : password;
  id = id.isEmpty() ? readFile("/id.txt") : id;
  DEVICE_TYPE = DEVICE_TYPE.isEmpty() ? readFile("/device.txt") : DEVICE_TYPE;
  STATIC_IP = STATIC_IP.isEmpty() ? readFile("/ip.txt") : STATIC_IP;

  int analogDeviceType = analogRead(ANALOGPIN);

  // if (analogDeviceType < 255){
  //   DEVICE_TYPE = "LAMPP";
  // } else if (analogDeviceType > 750) {
  //   DEVICE_TYPE = "BED";
  // } else {
  //   DEVICE_TYPE = "TOILET";
  // }

  btn_setup.begin();
  btn_setup.onValidClick(modeAP);

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

    MQTT_PUB_EMERGENCY = "toilet/" + id;

  } else if (DEVICE_TYPE == "LAMPP") {

    pinMode(LAMPP_LED_merah, OUTPUT);
    pinMode(LAMPP_LED_kuning, OUTPUT);
    pinMode(LAMPP_LED_hijau, OUTPUT);
    pinMode(LAMPP_LED_biru, OUTPUT);

  } 

  if(ssid == ""){
    ssid = "tidak boleh kosong";
  }

  // Menghapus konfigurasi WiFi sebelumnya
  WiFi.disconnect(true);

  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

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

  

}

void loop() {
  // put your main code here, to run repeatedly:
  btn_setup.update();

  if(DEVICE_TYPE == "BED") {
    ONEWAY_DEVICE();
  } 

  else if(DEVICE_TYPE == "TOILET") {
    TOILET_DEVICE();
  }
  
  else if (DEVICE_TYPE == "LAMPP") {
    LAMPP();
  } 

  else {
    btn_setup.update();
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

  if (x_server_topic_assist == 0 && x_server_topic_emergency == 0 && x_server_topic_infus == 0) {
    state_led = false;
  } else {
    state_led = true;
  }

  if(clear_dari_server == 1) {
    state_led = false;
    activity = false;
  }

  if(state_led){
    if(millis() - state_led_timer > 500){
      digitalWrite(BED_LED_CANCEL, 1);
      delay(100);
      digitalWrite(BED_LED_CANCEL, 0);
      state_led_timer = millis();
    }
  }

  if(state_mqtt) {
    if(millis() - timer_device_activation > 30000 ) {
      mqttClient.subscribe(MQTT_PUB_EMERGENCY.c_str(), 1);
      mqttClient.subscribe(MQTT_PUB_ASSIST.c_str(), 1);
      mqttClient.subscribe(MQTT_PUB_INFUS.c_str(), 1);
      mqttClient.publish("aktif", 0, false, id.c_str());

      if(x_server_topic_emergency == 1) {
        publish(MQTT_PUB_EMERGENCY, "e");
      } else {
        publish(MQTT_PUB_EMERGENCY, "c");
      }

      if(x_server_topic_infus == 1) {
        publish(MQTT_PUB_INFUS, "i");
      } else {
        publish(MQTT_PUB_INFUS, "c");
      }

      if(x_server_topic_assist == 1) {
        publish(MQTT_PUB_ASSIST, "a");
      } else {
        publish(MQTT_PUB_ASSIST, "c");
      }

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

  if (x_server_topic_emergency == 0) {
    state_led = false;
  } else {
    state_led = true;
  }

  if(clear_dari_server == 1) {
    state_led = false;
    activity = false;
  }

  if(state_led){
    if(millis() - state_led_timer > 500){
      digitalWrite(BED_LED_CANCEL, 1);
      delay(100);
      digitalWrite(BED_LED_CANCEL, 0);
      state_led_timer = millis();
    }
  }

  if(state_mqtt) {
    if(millis() - timer_device_activation > 30000 ) {
      mqttClient.subscribe(MQTT_PUB_EMERGENCY.c_str(), 1);
      mqttClient.publish("aktif", 0, false, id.c_str());

      if(x_server_topic_emergency == 1) {
        publish(MQTT_PUB_EMERGENCY, "e");
      } else {
        publish(MQTT_PUB_EMERGENCY, "c");
      }

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
