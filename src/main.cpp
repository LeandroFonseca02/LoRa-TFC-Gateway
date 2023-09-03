#include <WiFi.h>                 // Biblioteca WiFi do ESP32 necessária para a acessar á rede
#include <HTTPClient.h>           // Biblioteca HTTPClirnt do ESP32 necessária para comunicar com a api por HTTP request
#include <Wire.h>                 // Biblioteca Wire do Arduino necessária para o funcionamento do OLED
#include <Adafruit_GFX.h>         // Biblioteca GFX https://github.com/adafruit/Adafruit-GFX-Library
#include <Adafruit_SSD1306.h>     // Biblioteca SSD1306 https://github.com/adafruit/Adafruit_SSD1306
#include <SPI.h>                  // Biblioteca SPI do Arduino necessária para a comunicação com o modulo LoRa
#include <LoRa.h>                 // Biblioteca LoRa https://github.com/sandeepmistry/arduino-LoRa
#include <Arduino_JSON.h>         // Biblioteca JSON https://github.com/arduino-libraries/Arduino_JSON
#include "lib.h"                  // Biblioteca auxiliar com métodos relacianados com HTTP Requests

// Pinos do rádio LoRa
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

#define BAND 868E6                        // Frequência do rádio LoRa
#define SPREADING_FACTOR 7                // Spreading Factor do rádio LoRa
#define PREAMBLE_LENGTH 8                 // Preamble Length do rádio LoRa
#define BANDWIDTH 125E3                   // Bandwidth do rádio LoRa
#define CODING_RATE 7                     // Coding Rate do rádio LoRa
#define SYNC_WORD 0x12                    // Sync Word do rádio LoRa

// Pinos do display OLED
#define OLED_SDA 4
#define OLED_SCL 15
#define OLED_RST 16
#define SCREEN_WIDTH 128                  // OLED display width, in pixels
#define SCREEN_HEIGHT 64                  // OLED display height, in pixels

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

// Configurações da rede WiFi
const char* ssid = "NOWO-14C76";
const char* password = "wZ73kfejn8Wd";

// Dominios da API
String serverName = "http://192.168.0.17:8000/new_packet";
String getRequest = "http://192.168.0.17:8000/get_status/";


String httpGetResponse;
String LoRaData;
unsigned long lastPacketReceivedTime=0;
int counter = 0;
JSONVar receivedJson;
JSONVar sendJson;
JSONVar httpJson;


// Método para demonstrar no OLED o pacote recebido
void displayReceivedPacket(String data, int rssi){
    JSONVar receivedJson = JSON.parse(data);
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("devEUI:");
    String deveui = receivedJson["devEUI"];
    display.print(deveui);
    display.setCursor(0, 20);
    display.print("App:");
    String app = receivedJson["application"];
    display.print(app);
    display.setCursor(0, 30);
    int dist = receivedJson["data"]["distance"];
    double lat = receivedJson["data"]["latitude"];
    double lon = receivedJson["data"]["longitude"];
    if(dist){
        display.print("Distance:");
        display.print(dist);
        display.print("mm");
    }else if(lat){
        display.print("Latitude:");
        display.print(lat);
        display.setCursor(0, 40);
        display.print("Longitude:");
        display.print(lon);
    }
    display.setCursor(0, 55);

    display.print("RSSI:");
    display.print(rssi);
    display.display();

    Serial.println("Packet Received");
    Serial.printf("\tPayload: ");
    Serial.print(data);
    Serial.printf("\n\tRSSI: %d\n\n", rssi);
}

// Mètodo que converte uma string num JSON
void stringToJson(String data){
    JSONVar receivedJson = JSON.parse(data);
}

// Método que processa o pacote recebido e retorna o payload numa String
String receivePacket(int packetSize){
    lastPacketReceivedTime = millis();

    if(packetSize == 0){
        return "";
    }

    int rssi = LoRa.packetRssi();

    String data = "";

    while(LoRa.available()){
        byte b = LoRa.read();
        if(32 <= b && b <= 127){
            data += (char)b;
        }
        if(data[0] != '{')
            data = data.substring(1);
    }

    JSONVar receivedJson = JSON.parse(data);
    displayReceivedPacket(data,rssi);
    return data;
}

// Método para demonstrar no OLED o pacote enviado
void displaySendPacket(String data){
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("LORA Sender");
    display.setCursor(0, 20);
    display.print("Message Id: ");
    display.print(data);
    display.display();
    Serial.println("Packet Send");
    Serial.printf("\tPayload: ");
    Serial.print(String(data));
    Serial.print("\n\n");
}

// Método para enviar um pacote LoRA
void sendPacket(String data){
    LoRa.beginPacket();
    LoRa.print(data);
    LoRa.endPacket();
    displaySendPacket(data);
    counter++;
}

// Método de inicialização do rádio LoRa
void initLoRa(){
    // Definir os pinos do rádio LoRa
    SPI.begin(SCK, MISO, MOSI, SS);
    LoRa.setPins(SS, RST, DIO0);
    // Parametrização do rádio
    LoRa.setSpreadingFactor(SPREADING_FACTOR);
    LoRa.setPreambleLength(PREAMBLE_LENGTH);
    LoRa.setSignalBandwidth(BANDWIDTH);
    LoRa.setCodingRate4(CODING_RATE);
    LoRa.setSyncWord(SYNC_WORD);

    // Verifica a comunicação com o rádio LoRa
    if (!LoRa.begin(BAND)) {
        Serial.println("Starting LoRa failed!");
        while (1);
    }
}

// Método de inicialização do display OLED
void initOLED(){
    pinMode(OLED_RST, OUTPUT);
    digitalWrite(OLED_RST, LOW);
    delay(20);
    digitalWrite(OLED_RST, HIGH);

    Wire.begin(OLED_SDA, OLED_SCL);
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) {
        Serial.println(F("SSD1306 allocation failed"));
        for(;;);
    }

    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(0,0);
}

// Método de inicialização do WiFi
void setupWifi(const char * ssid, const char * password){
    WiFi.begin(ssid, password);
    Serial.println("Connecting");
    while(WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to WiFi network with IP Address: ");
    Serial.println(WiFi.localIP());
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("IP: ");
    display.print(WiFi.localIP().toString());
    display.display();
}

// Método de inicialização do dispositivo
void setup() {
    Serial.begin(9600);               // Inicialização da comunicação Serial

    initOLED();                       // Inicialização do display OLED

    setupWifi(ssid, password);        // Inicialização da comunicação WiFi

    initLoRa();                       // Inicialização do rádio LoRa
}

// Método de funcionamento do dispositivo
void loop() {

    int packetSize = LoRa.parsePacket();

    // Se receber um pacote verifica o conteudo
    if (packetSize) {
        LoRaData = receivePacket(packetSize);
        JSONVar receivedJson = JSON.parse(LoRaData);
        String status = receivedJson["data"]["status"];

        // Se for um status request pede á API o status do dispositivo
        if(status == "request"){
            delay(1000);
            String devEUI = receivedJson["devEUI"];
            String domain = getRequest + devEUI;

            httpGetResponse = sendHTTPGETRequest(domain);

            httpJson = JSON.parse(httpGetResponse);
            bool status = httpJson["status"];

            sendJson["destination"] = devEUI;
            sendJson["data"]["status"] = status;
            String json = JSON.stringify(sendJson);

            // Envia o status do dispositivo
            sendPacket(json);
        }

        // Envia para a API o pacote recebido
        sendHTTPPOSTRequest(serverName, LoRaData);
    }

}