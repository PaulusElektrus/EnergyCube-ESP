// Shelly Json
#include <ArduinoJson.h>
const String shellyUrl = "http://192.168.0.+++/rpc/EM.GetStatus?id=0";

// W-Lan & Webserver
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
const String WIFI_SSID = "+++";
const String WIFI_PASSWORD = "+++";

// InfluxDB
#include <InfluxDbClient.h>
const String INFLUXDB_URL = "+++";
const String INFLUXDB_DB_NAME = "+++";
const String INFLUXDB_USER = "+++";
const String INFLUXDB_PASSWORD = "+++";
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_DB_NAME);

// Incoming Communication
boolean newData = false;
const byte numChars = 32;
char receivedChars[numChars];
char tempChars[numChars];

// Incoming Data
int statusFromArduino = 0;
float uBatt = 0.0;
float iBatt = 0.0;
int bsPower = 0;
int percentBatt = 50;
float maxUBatt = 49.2;
float minUBatt = 44.4;
Point db("energy");

// Outgoing Communication
bool newGridData = false;
int command = 0;
int gridPower = 0;

// Sunrise & Sunset
#include "time.h"
const char* ntpServer = "pool.ntp.org";
const char* TZ = "CET-1CEST,M3.5.0,M10.5.0/3";
const float pi = 3.14159265;
const float lat = 48.13;
const float lon = 11.57;
int hourTime = 8;
int sunrise = 6;
int sunset = 19;


void setup() {
    Serial.begin(115200);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
    Serial.println(WiFi.localIP());

    // InfluxDB Version 1.0
    client.setConnectionParamsV1(INFLUXDB_URL, INFLUXDB_DB_NAME, INFLUXDB_USER, INFLUXDB_PASSWORD);
    db.addTag("device", "energy_cube");

    configTime(TZ, ntpServer);
}


void loop() { 
    recvWithStartEndMarkers();
    if (newData == true) {
        strcpy(tempChars, receivedChars);
        parseData();
        getGridPower();
        buildCommand();
        sendCommand();
        sendToServer();
        Serial.println(hourTime);
        Serial.println(sunrise); 
        Serial.println(sunset);  
        newData = false;
    }  
}


void recvWithStartEndMarkers() {
// https://forum.arduino.cc/t/serial-input-basics-updated/382007/3
    static boolean recvInProgress = false;
    static byte ndx = 0;
    char startMarker = '<';
    char endMarker = '>';
    char rc;

    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();
        if (recvInProgress == true) {
            if (rc != endMarker) {
                receivedChars[ndx] = rc;
                ndx++;
                if (ndx >= numChars) {
                    ndx = numChars - 1;
                }
            }
            else {
                receivedChars[ndx] = '\0';
                recvInProgress = false;
                ndx = 0;
                newData = true;
            }
        }
        else if (rc == startMarker) {
            recvInProgress = true;
        }
    }
}


void parseData() {
    char * strtokIndx;

    strtokIndx = strtok(tempChars,",");
    statusFromArduino = atoi(strtokIndx);

    strtokIndx = strtok(NULL, ",");
    uBatt = atof(strtokIndx); 

    strtokIndx = strtok(NULL, ",");
    iBatt = atof(strtokIndx);

    bsPower = uBatt * iBatt;

    if (uBatt <= minUBatt) {
        percentBatt = 0;
    }
    else percentBatt = ((uBatt - minUBatt) / (maxUBatt - minUBatt)*100);
}


void getGridPower() {
// https://arduinojson.org/v6/api/jsonobject/begin_end/
    WiFiClient client;
    HTTPClient http;
    http.begin(client,shellyUrl);
    int httpCode = http.GET();
    if (httpCode > 0) {
        String payload = http.getString();
        StaticJsonDocument<768> doc;
        deserializeJson(doc, payload);
        JsonObject root = doc.as<JsonObject>();
        int index = 18;
        JsonObject::iterator it = doc.as<JsonObject>().begin();
        it += index;
        gridPower = it->value().as<int>();
        newGridData = true;
    }
    else {
        Serial.print("F_DS");
        newGridData = false;
    }
    http.end();
}


void buildCommand() {
    if (newGridData = false) {
        command = 0;
    }
    if (newGridData = true) {
        getTime();
        if (hourTime >= sunrise && hourTime <= sunset) {
            command = 1;
        }
        else{
            command = 2;
        }
    }
}


void getTime() {
// https://werner.rothschopf.net/microcontroller/202103_arduino_esp32_ntp_en.htm
    tm timeinfo;
    time_t now;
    if(!getLocalTime(&timeinfo)){
        Serial.println("F-DT");
        return;
    }
    hourTime = timeinfo.tm_hour;
    bool summertime = timeinfo.tm_isdst;
    getRiseSet(now);
}


void getRiseSet(unsigned long unixTime) {
  // Convert Julian day to Unix Timestamp
  unsigned long Jdate = (unsigned long) unixTime / 86400.0 + 2440587.5;
  // Number of days since Jan 1st, 2000 12:00
  float n = (float)Jdate - 2451545.0 + 0.0008;
  // Mean solar noon
  float Jstar = -lon / 360 + n;
  // Solar mean anomaly
  float M = fmod((357.5291 + 0.98560028 * Jstar), 360);
  // Equation of the center
  float C = 1.9148 * sin(M / 360 * 2 * pi) + 0.02 * sin(2 * M / 360 * 2 * pi) + 0.0003 * sin(3 * M * 360 * 2 * pi);
  // Ecliptic longitude
  float lambda = fmod((M + C + 180 + 102.9372), 360);
  // Solar transit
  float Jtransit = Jstar + (0.0053 * sin(M / 360.0 * 2.0 * pi) - 0.0069 * sin(2.0 * (lambda / 360.0 * 2.0 * pi)));
  // Declination of the Sun
  float delta = asin(sin(lambda / 360 * 2 * pi) * sin(23.44 / 360 * 2 * pi)) / (2 * pi) * 360;
  // Hour angle
  float omega0 = 360 / (2 * pi) * acos((sin(-0.83 / 360 * 2 * pi) - sin(lat / 360 * 2 * pi) * sin(delta / 360 * 2 * pi)) / (cos(lat / 360 * 2 * pi) * cos(delta / 360 * 2 * pi)));
  // Julian day sunrise, sunset
  float Jset = Jtransit + omega0 / 360;
  float Jrise = Jtransit - omega0 / 360;
  // Convert to Unix Timestamp
  unsigned long unixRise = Jrise * 86400 + 946728000;
  unsigned long unixSet = Jset * 86400 + 946728000;
  Serial.println(unixRise);
  Serial.println(unixSet);
  sunrise = unixRise;
  sunset = unixSet; 
}


void sendCommand() {
    String outgoingData = "<" + String(command) + "," + String(gridPower) + ">";
    Serial.print(outgoingData);
}


void sendToServer() {
    db.clearFields();
    db.addField("Status", statusFromArduino);
    db.addField("uBatt", uBatt);
    db.addField("iBatt", iBatt);
    db.addField("bsPower", bsPower);
    db.addField("percentBatt", percentBatt);
    client.pointToLineProtocol(db);
    if (!client.writePoint(db)) {
        Serial.print("F_ID: ");
        Serial.println(client.getLastErrorMessage());
    }
}