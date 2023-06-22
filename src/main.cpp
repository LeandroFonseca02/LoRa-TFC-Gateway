#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "lib.h"
//Libraries for LoRa
#include <SPI.h>
#include <LoRa.h>
#include <ArduinoJson.h>

//define the pins used by the LoRa transceiver module
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

//433E6 for Asia
//866E6 for Europe
//915E6 for North America
#define BAND 868E6

//OLED pins
#define OLED_SDA 4
#define OLED_SCL 15
#define OLED_RST 16
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);


const char* ssid = "";
const char* password = "";

//Your Domain name with URL path or IP address with path
String serverName = "http://192.168.0.10:8000/new_message";

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastTime = 0;
// Timer set to 10 minutes (600000)
//unsigned long timerDelay = 600000;
// Set timer to 5 seconds (5000)
unsigned long timerDelay = 5000;


String LoRaData;
unsigned long lastPacketRecievedTime=0;
const byte channel = 4;
const byte nodeAddress = 0x02;
int counter = 0;
StaticJsonDocument<250> receiveJson;
StaticJsonDocument<250> sendJson;



void displayError(int errorCode, String error){
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("ERROR");
    display.setCursor(0, 20);
    display.print("Error Code: ");
    display.print(errorCode);
    display.setCursor(0, 30);
    display.print(error);
    display.display();
    Serial.println("ERROR OCCURRED");
    Serial.printf("\n\tError Code: %d", errorCode);
    Serial.print("\n\tError Message: ");
    Serial.print(error);
    Serial.print("\n\n");
}

void displayRecievePacket(String data, int rssi){
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print(data);
    display.setCursor(0, 60);
    display.print("RSSI:");
    display.print(rssi);
    display.display();
    Serial.printf("\n\tMessage: ");
    Serial.print(String(data));
    Serial.printf("\n\tRSSI: %d\n\n", rssi);
}

void displaySendPacket(byte destinationNode, String data, int messageId){
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("LORA Sender");
    display.setCursor(0, 20);
    display.print("Send packet to: ");
    display.print(String(destinationNode));
    display.setCursor(0, 30);
    display.print("Message Id: ");
    display.print(messageId);
    display.setCursor(0, 40);
    display.print(data);
    display.display();
    Serial.printf("Send Packet to 0x%02hhX with Message Id: %d", destinationNode, messageId);
    Serial.printf("\n\tMessage: ");
    Serial.print(String(data));
    Serial.print("\n\n");
}


String receivePacket(int packetSize){
    lastPacketRecievedTime = millis();

    if(packetSize == 0){
        displayError(-1,"Packet vazio");
        return "";
    }

    int rssi = LoRa.packetRssi();

    //byte dataLength = LoRa.read();

    String data = "";

    while(LoRa.available()){
        byte b = LoRa.read();
        if(b >= 32){
            data += (char)b;
        }
    }
    displayRecievePacket(data,rssi);
    return data;
}

void displaySendPacket(String data){
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("LORA Sender");
    display.setCursor(0, 20);
    display.print("Message Id: ");
    display.print(data);
    display.display();
    Serial.printf("\n\tMessage: ");
    Serial.print(String(data));
    Serial.print("\n\n");
}

void sendPacket(String data){
    LoRa.beginPacket();
    LoRa.write(data.length());
    LoRa.print(data);
    LoRa.endPacket();
    displaySendPacket(data);
    counter++;
}

void setup() {
    Serial.begin(115200);

    pinMode(OLED_RST, OUTPUT);
    digitalWrite(OLED_RST, LOW);
    delay(20);
    digitalWrite(OLED_RST, HIGH);

    //initialize OLED
    Wire.begin(OLED_SDA, OLED_SCL);
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) { // Address 0x3C for 128x32
        Serial.println(F("SSD1306 allocation failed"));
        for(;;); // Don't proceed, loop forever
    }

    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(0,0);


    setupWifi(ssid, password);
    Serial.println("");
    Serial.print("Connected to WiFi network with IP Address: ");
    Serial.println(WiFi.localIP());
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("IP: ");
    display.print(WiFi.localIP().toString());
    display.display();

    //SPI LoRa pins
    SPI.begin(SCK, MISO, MOSI, SS);
    //setup LoRa transceiver module
    LoRa.setPins(SS, RST, DIO0);
    LoRa.setSpreadingFactor(7);
    LoRa.setPreambleLength(8);
    LoRa.setSignalBandwidth(125E3);
    LoRa.setCodingRate4(7);
    LoRa.setSyncWord(0x12);

    if (!LoRa.begin(BAND)) {
        Serial.println("Starting LoRa failed!");
        while (1)
            ;
    }

    Serial.println("Timer set to 5 seconds (timerDelay variable), it will take 5 seconds before publishing the first reading.");
}

void loop() {
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
        LoRaData = receivePacket(packetSize);
        deserializeJson(receiveJson, LoRaData);
        sendHTTPPOSTRequest(serverName, LoRaData);
        if(receiveJson["data"]["status"] == "request"){
            Serial.println("Received Status Request");
            sendJson["destination"] = receiveJson["devEUI"];
            sendJson["data"]["status"] = true;
            String json;
            serializeJson(sendJson, json);
            sendPacket(json);
            Serial.println("Packet Sent "+json);
        }
    }
    //Send an HTTP POST request every 10 minutes
//    if ((millis() - lastTime) > timerDelay) {
    //Check WiFi connection status
//        sendHTTPGETRequest(serverName);

//        lastTime = millis();
//    }


}