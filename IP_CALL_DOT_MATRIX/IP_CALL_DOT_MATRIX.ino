
#include <DMDESP.h>
#include <fonts/Arial_Black_16.h>

//SETUP DMD
#define DISPLAYS_WIDE 6 // Kolom Panel
#define DISPLAYS_HIGH 1 // Baris Panel
DMDESP Disp(DISPLAYS_WIDE, DISPLAYS_HIGH);  // Jumlah Panel P10 yang digunakan (KOLOM,BARIS)


#include <SPI.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <ESPAsyncWebSrv.h>
#include <AsyncMqttClient.h>
#include <ESP8266HTTPClient.h>
#include <LittleFS.h>
#include <FS.h>
#include <ArduinoJson.h>

//////////////////////////////////////////////

String ssid = "Net_4X8G7L2M9K5_11";
String password = "ipcall123";
String id = "running_text_1";
String NURSESTATION;

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

#define MQTT_HOST "192.168.0.1"
#define MQTT_PORT 1883

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

String MQTT_SUB_MATRIX = "dotmatrix";

bool mqttconnected = false;
bool firstmqttconnect = true;
u_long timeSendActivation = 0;
u_long beforeResetWifi = 0;
u_long reset1hari = 0;

int counter_reset_wifi = 0;
int counter_reset_mqtt = 0;

char msg[100];
int lenmsg = 0;
int speed = 30;
int brightness = 20;
bool running = false;

DynamicJsonDocument doc(1024);

bool mode_ap = false;
bool state_buzzer_ap = false;
unsigned long time_buzzer_ap = 0;
unsigned long waiting_setting = 0;

#define BUZZER D4 // D4

bool state_buzzer = false;
u_long timer_buzzer = 0;

AsyncWebServer server(80);

String hasil_scan_network;
bool trigger_scan_network = false;

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

void connectToWifi() {

    Serial.print("Connecting to Wi-Fi ");
    Serial.print(ssid);
    Serial.print(" : ");
    Serial.println(password);
    WiFi.begin(ssid, password);
  
}

void onWifiConnect(const WiFiEventStationModeGotIP &event) {
  counter_reset_wifi = 0;
  Serial.println("Connected to Wi-Fi.");
  connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected &event) {
  counter_reset_wifi++;
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
  
  if(id != "") {
    mqttClient.subscribe(id.c_str(), 1);
  }

}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  counter_reset_mqtt++;
  Serial.println("Disconnected from MQTT.");
  mqttconnected = false;
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


void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  Serial.println("Publish received.");
  Serial.print("  topic: ");
  Serial.println(topic);
  Serial.print("  payload: ");
  Serial.println(payload);

  if(running == false) {

    char buffer[4];
    char buffer_brightness[4];

    for(int i = 0; i < len; i++) {
      if(i < 3) {
        buffer[i] = payload[i];
      }else if (i < 6 && i >= 3) {
        buffer_brightness[i - 3] = payload[i];
      }
       else {
        msg[i - 6] = payload[i];
      }
    }

    buffer[3] = '\0';
    buffer_brightness[3] = '\0';

    lenmsg = len;

    speed = atoi(&buffer[0]);
    brightness = atoi(&buffer_brightness[0]);

    Disp.setBrightness(brightness);

    if(payload[0] == 'x') {
      lenmsg = 0;
    }
  }


}

//////////////////////////////////////////////


void setup(void)
{
  Serial.begin(9600);

  if (!LittleFS.begin()) {
    Serial.println("LittleFS Mount Failed");
    return;
  }

  ssid = ssid.isEmpty() ? readFile("/ssid.txt") : ssid;
  password = password.isEmpty() ? readFile("/password.txt") : password;
  id = id.isEmpty() ? readFile("/id.txt") : id;
  NURSESTATION = NURSESTATION.isEmpty() ? readFile("/nursestation.txt") : NURSESTATION;

  Serial.println(id);

  if(NURSESTATION != "") {
    autoConnectWiFi();
  }



  pinMode(BUZZER, OUTPUT);

  digitalWrite(BUZZER, 0);

  Disp.start(); // Jalankan library DMDESP
  Disp.setBrightness(brightness); // Tingkat kecerahan
  Disp.setFont(Arial_Black_16); // Tentukan huruf


  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);
  
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);

  connectToWifi();
}

void loop(void)
{
  Disp.loop();
  // if(mqttconnected){
  //   digitalWrite(2, LOW);
  // } else {
  //   digitalWrite(2, HIGH);
  // }

  if(lenmsg != 0) {
    // Serial.println(lenmsg);
    TeksJalan(0, speed); 

    // if(millis() - timer_buzzer > 2000){
    //   state_buzzer = !state_buzzer;
    //   digitalWrite(BUZZER, state_buzzer);
    //   timer_buzzer = millis();
    //   Serial.println("TES BUZZER");
    //   Serial.println(state_buzzer);
    // }
  } else {
    // state_buzzer = false;
    // digitalWrite(BUZZER, 0);
  }

  if(Serial.available()){
    char c = Serial.read();
    if(c == 'x') {
      webServerInit();
      mode_ap = true;
    }
  }

  if (mode_ap) {
    if (millis() - time_buzzer_ap > 2000) {
      state_buzzer_ap = !state_buzzer_ap;
      if(state_buzzer_ap){
        digitalWrite(BUZZER, state_buzzer_ap);
      } else {
        analogWrite(BUZZER, 200);
      }
      time_buzzer_ap = millis();
    }

    if(millis() - waiting_setting > 300000) {
      ESP.reset();
    }
  }

  //======================================================================
  // COUNTER RESET
  if(counter_reset_wifi > 10){
    ESP.reset();
  }
  if(counter_reset_mqtt > 10){
    ESP.reset();
  }


  if(mqttconnected && lenmsg == 0){
    // if(firstmqttconnect) {
    //   digitalWrite(BED_LED_CANCEL, HIGH);
    //   delay(100);
    //   digitalWrite(BED_LED_CANCEL, LOW);
    //   delay(100);
    //   digitalWrite(BED_LED_CANCEL, HIGH);
    //   delay(100);
    //   digitalWrite(BED_LED_CANCEL, LOW);
    //   firstmqttconnect = false;
    // }

    if(millis() - timeSendActivation > 30000){
      mqttClient.subscribe(id.c_str(), 1);
      mqttClient.publish("aktif", 0, false, id.c_str());
      timeSendActivation = millis();
    }
  } 
  


  if (millis() > 86400000){
    ESP.reset();
  }



}

void TeksJalan(int y, uint8_t kecepatan) {

  static uint32_t pM;
  static uint32_t x;

  int width = Disp.width();
    
  int fullScroll = Disp.textWidth(msg) + width;
  if((millis() - pM) > kecepatan) { 
    pM = millis();
    if (x < fullScroll) {
      ++x;
      running = true;
    } else {
      running = false;
      x = 0;

      for(int i = 0; i < 100; i++) {
        msg[i] = '\0';
      }
      lenmsg = 0;

      return;
    }
    // Serial.print(width);
    // Serial.print("\t");
    // Serial.print(x);
    // Serial.print("\t");
    // Serial.println(width - x);
    Disp.drawText(width-x, y, msg);
    // Disp.drawText(width - (x - 64), y, msg);
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
    ESP.reset();
  });

  // Send a GET request to <IP>/get?message=<message>
  server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", "{\"ssid\": \"" + ssid + "\", \"password\": \"" + password + "\", \"id\": \"" + id + "\",\"nursestation\":\""+ NURSESTATION +"\"}");
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
