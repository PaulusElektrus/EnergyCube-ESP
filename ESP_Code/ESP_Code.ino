// Json
#include <ArduinoJson.h>
String url = "http://192.168.0.+++/rpc/EM.GetStatus?id=0";

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
char statusFromArduino[numChars] = {0};
float uBatt = 0.0;
float iBatt = 0.0;
int bsPower = 0;

// Outgoing Communication
String command = "O";
int power = 0;
Point sensor("energy");


void setup() {
    Serial.begin(115200);

    // W-Lan Verbindung
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    // W-Lan prüfen 
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
    Serial.println(WiFi.localIP());

    // InfluxDB V 1.0
    client.setConnectionParamsV1(INFLUXDB_URL, INFLUXDB_DB_NAME, INFLUXDB_USER, INFLUXDB_PASSWORD);
    sensor.addTag("device", "energy_cube");
}


void loop() {
    getPower();
    sendCommand();
    getMeasurements();
    sendToServer();
}


void getPower() {
// https://arduinojson.org/v6/api/jsonobject/begin_end/
    WiFiClient client;
    HTTPClient http;
    http.begin(client,url);
    int httpCode = http.GET();
    if (httpCode > 0) {
      String payload = http.getString();
      StaticJsonDocument<768> doc;
      deserializeJson(doc, payload);
      JsonObject root = doc.as<JsonObject>();
      int index = 18;
      JsonObject::iterator it = doc.as<JsonObject>().begin();
      it += index;
      power = it->value().as<int>();
    }
    else {
        Serial.print("F_DS");
        power = 0;
    }
    http.end();
}


void sendCommand() {
    String outgoingData = "<" + command + "," + power + ">";
    Serial.print(outgoingData);
    delay(1000);
}


void getMeasurements() {
    recvWithStartEndMarkers();
    if (newData == true) {
        strcpy(tempChars, receivedChars);
        parseData();
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
    strcpy(statusFromArduino, strtokIndx);

    strtokIndx = strtok(NULL, ",");
    uBatt = atof(strtokIndx); 

    strtokIndx = strtok(NULL, ",");
    iBatt = atof(strtokIndx);
    
    strtokIndx = strtok(NULL, ",");
    bsPower = atoi(strtokIndx);
    Serial.println(bsPower);

}


void sendToServer() {
    sensor.clearFields();
    sensor.addField("Status", statusFromArduino);
    sensor.addField("uBatt", uBatt);
    sensor.addField("iBatt", iBatt);
    sensor.addField("bsPower", bsPower);
    client.pointToLineProtocol(sensor);
    if (!client.writePoint(sensor)) {
        Serial.print("F_ID");
        Serial.println(client.getLastErrorMessage());
    }
}