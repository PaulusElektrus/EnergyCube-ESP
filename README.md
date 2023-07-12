# EnergyCube ESP Part

### --> For the Arduino Part see [here](https://github.com/PaulusElektrus/EnergyCube-Arduino)

This is a code documentation for an energy storage system that monitors battery status and grid power consumption. The system communicates with an Arduino and a Shelly device to gather data and sends the collected information to an InfluxDB database for analysis and monitoring.

## Hardware Requirements

- Arduino board
- Shelly device
- ESP8266 Wi-Fi module
- InfluxDB server

## Libraries Used

- ArduinoJson: This library is used to handle JSON data received from the Shelly device.
- ESP8266WiFi: This library provides Wi-Fi connectivity for the ESP8266 module.
- ESP8266HTTPClient: This library enables HTTP communication with the Shelly device.
- WiFiClient: This library provides a client for HTTP communication.
- InfluxDbClient: This library allows communication with the InfluxDB database.

## Shelly Json

- `shellyUrl`: The URL of the Shelly device's JSON API endpoint for obtaining grid power information.

## W-Lan & Webserver

- `WIFI_SSID`: The SSID of the Wi-Fi network to connect to.
- `WIFI_PASSWORD`: The password for the Wi-Fi network.

## InfluxDB

- `INFLUXDB_URL`: The URL of the InfluxDB server.
- `INFLUXDB_DB_NAME`: The name of the InfluxDB database.
- `INFLUXDB_USER`: The username for accessing the InfluxDB server.
- `INFLUXDB_PASSWORD`: The password for accessing the InfluxDB server.
- `client`: An instance of the `InfluxDBClient` class for communication with the InfluxDB server.
- `db`: An instance of the `Point` class representing the InfluxDB measurement.

## Incoming Communication

- `newData`: A boolean flag indicating whether new data has been received.
- `numChars`: The maximum number of characters for the received data.
- `receivedChars`: An array to store the received characters.
- `tempChars`: An array to store temporary characters during parsing.
- `statusFromArduino`: The status received from the Arduino.
- `uBatt`: The battery voltage received from the Arduino.
- `iBatt`: The battery current received from the Arduino.
- `bsPower`: The battery power calculated from the voltage and current.
- `percentBatt`: The battery percentage calculated based on voltage thresholds.

## Outgoing Communication

- `newGridData`: A boolean flag indicating whether new grid power data is available.
- `command`: The command to be sent to the Arduino.
- `gridPower`: The grid power received from the Shelly device.

## Time

- `ntpServer`: The NTP server to synchronize the time.
- `TZ`: The time zone for the system.

## Setup

- Establishes a serial connection for debugging.
- Connects to the Wi-Fi network.
- Sets up the connection parameters for the InfluxDB client.
- Configures the time synchronization with NTP server.

## Main Loop

- Receives data from the Arduino.
- Parses the received data.
- Retrieves grid power data from the Shelly device.
- Builds the command based on the availability of grid power data.
- Sends the command to the Arduino.
- Sends data to the InfluxDB server.

## Usage

1. Connect the required hardware components as per the pin configuration.
2. Install the required libraries in your Arduino IDE.
3. Set up the Wi-Fi credentials and InfluxDB server details.
4. Upload the code to your Arduino board.
5. Monitor the system through the InfluxDB dashboard (e. g. Grafana) or serial monitor.

## Acknowledgments

- ArduinoJson library by Benoit Blanchon (https://arduinojson.org/)
- ESP8266WiFi and ESP8266HTTPClient libraries by Ivan Grokhotkov
- InfluxDbClient library by Thomas Fach-Pedersen