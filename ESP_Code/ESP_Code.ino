// Shelly Json
#include <ArduinoJson.h>
String shellyUrl = "http://192.168.0.+++/rpc/EM.GetStatus?id=0";
// https://arduinojson.org/v6/api/jsonobject/begin_end/

// W-Lan & Webserver
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#define WIFI_SSID "+++"
#define WIFI_PASSWORD "+++"

// InfluxDB
#include <InfluxDbClient.h>
#define INFLUXDB_URL "+++"
#define INFLUXDB_DB_NAME "+++"
#define INFLUXDB_USER "+++"
#define INFLUXDB_PASSWORD "+++"
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

// Outgoing Communication
bool newGridData = false;
int command = 0;
int gridPower = 0;
Point db("energy");


void setup() {
    Serial.begin(115200);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
    Serial.println(WiFi.localIP());

    // InfluxDB V1.0
    client.setConnectionParamsV1(INFLUXDB_URL, INFLUXDB_DB_NAME, INFLUXDB_USER, INFLUXDB_PASSWORD);
    db.addTag("device", "energy_cube");
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
        newData = false;
    }  
}


void recvWithStartEndMarkers() {
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
}


void getGridPower() {
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
        command = 1;
    }
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
    client.pointToLineProtocol(db);
    if (!client.writePoint(db)) {
        Serial.print("F_ID: ");
        Serial.println(client.getLastErrorMessage());
    }
}