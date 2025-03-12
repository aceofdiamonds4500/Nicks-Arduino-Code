/**
Title: Door Security Circuit Board Design

Author: Nicholas Cieplensky
Date: October 29, 2024

Description:
This security design for automatic doors is written and designed to accept an RFID tag to scan and open the door.
The ESP8266 board implements a few different modules:
  RFID-RC522             - the aptly-named RFID card reader, sends tag info to the board to send over WiFi.
  128x64 px OLED display - currently displays door status, whether door is being held open
  Servo Motor SG90       - opens and closes the door
  PIR sensor             - holds the door open until no more motion is detected

The program tells NodeMCU to send data through MQTT, so a broker on the host computer is needed to view login info.
TO-DO: 
- add ability to log date and time for each door login
- find the most optimal way to write status on the OLED display
**/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "ESP8266WiFi.h"  // Enables the ESP8266 to connect to the local network (via WiFi)
#include "PubSubClient.h" // Connect and publish to the MQTT broker
#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>

#define SS_PIN D8        // SS pin for RFID
#define SERV_PIN D4
#define RST_PIN D0       // RESET pin for RFID
#define PIR_PIN D3       // INPUT pin for PIR sensor
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

Servo door;
int pos = 0; //servo rotation
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
MFRC522 mfrc522(SS_PIN, RST_PIN);

// WiFi
const char* ssid = "YourSSID";             	      // network name
const char* wifi_password = "YourPassword";           // network password

// MQTT
const char* mqtt_server = "192.168.x.x";             // IP (wlan0) of the MQTT broker
const char* door_topic = "workplace/doors/frontdoor";
const char* mqtt_username = "ESP";                    // MQTT username
const char* mqtt_password = "secretpassword";         // MQTT password
const char* clientID = "client_doors";                // MQTT client ID
const char* prefix = "\nLogin from /doors/frontdoor: "; // MQTT prefix to specify which /door/ topic

//VALID UIDs. Format: byte validUID[4] = {0x00,0x00,0x00,0x00};
byte validUIDArray[4] = {0xF9,0x6C,0x29,0xA4};
unsigned int* validUID = (unsigned int*) & validUIDArray;

//Motion sensor values
int pirStat = 0;

// Initialize the WiFi and MQTT Client objects
WiFiClient wifiClient;
// 1883 listener port (MQTT)
PubSubClient client(mqtt_server, 1883, wifiClient); 

void connect_wifi(){
  int offline_timer = 15;
  Serial.print("Connecting to ");
  Serial.println(ssid);

  // Connect to the WiFi
  WiFi.begin(ssid, wifi_password);

  // Wait until the connection has been confirmed before continuing
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if(offline_timer == 0){
      Serial.println("WiFi could not connect. Starting offline mode.");
      display.clearDisplay();
      display.setCursor(0, 10);
      display.print("Board is offline.");
      display.display();
      delay(750);
      return;
    }
    offline_timer -= 1;
  }
  // Debugging - Output the IP Address of the ESP8266
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  display.clearDisplay();
  display.setCursor(0, 10);
  display.print("Board is online.");
  display.display();
  delay(750);
}

//don't forget to change clientID
void connect_MQTT(){
  if (client.connect(clientID, mqtt_username, mqtt_password)) {
    Serial.println("Connected to MQTT Broker!");
  }
  else {
    Serial.println("Connection to MQTT Broker failed...");
  }
}

void setup() {
  Serial.begin(115200);
  door.attach(SERV_PIN);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  SPI.begin();                                                  // Init SPI bus
  mfrc522.PCD_Init();                                           // Init MFRC522 card
  delay(2000);

  pinMode(PIR_PIN, INPUT);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);

  connect_wifi();
  door_status(false);
}

// true = open, false = close
void door_status(bool isOpen) {
  String status;
  int timeout = 5;
  bool status_sens = false;
  if(isOpen == false){
    for(pos = 180; pos >= 0; pos--){
      door.write(pos);
      delay(1);
    }
    status = "CLOSED";
    display.clearDisplay();
    display.setCursor(0, 10);
    display.print("Door status: ");
    display.print(status);
    display.display();
  }
  else{
    status = "OPEN";
    for(pos = 0; pos <= 180; pos++){
      door.write(pos);
      delay(1);
    }
    while(timeout != 0){
      display.clearDisplay();
      display.setCursor(0, 10);
      display.print("Door status: ");
      display.println(status);
      display.print("Motion status: OFF");        
      display.display();

      timeout -= 1;
      pirStat = digitalRead(PIR_PIN);
      if(pirStat == HIGH){
        timeout = 5;
        display.clearDisplay();
        display.setCursor(0, 10);
        display.print("Door status: ");
        display.println(status);
        display.print("Motion status: ON");
        display.display();
        //delay(1000);
      }
      delay(1000);
    }

    door_status(false);
  }
}

void loop() {
  // Prepare key - all keys are set to FFFFFFFFFFFFh at chip delivery from the factory
  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;
  
  byte block;
  byte len;
  MFRC522::StatusCode status;

  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  Serial.println(F("**Card Detected:**"));

  //-------------------------------------------

  mfrc522.PICC_DumpDetailsToSerial(&(mfrc522.uid)); //dump some details about the card

  byte buffer1[18];
  char name_first[17];

  block = 4;
  len = 18;

  //------------------------------------------- GET FIRST NAME
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 4, &key, &(mfrc522.uid)); //line 834 of MFRC522.cpp file
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Authentication failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    return;
  }

  status = mfrc522.MIFARE_Read(block, buffer1, &len);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Reading failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    return;
  }

  //PRINT FIRST NAME
  for (uint8_t i = 0; i < 16; i++)
  {
    if (buffer1[i] != 32)
    {
      name_first[i] = buffer1[i];
    }
    else
    {
      name_first[i] = '\0';
      break;
    }
  }
  //----------------------------------------

  //---------------------------------------- GET LAST NAME

  byte buffer2[18];
  char name_last[17];
  block = 1;

  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 1, &key, &(mfrc522.uid)); //line 834
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Authentication failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    return;
  }

  status = mfrc522.MIFARE_Read(block, buffer2, &len);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Reading failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    return;
  }

  //PRINT LAST NAME
  uint8_t index = 0;
  for (uint8_t i = 0; i < 16; i++) {
    name_last[index++] = buffer2[i];
  }
  name_last[index] = '\0';

  //----------------------------------------
  Serial.println(F("\n**End Reading**\n"));

  connect_MQTT();

  char name_full[40];            
  strcpy(name_full, name_first);
  strcat(name_full, " ");
  strcat(name_full, name_last);

  Serial.print("Name: ");
  Serial.print(name_full);

  if(WiFi.status() == WL_CONNECTED){
    client.publish(door_topic, String(prefix).c_str());
  }

  //---------------------------------Compare RFID UID to list of valid UID byte arrays. See line 60.
  
  byte inputIDArray[4] = {mfrc522.uid.uidByte[0],mfrc522.uid.uidByte[1],mfrc522.uid.uidByte[2],mfrc522.uid.uidByte[3]};
  unsigned int* inputID = (unsigned int*) & inputIDArray;

  unsigned int result = *inputID ^ *validUID;
  if(result != 0){
    if(WiFi.status() == WL_CONNECTED){
      client.publish(door_topic, "--Invalid ID scanned at \"frontdoor\". Details:");
      client.publish(door_topic, String(name_full).c_str());
    }
    Serial.println("\nID card invalid. Report logged to broker. \nResult:");
    Serial.println(result);
    Serial.println();
    delay(3000);
  }
  else{
    Serial.println("\nValid ID logged to broker.\n");
    if(WiFi.status() == WL_CONNECTED){
      client.publish(door_topic, "++Valid ID scanned at \"frontdoor\". Details:");
      client.publish(door_topic, String(name_full).c_str());
    }
    door_status(true);
  }
  
  client.disconnect();
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}
