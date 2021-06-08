/*先取mqtt的封包放進byte[]然後丟入*/
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <vector>
#include <Arduino.h>
#include "AudioFileSourcePROGMEM.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2SNoDAC.h"
#include "string.h"
#include "stdlib.h"
#include <SPI.h>
#include "ESP8266Spiram.h"
using namespace std;
const char *ssid = "MSHOME";
const char *password = "0226919072";
const char *mqtt_server = "192.168.0.105";
const char *mp3_frame_byte = "mp3_frame_byte"; //topic
const char *mynowplay = "mynowplay";           //topic
const unsigned short int one_siprambuffer_size = 32768, spiram_piece = 4;
boolean buffer_filled = false;
int now_play = 0, data_p = 0;
WiFiClient espClient;
PubSubClient client(espClient);
AudioFileSourcePROGMEM *file;
AudioGeneratorMP3 *mp3;
AudioOutputI2SNoDAC *out;
ESP8266Spiram *spiram;
unsigned short int r_sent = 0;
struct spiram_part
{
  unsigned int start_position;                 //區塊起始位址
  unsigned int length = one_siprambuffer_size; //區塊長度
  unsigned int pointer = 0;                    //內部指針
  unsigned int data_count = 0;                 //總共載入幾次
  vector<int> data_position;
  bool can_fill = true;
};
spiram_part spiram_pt[spiram_piece];
void resent()
{
  if (r_sent == 100)
    client.publish("can_next", "1");
  else
    r_sent++;
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
void callback(char *topic, byte *payload, unsigned int length) //接收回傳
{
  //Serial.println("收到callback");
  if ((String)topic == (String)mp3_frame_byte)
  {
    Serial.println("收到data");
    r_sent = 0;
    byte file[length];
    for (int f = 0; f < length; f++)
    {
      if (f != 0)
        file[f] = *payload;
      else
        Serial.println(payload[f], HEX);
    }
    unsigned int length2 = sizeof(file);
    fill(file, sizeof(file));
  }
  else
  {
    for (int f = 0; f < length; f++)
      Serial.println(payload[f], HEX);
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
      client.subscribe(mp3_frame_byte, 1);
      client.subscribe(mynowplay);
      client.subscribe("order", 1);
      client.publish("online", "online");
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
boolean fill(byte payload[], unsigned int length)
{
  for (int pointer = 0; pointer < spiram_piece; pointer++)
  {
    if (spiram_pt[pointer].can_fill)
    {
      Serial.print("第:");
      Serial.print(pointer);
      Serial.println("塊");
      Serial.print("位址:");
      Serial.println(spiram_pt[pointer].pointer);
      Serial.print("長:");
      Serial.println(length);
      Serial.println("--------------------");
      if (spiram_pt[pointer].pointer + length > spiram_pt[pointer].length)
      { //檢查此塊是否已滿
        Serial.println("這塊滿了");
        spiram_pt[pointer].can_fill = false;
        spiram_pt[pointer].pointer = 0;
        if (pointer == spiram_piece)
          buffer_filled = true; //暫存已滿
      }
      else
      {
        //Serial.println("1");
        spiram->write(spiram_pt[pointer].pointer, payload, length); //寫入暫存
        //Serial.println("2");
        spiram_pt[pointer].data_position.push_back(pointer);
        //Serial.println("3");
        spiram_pt[pointer].pointer += length;
        //Serial.println("4");
        //Serial.println(spiram_pt[pointer].data_count);
        spiram_pt[pointer].data_count += 1;
        //Serial.println(spiram_pt[pointer].data_count);
        client.publish("can_next", "1");
        break;
      }
    }
  }
  return true;
}
void setup()
{
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  spiram = new ESP8266Spiram(15, 40e6);
  spiram->begin();
  spiram->setSeqMode();
  audioLogger = &Serial;
  out = new AudioOutputI2SNoDAC();
  mp3 = new AudioGeneratorMP3();
  for (unsigned short int spiram_pe = 0; spiram_pe < spiram_piece; spiram_pe++)
    spiram_pt[spiram_pe].start_position = spiram_pe * one_siprambuffer_size;
}
void loop()
{
  if (!client.connected())
  {
    Serial.printf("重新連接");
    reconnect();
  }
  client.loop();
  resent();
  if (spiram_pt[now_play].can_fill == false)
  {
    Serial.println("u");
    delay(10);
    spiram_pt[now_play].can_fill = true;
    now_play++;
    if (now_play == spiram_piece)
      now_play = 0;
  }
}
