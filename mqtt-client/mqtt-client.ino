/*
 Basic ESP8266 MQTT example
 This sketch demonstrates the capabilities of the pubsub library in combination
 with the ESP8266 board/library.
 It connects to an MQTT server then:
  - publishes "hello world" to the topic "outTopic" every two seconds
  - subscribes to the topic "inTopic", printing out any messages
    it receives. NB - it assumes the received payloads are strings not binary
  - If the first character of the topic "inTopic" is an 1, switch ON the ESP Led,
    else switch it off
 It will reconnect to the server if the connection is lost using a blocking
 reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
 achieve the same result without blocking the main loop.
 To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"
*/

/* Simon:
verwendete Bibliothek: PubSubClient  die Bibliothek hab ich über die Arduino-IDE installiert.
Anpassen muss man:
    wlan ssid und passwort
    IP das mqtt Server
das verwendete topic lautet " /outTopic "; der Befehl auf dem Raspi: mosquitto_sub -d -t outTopic
zum prüfen, ob der NodeMCU sich mit dem wlan verbindet und etwas ausgibt, hilft der serielle Monitor
*/

#include <PubSubClient.h>
//ADC
#include "driver/adc.h"
#include "esp_adc_cal.h"
//#include <ESP8266WiFi.h> //Für ESP8266
#include <WiFi.h> //FÜR ESP32
#define BUILTIN_LED 2 //Für ESP32
#define PUMP_PIN 4
#define MQTT_PATH_COMMAND  "command_channel"
#define MQTT_PATH_EARTH_HUMIDITY  "earth_humidity_channel"
// Update these with values suitable for your network.

/*const char* ssid = " hier ssid einfuegen ";
const char* password = " hier passwort einfuegen ";
const char* mqtt_server = "192.168.10.58";*/


const char* ssid = "o2-WLAN80";
const char* password = "49YMT8F7E84L8673";
const char* mqtt_server = "192.168.178.57";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

double getHumidity() { //TODO Testen und evtl. callibriren
    double Hum;
    int rawHum = adc1_get_raw(ADC1_CHANNEL_0);
    Hum = (rawHum * 10) / 4019; //10 ist feucht 0 ist trocken
    return Hum;
}

void setup_wifi() {

    delay(10);
    // We start by connecting to a WiFi network
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    randomSeed(micros());

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();

    // Switch on the LED if an 1 was received as first character
    //if ((char)payload[0] == '1') {
    //    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    //    // but actually the LED is on; this is because
    //    // it is active low on the ESP-01)
    //}
    //else {
    //    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
    //}

    if ((char)payload[0] == 'p') //pump befehl
    {//TODO wird das relais high oder low gesteuert?
        //TODO DELAY IM CALLBACK UND LED GEHT NICHT WIRKLICH LANGE
        digitalWrite(PUMP_PIN, HIGH);   
        digitalWrite(BUILTIN_LED, LOW);//zum veranschaulichen
        delay(1000);
        delay(1000);
        delay(1000);
        delay(1000);
        digitalWrite(PUMP_PIN, HIGH);
        digitalWrite(BUILTIN_LED, LOW);
        digitalWrite(PUMP_PIN, HIGH);
        digitalWrite(BUILTIN_LED, LOW);
        digitalWrite(PUMP_PIN, LOW);
        digitalWrite(BUILTIN_LED, HIGH);
    }

}

void reconnect() {
    // Loop until we're reconnected
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        // Create a random client ID
        String clientId = "ESP8266Client-";
        clientId += String(random(0xffff), HEX);
        // Attempt to connect
        if (client.connect(clientId.c_str())) {
            Serial.println("connected");
            // Once connected, publish an announcement...
            //client.publish("outTopic", "hello world");

            int iHum = 40; //TODO Hum auslesen und als string in MQTT
            snprintf(msg, MSG_BUFFER_SIZE, "%ld", iHum);
            client.publish(MQTT_PATH_EARTH_HUMIDITY, msg);
            // ... and resubscribe
           // client.subscribe("inTopic");
            client.subscribe(MQTT_PATH_COMMAND);
        }
        else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}


void setup() {
    pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
    pinMode(PUMP_PIN, OUTPUT);//Pin Pumpe
    //start adc
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_0);

    Serial.begin(115200);
    setup_wifi();
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
}

void loop() {

    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    unsigned long now = millis();
    if (now - lastMsg > 2000) {
        lastMsg = now;
        ++value;
        //snprintf(msg, MSG_BUFFER_SIZE, "hello world #%ld", value);
        //Serial.print("Publish message: ");
        //Serial.println(msg);
        //client.publish("outTopic", msg);
        //Pi test
        //client.publish("test_channel", msg); //funktioniert
        int iHum = 40; //TODO Hum auslesen und als string in MQTT
        snprintf(msg, MSG_BUFFER_SIZE, "%ld", iHum);
        client.publish(MQTT_PATH_EARTH_HUMIDITY, msg);

    }
}