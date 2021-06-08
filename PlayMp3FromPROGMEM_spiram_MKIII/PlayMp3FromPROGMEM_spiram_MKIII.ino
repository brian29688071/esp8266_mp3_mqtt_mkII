/*先取mqtt的封包放進byte[]然後丟入*/
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <vector>
#include <Arduino.h>
#include "AudioFileSourceMQTT.h"
#include "AudioFileSourceSPIRAMBuffer.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2SNoDAC.h"
#include "string.h"
#include "stdlib.h"
using namespace std;
const char *ssid = "MSHOME";
const char *password = "0226919072";
const char *mqtt_server_l = "192.168.0.111";
const char *mp3_frame_byte = "mp3_frame_byte", *mynowplay = "mynowplay"; //topic
int f_size = 0;
byte file_d[1024];
WiFiClient server;
PubSubClient mqtt(server);
AudioFileSourceMQTT *file;
AudioFileSourceSPIRAMBuffer *buffer;
AudioGeneratorMP3 *mp3;
AudioOutputI2SNoDAC *out;
void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.println("收到data");
  if ((String)topic == "mp3_frame_byte")
  {
    Serial.println("load");
    file->isplay = true;
    Serial.println(file->isplay);
    for (int f = 0; f < length; f++)
    {
      if (f != 0)
        file_d[f - 1] = payload[f];
    }
    file->load(file_d);
  }
  else if ((String)topic == "play_start")
  {
    Serial.println("開始撥放");
    mqtt.publish("start_play", "start_play");
    mqtt.publish("start_play", "start_play");
    mqtt.publish("start_play", "start_play");
    mp3->begin(buffer, out);
  }
  else if ((String)topic == "end")
  {
    file->isplay = false;
    mqtt.publish("end_play", "end_play");
  }
  else if ((String)topic == "size")
  {
    char c_size[length];
    for (int f = 0; f < length; f++)
    {
      c_size[f] = (char)payload[f];
      //Serial.println(c_size[f]);
    }
    f_size = atoi(c_size);
    file->size = f_size;
    //Serial.println(f_size);
  }
}
void reconnect()
{
  while (!mqtt.connected())
  {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Clinet-";
    clientId += String(random(0xffff), HEX);
    if (mqtt.connect(clientId.c_str()))
    {
      Serial.println("connected");
      mqtt.subscribe("mp3_frame_byte", 0);
      mqtt.subscribe("mynowplay", 0);
      mqtt.subscribe("order", 0);
      mqtt.subscribe("size", 0);
      mqtt.subscribe("play_start", 0);
      mqtt.subscribe("end", 0);
      mqtt.publish("online", "online");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 2 seconds");
      delay(2000);
    }
  }
}
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
void setup()
{
  Serial.begin(115200);
  setup_wifi();
  mqtt.setServer(mqtt_server_l, 1883);
  mqtt.setCallback(callback);
  audioLogger = &Serial;
  file = new AudioFileSourceMQTT(&mqtt);
  buffer = new AudioFileSourceSPIRAMBuffer(file, 15, 128 * 1024);
  out = new AudioOutputI2SNoDAC();
  mp3 = new AudioGeneratorMP3();
}
void loop()
{
  if (file->isplay == true)
  {
    Serial.println("loop");
    mp3->loop();
  }
  if (!mqtt.connected())
  {
    reconnect();
  }
  mqtt.loop();
}
//錯誤為數據幀8bit頭無法解碼 可能解決辦法:先確認傳來的data是否有誤