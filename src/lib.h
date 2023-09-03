#include <WiFi.h>                 // Biblioteca WiFi do ESP32 necessária para a acessar á rede
#include <HTTPClient.h>           // Biblioteca HTTPClirnt do ESP32 necessária para comunicar com a api por HTTP request
#include <Wire.h>                 // Biblioteca Wire do Arduino necessária para o funcionamento do OLED

#ifndef WFRQFRQ_LIB_H
#define WFRQFRQ_LIB_H

#endif

// Método que envia um HTTP GET Request
String sendHTTPGETRequest(String serverName){
    if(WiFi.status()== WL_CONNECTED){
        HTTPClient http;

        http.begin(serverName.c_str());

        // Envia o HTTP GET Request
        int httpResponseCode = http.GET();

        if (httpResponseCode>0) {
            String httpGetResponse = http.getString();
            return httpGetResponse;
        }
        else {
            Serial.print("Error code: ");
            Serial.println(httpResponseCode);
        }
        http.end();
    }
    else {
        Serial.println("WiFi Disconnected");
        return "";
    }
}

// Método que envia um HTTP POST Request
void sendHTTPPOSTRequest(String serverName, String json){

    HTTPClient http;
    http.begin(serverName);

    http.addHeader("Content-Type", "application/json");

    String httpRequestData = json;

    int httpResponseCode = http.POST(httpRequestData);
}
