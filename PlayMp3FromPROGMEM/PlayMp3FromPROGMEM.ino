#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Arduino.h>
#include "AudioFileSourcePROGMEM.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2SNoDAC.h"
#include "string.h"
#include "stdlib.h"
#include "ESP8266Spiram.h"
const char *ssid = "MSHOME";
const char *password = "0226919072";
const char *mqtt_server = "192.168.0.107";
const char *mp3_frame_byte = "mp3_frame_byte";//topic
const char *mynowplay = "mynowplay"; //topic
WiFiClient espClient;
PubSubClient client(espClient);
AudioFileSourcePROGMEM *file;
AudioGeneratorMP3 *mp3;
AudioOutputI2SNoDAC *out;
void setup_wifi()
{
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
void callback(char *topic, byte *payload, unsigned int length) //接收回傳
{
  if ( (String)topic == (String)mp3_frame_byte)
  {
    file = new AudioFileSourcePROGMEM(payload, length);
    if(mp3->begin(file, out))
    while (mp3->isRunning())
    {
      if (!mp3->loop())
        mp3->stop();
    }
    free(file);
    client.publish("can_next", "can_next");
  }
}
void reconnect()
{
  // Loop until  reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Clinet-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str()))
    {
      Serial.println("connected");
      if (client.subscribe(mp3_frame_byte))
        Serial.println("mp3_frame_byte has been subscribed");
      client.subscribe(mynowplay);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2 seconds");
      delay(2000);
    }
  }
}
void setup()
{
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  audioLogger = &Serial;
  out = new AudioOutputI2SNoDAC();
  mp3 = new AudioGeneratorMP3();
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();
}
