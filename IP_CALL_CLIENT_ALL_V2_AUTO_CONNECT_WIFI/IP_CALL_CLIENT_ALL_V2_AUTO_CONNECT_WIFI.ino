#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <ESPAsyncWebSrv.h>
#include <AsyncMqttClient.h>
#include <ESP8266HTTPClient.h>
#include <LittleFS.h>
#include <FS.h>
#include <ArduinoJson.h>

// CONFIGURATION
//////////////////////////////////////////////

String ssid = "";
String password = "";
String id = "";
String DEVICE_TYPE = "";
String NURSESTATION = "";

#define MQTT_HOST "192.168.0.1"
#define MQTT_PORT 1883

String SSID_NURSESTATION_1[] = {
  "Net_4X8G7L2M9K5_1",
  "Net_4X8G7L2M9K5_2",
  "Net_4X8G7L2M9K5_3",
  "Net_4X8G7L2M9K5_4",
  "Net_4X8G7L2M9K5_5",
  "Net_4X8G7L2M9K5_6",
  "Net_4X8G7L2M9K5_7",
  "Net_4X8G7L2M9K5_8",
  "Net_4X8G7L2M9K5_9",
  "Net_4X8G7L2M9K5_10",
};

String SSID_NURSESTATION_2[] = {
  "Net_4X8G7L2M9K5_11",
  "Net_4X8G7L2M9K5_12",
  "Net_4X8G7L2M9K5_13",
  "Net_4X8G7L2M9K5_14",
  "Net_4X8G7L2M9K5_15",
  "Net_4X8G7L2M9K5_16",
  "Net_4X8G7L2M9K5_17",
  "Net_4X8G7L2M9K5_18",
  "Net_4X8G7L2M9K5_19",
  "Net_4X8G7L2M9K5_20",
};

String SSID_NURSESTATION_3[] = {
  "Net_4X8G7L2M9K5_21",
  "Net_4X8G7L2M9K5_22",
  "Net_4X8G7L2M9K5_23",
  "Net_4X8G7L2M9K5_24",
  "Net_4X8G7L2M9K5_25",
  "Net_4X8G7L2M9K5_26",
  "Net_4X8G7L2M9K5_27",
  "Net_4X8G7L2M9K5_28",
  "Net_4X8G7L2M9K5_29",
  "Net_4X8G7L2M9K5_30",
};

//////////////////////////////////////////////

AsyncWebServer server(80);




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

String readFile(const char *path) {
  Serial.printf("Reading file: %s\n", path);

  File file = LittleFS.open(path, "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
    // return;
  }

  String data = file.readString();
  file.close();
  return data;
}

void writeFile(const char *path, const char *message) {
  Serial.printf("Writing file: %s : ", path);
  Serial.println(message);
  File file = LittleFS.open(path, "w");
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  delay(500);  // Make sure the CREATE and LASTWRITE times are different
  file.close();
}

void autoConnectWiFi() {
  Serial.println("-- AUTOCONNECT MODE--");
  int n = WiFi.scanNetworks();

  int min_rssi = -100;
  password = "ipcall123";

  if(NURSESTATION == "1") {
    for (int i = 0; i < n; ++i) {
      // TEMUKAN SSID YANG SAMA
      for (int j = 0; j < 10; j++) {
        if(SSID_NURSESTATION_1[j] == WiFi.SSID(i)) {
          Serial.print(WiFi.SSID(i));
          Serial.print("\t");
          Serial.println(WiFi.RSSI(i));
          if(min_rssi < WiFi.RSSI(i)) {
            min_rssi = WiFi.RSSI(i);
            ssid = WiFi.SSID(i);
          }
        }
      }
    }
  }
  else if (NURSESTATION == "2") {
    for (int i = 0; i < n; ++i) {
      // TEMUKAN SSID YANG SAMA
      for (int j = 0; j < 10; j++) {
        if(SSID_NURSESTATION_1[j] == WiFi.SSID(i)) {
          if(min_rssi < WiFi.RSSI(i)) {
            min_rssi = WiFi.RSSI(i);
            ssid = WiFi.SSID(i);
          }
        }
      }
    }
  } 
  else if (NURSESTATION == "3") {
    for (int i = 0; i < n; ++i) {
      // TEMUKAN SSID YANG SAMA
      for (int j = 0; j < 10; j++) {
        if(SSID_NURSESTATION_1[j] == WiFi.SSID(i)) {
          if(min_rssi < WiFi.RSSI(i)) {
            min_rssi = WiFi.RSSI(i);
            ssid = WiFi.SSID(i);
          }
        }
      }
    }
  }

  Serial.println(ssid);

  writeFile("/ssid.txt", ssid.c_str());
  writeFile("/password.txt", password.c_str());
  
}


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
  <h2>ESP Web Server</h2>
  <div id="realtime"></div>
  <button id="reset">Reset value</button>
  <button id="btn-scan" >scan</button>
  <div id="element-scan"></div>
  <form action="/wifi" method="post" style="display: flex; flex-direction: column; padding: 20px">
    <select name="device" id="device">
      <option value="">NOT REGISTERED</option>
      <option value="BED">BED</option>
      <option value="TOILET">TOILET</option>
      <option value="LAMPP">LAMPP</option>
    </select>
    <select name="nursestation" id="nursestation">
      <option value="">NOT AUTOCONNECT MODE</option>
      <option value="1">1</option>
      <option value="2">2</option>
      <option value="3">3</option>
    </select>
    <input type="text" id="ssid" name="ssid" placeholder="ssid">
    <input type="text" name="password" id="password" placeholder="password">
    <input type="text" name="id" id="id" placeholder="id">
    <button type="submit" >Save</button>
  </form>
  <button id="restart">Restart</button>
<script>

  function onStart() {
    const xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
          // Typical action to be performed when the document is ready:
          const dataForm = JSON.parse(xhttp.responseText);
          document.getElementById("ssid").value = dataForm.ssid;
          document.getElementById("password").value = dataForm.password;
          document.getElementById("id").value = dataForm.id;
          document.getElementById("device").value = dataForm.device;
          document.getElementById("nursestation").value = dataForm.nursestation;
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
    document.getElementById("password").value = "";
    document.getElementById("id").value = "";
    document.getElementById("device").value = "";
    document.getElementById("nursestation").value = "";
  }

  function setSSID(val) {
    document.getElementById("ssid").value = val;
    document.getElementById("password").focus();
  }
</script>
</body>
</html>
)rawliteral";

// BUZZER DEMO
// #define BUZZER D1

// BUZZER
#define BUZZER 2 // D4

// BED DEMO
// #define BED_PIN_ASSIST D2
// #define BED_PIN_INFUS D6
// #define BED_PIN_EMERGENCY D7
// #define BED_PIN_CANCEL D5
// #define BED_LED_CANCEL D3

// BED
#define BED_PIN_ASSIST 4 // D2
#define BED_PIN_INFUS 12 // D6
#define BED_PIN_EMERGENCY 13 // D7
#define BED_PIN_CANCEL 14 // D5
#define BED_LED_CANCEL 0 // D3
#define BED_CEK_ASSIST digitalRead(BED_PIN_ASSIST)
#define BED_CEK_INFUS digitalRead(BED_PIN_INFUS)
#define BED_CEK_EMERGENCY digitalRead(BED_PIN_EMERGENCY)
#define BED_CEK_CANCEL digitalRead(BED_PIN_CANCEL)

// BED DEMO
// #define TOILET_PIN_EMERGENCY D7
// #define TOILET_PIN_CANCEL D5
// #define TOILET_LED_CANCEL D6

// TOILET
#define TOILET_PIN_EMERGENCY 13 // D7  // B
#define TOILET_PIN_CANCEL  14 // D5     // D
#define TOILET_LED_CANCEL 0 // D3     // C
#define TOILET_CEK_EMERGENCY digitalRead(TOILET_PIN_EMERGENCY)
#define TOILET_CEK_CANCEL digitalRead(TOILET_PIN_CANCEL)


// LAMPP DEMO 1
// #define LAMPP_LED_merah   D5
// #define LAMPP_LED_hijau   D7
// #define LAMPP_LED_biru    D2
// #define LAMPP_LED_kuning  D1
// #define LAMPP_BTN_SETUP   D5

// LAMPP DEMO 2
// #define LAMPP_LED_merah   D5
// #define LAMPP_LED_hijau   D5
// #define LAMPP_LED_biru    D5
// #define LAMPP_LED_kuning  D5
// #define LAMPP_BTN_SETUP   D5

// // LAMPP
#define LAMPP_LED_merah 	12 // D6
#define LAMPP_LED_hijau 	0 // D3
#define LAMPP_LED_biru	  4 // D2
#define LAMPP_LED_kuning 	5 // D1
#define LAMPP_BTN_SETUP 	14 // D5
// #define LAMPP_LED_merah 	D5
// #define LAMPP_LED_hijau 	D7
// #define LAMPP_LED_biru	  D2
// #define LAMPP_LED_kuning 	D1
// #define LAMPP_BTN_SETUP 	D3

// UNREGISTERED DEVICE
#define BTN_UNREGISTERED  14 // D5

// GENERAL VARIABLE
AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

String hasil_scan_network;
bool trigger_scan_network = false;

bool mqttconnected = false;
bool firstmqttconnect = true;
u_long timeSendActivation = 0;
u_long beforeResetWifi = 0;

int counter_reset_wifi = 0;
int counter_reset_mqtt = 0;

////////////////////////////////////////////////////////////////////////////////
// BED VARIABLE
////////////////////////////////////////////////////////////////////////////////
String MQTT_PUB_EMERGENCY;
String MQTT_PUB_INFUS;
String MQTT_PUB_ASSIST;

bool kuning_nyala = false;

int clear_dari_server = 0;
u_long timer_setelah_klik = 0;

//===============================================================
u_long time_emergency = 0;
u_long hsl_count_emergency = 0;
int lock_emergency_1 = 0, lock_emergency_2 = 0;
int hsl_emergency = 0;
int lock_kon_emergency = 0;
int x_server_topic_emergency = 0;
//===============================================================
u_long time_cancel = 0;
u_long hsl_count_cancel = 0;
int lock_cancel_1 = 0, lock_cancel_2 = 0;
int hsl_cancel = 0;
int lock_kon_cancel = 0;
//===============================================================
u_long time_infus = 0;
u_long hsl_count_infus = 0;
int lock_infus_1 = 0, lock_infus_2 = 0;
int hsl_infus = 0;
int lock_kon_infus = 0;
int x_server_topic_infus = 0;
//===============================================================
u_long time_assist = 0;
u_long hsl_count_assist = 0;
int lock_assist_1 = 0, lock_assist_2 = 0;
int hsl_assist = 0;
int lock_kon_assist = 0;
int x_server_topic_assist = 0;
//===============================================================
u_long time_buzzer = 0;
bool mode_ap = false;
u_long time_buzzer_ap = 0;
bool state_buzzer_ap = true;
// ==============================================================

int lock_waiting = 0;
u_long lock_waiting_timer = 0;

u_long waiting_setting = 0;

u_long reset_3_hari = 0;

/////////////////////////////////////////////////////////////////////////////////
// LAMPP VARIABLE
/////////////////////////////////////////////////////////////////////////////////
String activeTopic[20];
char valTopic[20];
int countTopic = 0;
bool newData = false;

int counter = 0;
DynamicJsonDocument doc(1024);

u_long before = 0;
bool state = false;

int index_saat_ini = 0;


////////////////////////////////////////////
// GENERAL FUNCTION
////////////////////////////////////////////
void connectToWifi() {
  if(!mode_ap) {
    Serial.print("Connecting to Wi-Fi ");
    Serial.print(ssid);
    Serial.print(" : ");
    Serial.println(password);
    WiFi.begin(ssid, password);
  }
}

void onWifiConnect(const WiFiEventStationModeGotIP &event) {
  counter_reset_wifi = 0;
  Serial.println("Connected to Wi-Fi.");
  connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected &event) {
  counter_reset_wifi++;
  Serial.print(counter_reset_wifi);
  Serial.print("\t");
  Serial.println("Disconnected from Wi-Fi.");
  mqttReconnectTimer.detach();  // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  wifiReconnectTimer.once(2, connectToWifi);
}

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void onMqttConnect(bool sessionPresent) {
  counter_reset_mqtt = 0;
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
  mqttconnected = true;
  firstmqttconnect = true;
  if(DEVICE_TYPE == "BED") {
    mqttClient.subscribe(MQTT_PUB_EMERGENCY.c_str(), 1);
    mqttClient.subscribe(MQTT_PUB_INFUS.c_str(), 1);
    mqttClient.subscribe(MQTT_PUB_ASSIST.c_str(), 1);
  } 
  else if(DEVICE_TYPE == "TOILET") {
    mqttClient.subscribe(MQTT_PUB_EMERGENCY.c_str(), 1);
  }
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  counter_reset_mqtt++;
  Serial.print(counter_reset_mqtt);
  Serial.print("\t");

  Serial.println("Disconnected from MQTT.");
  mqttconnected = false;
  firstmqttconnect = false;
  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  Serial.println("Subscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  Serial.print("  qos: ");
  Serial.println(qos);
}

void onMqttUnsubscribe(uint16_t packetId) {
  Serial.println("Unsubscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void onMqttPublish(uint16_t packetId) {
  Serial.print("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void publish(String topic, String payload) {
  uint16_t packetIdPub1 = mqttClient.publish(topic.c_str(), 1, true, payload.c_str());
  Serial.printf("Publishing on topic %s at QoS 1, packetId: %i\n", topic, packetIdPub1);
  Serial.printf("Message: %s \n", payload);
}

void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  Serial.println("Publish received.");
  Serial.print("  topic: ");
  Serial.println(topic);
  Serial.print("  payload: ");
  Serial.println(payload[0]);

  if(DEVICE_TYPE == "BED") {
    if(millis() - timer_setelah_klik > 6000) {
      if(topic[0] == 'a' && (payload[0] == 'x' || payload[0] == 'c')){//assist
        x_server_topic_assist = 1;
      }
      if(topic[0] == 'i' && (payload[0] == 'x' || payload[0] == 'c')){//infus
        x_server_topic_infus = 1;
      }
      if(topic[0] == 'b' && (payload[0] == 'x' || payload[0] == 'c')){//emergency
        x_server_topic_emergency = 1;
      }
    }

    Serial.println("==================================================");
    Serial.println("BED");
    Serial.print(x_server_topic_emergency);
    Serial.print("\t");
    Serial.print(x_server_topic_infus);
    Serial.print("\t");
    Serial.println(x_server_topic_assist);
    Serial.println("==================================================");
  }
  
  else if(DEVICE_TYPE == "TOILET") {
    if(millis() - timer_setelah_klik > 6000) {
      if(topic[0] == 't' && (payload[0] == 'x' || payload[0] == 'c')){//emergency
        x_server_topic_emergency = 1;
      }
    }

    Serial.println("==================================================");
    Serial.println("TOILET");
    Serial.println(x_server_topic_emergency);
    Serial.println("==================================================");
  }

  else if(DEVICE_TYPE == "LAMPP") {
    if(payload[0] == 'c' || payload[0] == 'x'){
      deleteTopic(topic);
    } else {
      if(payload[0] != '1' && payload[0] != 's'){
        Serial.println("=======================");
        Serial.println("NEW DATA");
        pushTopic(topic, payload[0]);
        newData = true;
      }
    }
  }

}

// ==========================================================================






/////////////////////////////////////
// LAMPP FUNCTION
/////////////////////////////////////

void printTopic(){
  Serial.print("Active Topic : ");
  for(int a = 0; a < countTopic; a++){
    Serial.print(activeTopic[a]); Serial.print(valTopic[a]); Serial.print(", ");
  }
  Serial.println();
  Serial.print("COUNT : "); Serial.println(countTopic);
}

void pushTopic(String topic, char val){
  for(int a = 0; a < countTopic; a++){
    if(topic == activeTopic[a]){
      return;
    }
  }
  activeTopic[countTopic] = topic;
  valTopic[countTopic] = val;
  countTopic++;
  // printTopic();
}

void deleteTopic(String topic){
  for(int a = 0; a < countTopic; a++){
    if(activeTopic[a] == topic){
      if(valTopic[a] == 'e') digitalWrite(LAMPP_LED_merah, 1);
      if(valTopic[a] == 'a') digitalWrite(LAMPP_LED_kuning, 1);
      if(valTopic[a] == 'i') digitalWrite(LAMPP_LED_hijau, 1);
      if(valTopic[a] == 'b') digitalWrite(LAMPP_LED_biru, 1);

      for(int j = a; j < countTopic; j++){
        activeTopic[a] = activeTopic[a+1];
        valTopic[a] = valTopic[a+1];
      }
      countTopic--;
      break;
    }
  }
  // printTopic();
}
//////////////////////////////////////////////////////



void setup() {
  // put your setup code here, to run once:
  // Serial.begin(115200);


  if (!LittleFS.begin()) {
    Serial.println("LittleFS Mount Failed");
    return;
  }

  ssid = ssid.isEmpty() ? readFile("/ssid.txt") : ssid;
  password = password.isEmpty() ? readFile("/password.txt") : password;
  id = id.isEmpty() ? readFile("/id.txt") : id;
  DEVICE_TYPE = DEVICE_TYPE.isEmpty() ? readFile("/device.txt") : DEVICE_TYPE;
  NURSESTATION = NURSESTATION.isEmpty() ? readFile("/nursestation.txt") : NURSESTATION;

  if(NURSESTATION != "") {
    autoConnectWiFi();
  }

  pinMode(BUZZER, OUTPUT);

  if(DEVICE_TYPE == "BED"){
    pinMode(BED_LED_CANCEL, OUTPUT);
    pinMode(BED_PIN_EMERGENCY, INPUT_PULLUP);
    pinMode(BED_PIN_CANCEL, INPUT_PULLUP);
    pinMode(BED_PIN_INFUS, INPUT_PULLUP);
    pinMode(BED_PIN_ASSIST, INPUT_PULLUP);

    digitalWrite(BUZZER, HIGH);

    MQTT_PUB_EMERGENCY = "bed/" + id;
    MQTT_PUB_INFUS = "infus/" + id;
    MQTT_PUB_ASSIST = "assist/" + id;
  } 

  else if(DEVICE_TYPE == "TOILET") {
    pinMode(TOILET_LED_CANCEL, OUTPUT);
    pinMode(TOILET_PIN_EMERGENCY, INPUT_PULLUP);
    pinMode(TOILET_PIN_CANCEL, INPUT_PULLUP);

    MQTT_PUB_EMERGENCY = "toilet/" + id;

    digitalWrite(BUZZER, HIGH);
  }
  
  else if(DEVICE_TYPE == "LAMPP") {
    pinMode(LAMPP_LED_merah, OUTPUT);
    pinMode(LAMPP_LED_kuning, OUTPUT);
    pinMode(LAMPP_LED_hijau, OUTPUT);
    pinMode(LAMPP_LED_biru, OUTPUT);
    pinMode(LAMPP_BTN_SETUP, INPUT_PULLUP);

    digitalWrite(LAMPP_LED_merah, 1);
    digitalWrite(LAMPP_LED_kuning, 1);
    digitalWrite(LAMPP_LED_hijau, 1);
    digitalWrite(LAMPP_LED_biru, 1);
  } 
  
  else {
    pinMode(BTN_UNREGISTERED, INPUT_PULLUP);
  }
  

  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);
  
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);

  connectToWifi();

  // webServerInit();
}

void loop() {
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

    if(BED_CEK_CANCEL == LOW) {
      if (mode_ap == false) {
        Serial.println("----------------------------------");
        Serial.println("---     WEB SERVER RUNNING     ---");
        Serial.println("----------------------------------");
        webServerInit();
      }
      mode_ap = true;
    }

    // if (lock_cancel_1 == 0 && BED_CEK_CANCEL == LOW) {
    //   lock_cancel_1 = 1;
    //   time_cancel = millis();
    //   digitalWrite(BED_LED_CANCEL, HIGH);
    // } 

    // if(BED_CEK_CANCEL == HIGH) {
    //   lock_cancel_1 = 0;
    // }

    // hsl_count_cancel = millis() - time_cancel;

    // // CANCEL DITEKAN LEBIH DARI 30 DETIK
    // if (lock_cancel_1 == 1 && hsl_count_cancel > 30000) {
    //   lock_waiting = 1;
    //   lock_cancel_1 = 0;
    //   digitalWrite(BUZZER, LOW);
    //   delay(100);
    //   digitalWrite(BUZZER, HIGH);
    //   lock_waiting_timer = millis();
    // }


    // if (lock_emergency_1 == 0 && BED_CEK_EMERGENCY == LOW) {
    //   lock_emergency_1 = 1;
    //   time_emergency = millis();
    // } 

    // if(BED_CEK_EMERGENCY == HIGH) {
    //   lock_emergency_1 = 0;
    // }

    // hsl_count_emergency = millis() - time_emergency;

    // // EMERGENCY DITEKAN LEBIH DARI 5 DETIK
    // if (lock_emergency_1 == 1 && hsl_count_emergency > 5000 && lock_waiting == 1) {
    //   lock_waiting = 0;
    //   if (mode_ap == false) {
    //     Serial.println("----------------------------------");
    //     Serial.println("---     WEB SERVER RUNNING     ---");
    //     Serial.println("----------------------------------");
    //     webServerInit();
    //   }
    //   mode_ap = true;
    // }

  }

  //=========================================================
  // BUZZER
  //=========================================================


  if(!mode_ap) {
    if (millis() - time_buzzer > 200) {
      digitalWrite(BUZZER, HIGH);
    } else {
      digitalWrite(BUZZER, LOW);
    }
  }



  //====================================================================
  // SETTING AP MODE
  //====================================================================


  //==========================================================
  // WAITING TO SETTINGG
  //==========================================================
  if(lock_waiting && (millis() - lock_waiting_timer) > 10000) {
    lock_waiting = 0;
    digitalWrite(BUZZER, LOW);
    delay(100);
    digitalWrite(BUZZER, HIGH);
    delay(100);
    digitalWrite(BUZZER, LOW);
    delay(100);
    digitalWrite(BUZZER, HIGH);
  } 

  if (mode_ap) {
    if (millis() - time_buzzer_ap > 2000) {
      state_buzzer_ap = !state_buzzer_ap;
      if(state_buzzer_ap){
        digitalWrite(BUZZER, state_buzzer_ap);
      } else {
        digitalWrite(BUZZER, LOW);
      }
      time_buzzer_ap = millis();
    }

    if(millis() - waiting_setting > 300000) {
      ESP.reset();
    }
  }

  if(trigger_scan_network) {
    hasil_scan_network = scanNetwork();
    trigger_scan_network = false;
  }
  //======================================================================


    //======================================================================
    // COUNTER RESET
    if(counter_reset_wifi > 10){
      ESP.reset();
    }
    if(counter_reset_mqtt > 10){
      ESP.reset();
    }

    if(millis() - reset_3_hari > 604800000) {
      ESP.reset();
    }
}

void webServerInit() {
  waiting_setting = millis();
  WiFi.disconnect();
  WiFi.softAP("");

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", index_html);
  });

  server.on("/restart", HTTP_GET, [](AsyncWebServerRequest *request) {
    ESP.restart();
  });

  // Send a GET request to <IP>/get?message=<message>
  server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", "{\"ssid\": \"" + ssid + "\", \"password\": \"" + password + "\", \"id\": \"" + id + "\",\"device\": \"" + DEVICE_TYPE + "\",\"nursestation\":\""+ NURSESTATION +"\"}");
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
      Serial.print("SSID : ");
      Serial.println(message);
      writeFile("/ssid.txt", message.c_str());
    }
    if (request->hasParam("password", true)) {
      String message = request->getParam("password", true)->value();
      password = message;
      Serial.print("PASS : ");
      Serial.println(message);
      writeFile("/password.txt", message.c_str());
    }
    if (request->hasParam("id", true)) {
      String message = request->getParam("id", true)->value();
      id = message;
      Serial.print("ID : ");
      Serial.println(message);
      writeFile("/id.txt", message.c_str());
    }
    if (request->hasParam("device", true)) {
      String message = request->getParam("device", true)->value();
      DEVICE_TYPE = message;
      Serial.print("DEVICE_TYPE : ");
      Serial.println(message);
      writeFile("/device.txt", message.c_str());
    }
    if (request->hasParam("nursestation", true)) {
      String message = request->getParam("nursestation", true)->value();
      NURSESTATION = message;
      Serial.print("NURSESTATION : ");
      Serial.println(message);
      writeFile("/nursestation.txt", message.c_str());
    }
    request->send(200, "text/html", saved);
  });

  server.begin();
}


void ONEWAY_DEVICE() {

  //==========================================================
  // TOMBOL EMERGENCY
  //==========================================================
  if (lock_emergency_1 == 0 && BED_CEK_EMERGENCY == LOW) {
    lock_emergency_1 = 1;
    time_emergency = millis();
  }

  hsl_count_emergency = millis() - time_emergency;

  // EMERGENCY DITEKAN LEBIH DARI 5 DETIK
  if (lock_emergency_1 == 1 && hsl_count_emergency > 5000 && lock_waiting == 1) {
    lock_waiting = 0;
    if (mode_ap == false) {
      Serial.println("----------------------------------");
      Serial.println("---     WEB SERVER RUNNING     ---");
      Serial.println("----------------------------------");
      webServerInit();
    }
    mode_ap = true;
  }

  if (lock_emergency_1 == 1 && BED_CEK_EMERGENCY == HIGH) {
    
    if (hsl_count_emergency < 2000 && hsl_count_emergency > 100) {
      hsl_emergency = 1;
      Serial.println("TOMBOL EMERGENCY DITEKAN");
      time_buzzer = millis();
    }
    lock_emergency_1 = 0;
  }

  if(hsl_emergency == 1) {
    publish(MQTT_PUB_EMERGENCY, "e");
    kuning_nyala = 1;
    hsl_emergency = 0;
    clear_dari_server = 0;
    x_server_topic_emergency = 0;
    lock_kon_emergency = 1;
    timer_setelah_klik = millis();
  }

  //==========================================================
  // TOMBOL INFUS
  //==========================================================
  if (lock_infus_1 == 0 && BED_CEK_INFUS == LOW) {
    lock_infus_1 = 1;
    time_infus = millis();
  }

  if (lock_infus_1 == 1 && BED_CEK_INFUS == HIGH) {
    hsl_count_infus = millis() - time_infus;
    if (hsl_count_infus < 2000 && hsl_count_infus > 100) {
      hsl_infus = 1;
      Serial.println("TOMBOL INFUS DITEKAN");
      time_buzzer = millis();
    }
    lock_infus_1 = 0;
  }

  if(hsl_infus == 1) {
    publish(MQTT_PUB_INFUS, "i");
    kuning_nyala = 1;
    hsl_infus = 0;
    clear_dari_server = 0;
    x_server_topic_infus = 0;
    lock_kon_infus = 1;
    timer_setelah_klik = millis();
  }

  //==========================================================
  // TOMBOL ASSIST
  //==========================================================
  if (lock_assist_1 == 0 && BED_CEK_ASSIST == LOW) {
    lock_assist_1 = 1;
    time_assist = millis();
  }

  if (lock_assist_1 == 1 && BED_CEK_ASSIST == HIGH) {
    hsl_count_assist = millis() - time_assist;
    if (hsl_count_assist < 2000 && hsl_count_assist > 100) {
      hsl_assist = 1;
      Serial.println("TOMBOL ASSIST DITEKAN");
      time_buzzer = millis();
    }
    lock_assist_1 = 0;
  }

  if(hsl_assist == 1) {
    publish(MQTT_PUB_ASSIST, "a");
    kuning_nyala = 1;
    hsl_assist = 0;
    clear_dari_server = 0;
    x_server_topic_assist = 0;
    lock_kon_assist = 1;
    timer_setelah_klik = millis();
  }


  //==========================================================
  // TOMBOL CANCEL
  //==========================================================
  if (lock_cancel_1 == 0 && BED_CEK_CANCEL == LOW) {
    lock_cancel_1 = 1;
    time_cancel = millis();
    digitalWrite(BED_LED_CANCEL, HIGH);
  }

  hsl_count_cancel = millis() - time_cancel;

  // CANCEL DITEKAN LEBIH DARI 30 DETIK
  if (lock_cancel_1 == 1 && hsl_count_cancel > 30000) {
    lock_waiting = 1;
    lock_cancel_1 = 0;
    digitalWrite(BUZZER, LOW);
    delay(100);
    digitalWrite(BUZZER, HIGH);
    lock_waiting_timer = millis();
  }
  
  if (lock_cancel_1 == 1 && BED_CEK_CANCEL == HIGH) {
    digitalWrite(BED_LED_CANCEL, LOW);
    if (hsl_count_cancel < 2000 && hsl_count_cancel > 100) {
      hsl_cancel = 1;
      Serial.println("TOMBOL CANCEL DITEKAN");
      time_buzzer = millis();
    }
    lock_cancel_1 = 0;
  }
  if (hsl_cancel == 1) {
    hsl_cancel = 0;
    for (int g = 0; g < 2; g++) {
      publish(MQTT_PUB_EMERGENCY, "c");
      publish(MQTT_PUB_INFUS, "c");
      publish(MQTT_PUB_ASSIST, "c");
    }

    kuning_nyala = false;
    clear_dari_server = 0;
    x_server_topic_emergency = 0;
    x_server_topic_infus = 0;
    lock_kon_emergency = 0;
    lock_kon_infus = 0;
  }
  //==========================================================
  //==========================================================


  if(lock_kon_emergency && x_server_topic_emergency) {
    lock_kon_emergency = 0;
    if(!lock_kon_emergency && !lock_kon_infus && !lock_kon_assist){
      clear_dari_server = 1;
    }
  }

  if(lock_kon_infus && x_server_topic_infus) {
    lock_kon_infus = 0;
    if(!lock_kon_emergency && !lock_kon_infus && !lock_kon_assist){
      clear_dari_server = 1;
    }
  }

  if(lock_kon_assist && x_server_topic_assist) {
    lock_kon_assist = 0;
    if(!lock_kon_emergency && !lock_kon_infus && !lock_kon_assist){
      clear_dari_server = 1;
    }
  }

  if(clear_dari_server == 1) {
    kuning_nyala = false;
  }

  if(kuning_nyala){
    if(millis() - beforeResetWifi > 500){
      digitalWrite(BED_LED_CANCEL, 1);
      delay(100);
      digitalWrite(BED_LED_CANCEL, 0);
      beforeResetWifi = millis();
    }
  }

  if(mqttconnected){
    if(firstmqttconnect) {
      digitalWrite(BED_LED_CANCEL, HIGH);
      delay(100);
      digitalWrite(BED_LED_CANCEL, LOW);
      delay(100);
      digitalWrite(BED_LED_CANCEL, HIGH);
      delay(100);
      digitalWrite(BED_LED_CANCEL, LOW);
      firstmqttconnect = false;
    }

    if(millis() - timeSendActivation > 30000){
      mqttClient.subscribe(MQTT_PUB_EMERGENCY.c_str(), 1);
      mqttClient.subscribe(MQTT_PUB_ASSIST.c_str(), 1);
      mqttClient.subscribe(MQTT_PUB_INFUS.c_str(), 1);
      mqttClient.publish("aktif", 0, false, id.c_str());
      timeSendActivation = millis();
    }
  } 
}

void TOILET_DEVICE() {


  //==========================================================
  // TOMBOL EMERGENCY
  //==========================================================
  if (lock_emergency_1 == 0 && TOILET_CEK_EMERGENCY == LOW) {
    lock_emergency_1 = 1;
    time_emergency = millis();
  }

  hsl_count_emergency = millis() - time_emergency;

  // EMERGENCY DITEKAN LEBIH DARI 5 DETIK
  if (lock_emergency_1 == 1 && hsl_count_emergency > 5000 && lock_waiting == 1) {
    lock_waiting = 0;
    if (mode_ap == false) {
      Serial.println("----------------------------------");
      Serial.println("---     WEB SERVER RUNNING     ---");
      Serial.println("----------------------------------");
      webServerInit();
    }
    mode_ap = true;
  }

  if (lock_emergency_1 == 1 && TOILET_CEK_EMERGENCY == HIGH) {
    if (hsl_count_emergency < 2000 && hsl_count_emergency > 100) {
      hsl_emergency = 1;
      Serial.println("TOMBOL EMERGENCY DITEKAN");
      time_buzzer = millis();
    }
    lock_emergency_1 = 0;
  }

  if(hsl_emergency == 1) {
    publish(MQTT_PUB_EMERGENCY, "e");
    kuning_nyala = 1;
    hsl_emergency = 0;
    clear_dari_server = 0;
    x_server_topic_emergency = 0;
    lock_kon_emergency = 1;
    timer_setelah_klik = millis();
  }

  

  //==========================================================
  // TOMBOL CANCEL
  //==========================================================
  if (lock_cancel_1 == 0 && TOILET_CEK_CANCEL == LOW) {
    lock_cancel_1 = 1;
    time_cancel = millis();
    digitalWrite(TOILET_LED_CANCEL, HIGH);
  }

  hsl_count_cancel = millis() - time_cancel;

  // CANCEL DITEKAN LEBIH DARI 30 DETIK
  if (lock_cancel_1 == 1 && hsl_count_cancel > 30000) {
    lock_waiting = 1;
    lock_cancel_1 = 0;
    digitalWrite(BUZZER, LOW);
    delay(100);
    digitalWrite(BUZZER, HIGH);
    lock_waiting_timer = millis();
  }
  
  if (lock_cancel_1 == 1 && TOILET_CEK_CANCEL == HIGH) {
    digitalWrite(TOILET_LED_CANCEL, LOW);
    if (hsl_count_cancel < 2000 && hsl_count_cancel > 100) {
      hsl_cancel = 1;
      Serial.println("TOMBOL CANCEL DITEKAN");
      time_buzzer = millis();
    }
    lock_cancel_1 = 0;
  }
  if (hsl_cancel == 1) {
    hsl_cancel = 0;
    for (int g = 0; g < 2; g++) {
      publish(MQTT_PUB_EMERGENCY, "c");
    }

    kuning_nyala = false;
    clear_dari_server = 0;
    x_server_topic_emergency = 0;
    lock_kon_emergency = 0;
  }
  //==========================================================
  //==========================================================


  if(lock_kon_emergency && x_server_topic_emergency) {
    lock_kon_emergency = 0;
    if(!lock_kon_emergency ){
      clear_dari_server = 1;
    }
  }

  if(clear_dari_server == 1) {
    kuning_nyala = false;
  }

  if(kuning_nyala){
    if(millis() - beforeResetWifi > 500){
      digitalWrite(TOILET_LED_CANCEL, 1);
      delay(100);
      digitalWrite(TOILET_LED_CANCEL, 0);
      beforeResetWifi = millis();
    }
  }

  if(mqttconnected){
    if(firstmqttconnect) {
      digitalWrite(TOILET_LED_CANCEL, 1);
      delay(100);
      digitalWrite(TOILET_LED_CANCEL, 0);
      delay(100);
      digitalWrite(TOILET_LED_CANCEL, 1);
      delay(100);
      digitalWrite(TOILET_LED_CANCEL, 0);
      firstmqttconnect = false;
    }
    if(millis() - timeSendActivation > 5000){
      mqttClient.subscribe(MQTT_PUB_EMERGENCY.c_str(), 1);
      mqttClient.publish("aktif", 0, false, id.c_str());
      timeSendActivation = millis();
    }
  } 
}

void LAMPP(){

  if(digitalRead(LAMPP_BTN_SETUP) == LOW){
    if (mode_ap == false) {
      Serial.println("----------------------------------");
      Serial.println("---     WEB SERVER RUNNING     ---");
      Serial.println("----------------------------------");
      webServerInit();
    }
    mode_ap = true;
  }

  if(newData) {
    if(valTopic[countTopic - 1] == 'e' && activeTopic[countTopic - 1][0] == 'b') {

      String id_temp = activeTopic[countTopic - 1].substring(4);

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
            valTopic[countTopic - 1] = 'b';
          }
      }
    }

    newData = false;
  }

  if(countTopic > 0){
    if(millis() - before > 1000){
      if(valTopic[index_saat_ini] == 'e') {
        digitalWrite(LAMPP_LED_merah, state);
      } else if (valTopic[index_saat_ini] == 'b') {
        digitalWrite(LAMPP_LED_biru, state);
      } else if (valTopic[index_saat_ini] == 'i') {
        digitalWrite(LAMPP_LED_hijau, state);
      } else if (valTopic[index_saat_ini] == 'a') {
        digitalWrite(LAMPP_LED_kuning, state);
      }
      state = !state;

      if(!state) {
        index_saat_ini ++;
        if (index_saat_ini >= countTopic) index_saat_ini = 0;
      }

      before = millis();
    }
  } else {
    digitalWrite(LAMPP_LED_merah, 1);
    digitalWrite(LAMPP_LED_biru, 1);
    digitalWrite(LAMPP_LED_hijau, 1);
    digitalWrite(LAMPP_LED_kuning, 1);
  }

  if(mqttconnected && firstmqttconnect){
    digitalWrite(LAMPP_LED_merah, 0);
    delay(100);
    digitalWrite(LAMPP_LED_merah, 1);
    delay(100);
    digitalWrite(LAMPP_LED_merah, 0);
    delay(100);
    digitalWrite(LAMPP_LED_merah, 1);
    firstmqttconnect = false;
  }

  if(millis() - timeSendActivation > 5000) {
    subscribeRoom();
    mqttClient.publish("aktif", 0, false, id.c_str());
    timeSendActivation = millis();
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

  Serial.println(payload);

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


  endpoint = "/ip-call/server/bed/get.php?id=";
  protocol = "http://";
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
