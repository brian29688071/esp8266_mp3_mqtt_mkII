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
const char *mqtt_server_l = "192.168.0.109";
const char *mp3_frame_byte = "mp3_frame_byte", *size = "size", *mynowplay = "mynowplay"; //topic
int file_size = 0;
bool opened = false;
AudioFileSourceMQTT *file;
AudioFileSourceSPIRAMBuffer *buffer;
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
void setup()
{
  Serial.begin(115200);
  setup_wifi();
  file = new AudioFileSourceMQTT(mqtt_server_l);
  buffer=new AudioFileSourceSPIRAMBuffer(file,15, 128*1024);
  out = new AudioOutputI2SNoDAC();
  mp3 = new AudioGeneratorMP3();
}
void loop()
{

 if(file->control_loop())
 {
   mp3->begin(buffer,out);
 }
 if(mp3->isRunning()){
   mp3->loop();
 }
}
