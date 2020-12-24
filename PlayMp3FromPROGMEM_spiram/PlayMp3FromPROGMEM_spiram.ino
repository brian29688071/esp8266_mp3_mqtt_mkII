/*先取mqtt的封包放進byte[]然後丟入*/
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Arduino.h>
#include "AudioFileSourcePROGMEM.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2SNoDAC.h"
#include "string.h"
#include "stdlib.h"
#include <SPI.h>
#include "ESP8266Spiram.h"
const char *ssid = "MSHOME";
const char *password = "0226919072";
const char *mqtt_server = "192.168.0.107";
const char *mp3_frame_byte = "mp3_frame_byte";//topic
const char *mynowplay = "mynowplay"; //topic
const unsigned short int one_siprambuffer_size=16384,spiram_piece=8;
WiFiClient espClient;
PubSubClient client(espClient);
AudioFileSourcePROGMEM *file;
AudioGeneratorMP3 *mp3;
AudioOutputI2SNoDAC *out;
ESP8266Spiram *spiram;
struct spiram_part
{
  unsigned short int start_position;
  unsigned short int length;
  unsigned short int pointer;
  unsigned short int data_length;
  bool filled=false;
};
spiram_part spiram_part_1,spiram_part_2,spiram_part_3
            ,spiram_part_4,spiram_part_5,spiram_part_6
            ,spiram_part_7,spiram_part_8;
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
  if(topic=="mp3_frame_byte")
  {
    fill(payload,length);
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
      Serial.println(client.subscribe(mp3_frame_byte));
      Serial.println(client.subscribe(mynowplay));
      Serial.println(client.publish("online", "online"));
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
boolean fill(byte *payload, unsigned int length)
{

  spiram->write(0x00,payload,length);
  return true;

}
void setup()
{
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  spiram  = new ESP8266Spiram(4, 40e6);
  spiram->begin();
  spiram->setByteMode();
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
