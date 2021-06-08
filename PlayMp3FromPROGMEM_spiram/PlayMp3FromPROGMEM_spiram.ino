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
const unsigned short int one_siprambuffer_size = 16384, spiram_piece = 8;
boolean buffer_filled = false;
int now_play = 0, data_p = 0;
WiFiClient espClient;
PubSubClient client(espClient);
AudioFileSourcePROGMEM *file;
AudioGeneratorMP3 *mp3;
AudioOutputI2SNoDAC *out;
ESP8266Spiram *spiram;
struct spiram_part
{
  unsigned int start_position;                 //區塊起始位址
  unsigned int length = one_siprambuffer_size; //區塊長度
  unsigned int pointer = 0;                    //內部指針
  unsigned int data_count = 0;                 //總共載入幾次
  vector<int> data_position;
  bool can_fill = true;
};
spiram_part spiram_pt[8];
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

  Serial.println("收到callback");
  if ((String)topic == (String)mp3_frame_byte)
  {
    byte file[length];
    for (int f = 0; f < length; f++)
      file[f] = *payload;
    Serial.println("收到data");
    unsigned int length2 = sizeof(file);
    Serial.println(length, DEC);
    Serial.println(length2, DEC);
    fill(file, sizeof(file));
  }
  else
  {
    unsigned int length = sizeof(payload);
    Serial.print(length, DEC);
    Serial.print(*payload, DEC);
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
        if (pointer == 8)
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
  spiram = new ESP8266Spiram(4, 40e6);
  spiram->begin();
  spiram->setByteMode();
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
  if (!spiram_pt[now_play].can_fill)
  {
    Serial.println("撥放");
    Serial.println("read_s");
    if (data_p < spiram_pt[now_play].data_position.size())
    {
      byte data[spiram_pt[now_play].data_position[data_p + 1] - spiram_pt[now_play].data_position[data_p]];
      Serial.println("read_m");
      spiram->read(spiram_pt[now_play].data_position[data_p], data, spiram_pt[now_play].data_position[data_p + 1] - spiram_pt[now_play].data_position[data_p]);
      Serial.println("read_e");
      file = new AudioFileSourcePROGMEM(data, sizeof(data));
      if (mp3->begin(file, out))
        Serial.println("開始");
      while (mp3->isRunning())
      {
        Serial.println("while");
        if (!mp3->loop())
        {
          Serial.println("結束");
          mp3->stop();
        }
        if (!client.connected())
          reconnect();
        client.loop();
      }
      delete (file);
      data_p++;
    }
    else
    {
    }
  }
}
