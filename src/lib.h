//
// Created by refr3sh on 3/13/23.
//
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>

#ifndef WFRQFRQ_LIB_H
#define WFRQFRQ_LIB_H

#endif //WFRQFRQ_LIB_H

void sendHTTPGETRequest(String serverName){
    if(WiFi.status()== WL_CONNECTED){
        HTTPClient http;


        // Your Domain name with URL path or IP address with path
        http.begin(serverName.c_str());

        // If you need Node-RED/server authentication, insert user and password below
        //http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD");

        // Send HTTP GET request
        int httpResponseCode = http.GET();

        if (httpResponseCode>0) {
            Serial.print("HTTP Response code: ");
            Serial.println(httpResponseCode);
            String payload = http.getString();
            Serial.println(payload);
        }
        else {
            Serial.print("Error code: ");
            Serial.println(httpResponseCode);
        }
        // Free resources
        http.end();
    }
    else {
        Serial.println("WiFi Disconnected");
    }
}

void sendHTTPPOSTRequest(String serverName, String json){

    HTTPClient http;
    // Your Domain name with URL path or IP address with path
    http.begin(serverName);

// If you need Node-RED/server authentication, insert user and password below
//http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD");

// Specify content-type header
    http.addHeader("Content-Type", "application/json");

// Data to send with HTTP POST
    String httpRequestData = json;

// Send HTTP POST request
    int httpResponseCode = http.POST(httpRequestData);
}

void setupWifi(const char * ssid, const char * password){
    WiFi.begin(ssid, password);
    Serial.println("Connecting");
    while(WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
}