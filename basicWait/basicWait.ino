#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <SimpleTimer.h>

#include "config.h"

const int chip_uid = ESP.getChipId();
const int flash_uid = ESP.getFlashChipId();
const int flash_size = ESP.getFlashChipSize();

WiFiClient wifi_client;
PubSubClient mqtt_client(wifi_client);
char msg[20];
String device_topic_name = "/device/esp8266-" + String(chip_uid) + "-" + String(flash_uid);
boolean ledStatus = false;

SimpleTimer timer;

void cbLedBlinking() {
  if (ledStatus == false) {
    ledStatus = true;
    digitalWrite(BUILTIN_LED, LOW);
  } else {
    ledStatus = false;
    digitalWrite(BUILTIN_LED, HIGH);
  }
}

void cbSendKeepAlive() {
  String keepalive_topic_name = device_topic_name + "/keepAlive";
  unsigned long now = millis();
  snprintf (msg, 20, "{\"ut\":%ld}", now);
  Serial.print("Publish message on ");
  Serial.print(keepalive_topic_name.c_str());
  Serial.print(": ");
  Serial.println(msg);
  mqtt_client.publish(keepalive_topic_name.c_str(), msg);
}

void setup_wifi() {
  delay(10);

  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void otaUpdate() {
  t_httpUpdate_return ret = ESPhttpUpdate.update("http://server/file.bin");
  switch(ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("HTTP_UPDATE_NO_UPDATES");
      break;
    case HTTP_UPDATE_OK:
      Serial.println("HTTP_UPDATE_OK");
      break;
  }
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
//  if ((char)payload[0] == 'u') {
//    Serial.println("Start OTA update");
//    otaUpdate();
//  }

}

void mqtt_reconnect() {
  // Loop until we're reconnected
  while (!mqtt_client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    String clientId = "esp8266_" + chip_uid;
    clientId += "_" + flash_uid;
    String statusTopicName = device_topic_name + "/status";

    // Attempt to connect
    if (mqtt_client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      mqtt_client.publish(statusTopicName.c_str(), "initial connected");
      // ... and resubscribe
      mqtt_client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt_client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(9600);
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  timer.setInterval(2000, cbLedBlinking);
  timer.setInterval(10000, cbSendKeepAlive);
  setup_wifi();
  mqtt_client.setServer(mqtt_server, mqtt_port);
  mqtt_client.setCallback(mqtt_callback);
}

// the loop function runs over and over again forever
void loop() {  
  if (!mqtt_client.connected()) {
    mqtt_reconnect();
  }
  mqtt_client.loop();
  timer.run();
}
