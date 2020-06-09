

/* Simon:
verwendete Bibliothek: PubSubClient  die Bibliothek hab ich über die Arduino-IDE installiert.
Anpassen muss man:
    wlan ssid und passwort
    IP das mqtt Server
das verwendete topic lautet " /outTopic " (inwischen andere Topics); der Befehl auf dem Raspi: mosquitto_sub -d -t outTopic
zum prüfen, ob der NodeMCU sich mit dem wlan verbindet und etwas ausgibt, hilft der serielle Monitor


*/
#include <Arduino.h>

#include <PubSubClient.h>
//ADC
//#include "driver/adc.h"
//#include "esp_adc_cal.h"

#define ESP32
//#define ESP8266
#ifdef ESP8266
#include <ESP8266WiFi.h> //Für ESP8266
#define PUMP_PIN 4 //Pin D2
#define sensorPin 0
#endif

#ifdef ESP32
#include <WiFi.h> //FÜR ESP32
#define BUILTIN_LED 2 //Für ESP32
#define PUMP_PIN 4
#define sensorPin 34
#endif

//////////////////////////////////////////////////////

//Je nach Teilnehmer die Nummer hinter den MQTT_PATH ändern ( bei 3 teilnehmer 0-2)

////////////////////////////////////////////////////
#define MQTT_PATH_COMMAND  "command_channel0"
#define MQTT_PATH_EARTH_HUMIDITY  "earth_humidity_channel0"
// Update these with values suitable for your network.

//Bedingte Compilierung zum Debuggen
#define DEBUG


//const char* ssid = "wlankuhl";
//const char* password = "leer";
//const char* mqtt_server = "192.168.10.58";


const char* ssid = "o2-WLAN80";
const char* password = "49YMT8F7E84L8673";//4 3
const char* mqtt_server = "192.168.178.57";

//Timer
//hw_timer_t* timer = NULL;


WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];

char cStatus = 's';//anfangswertStop

//Funktioniert am ESP8266 leider nicht! -> stattdessen mit millis() arbeiten, läuft überall
//Timer um pumpen abzubrehcne
void IRAM_ATTR onTimer() {
#ifdef DEBUG
    Serial.println("TimInt");
#endif
    cStatus = 's'; //stopstatus der variable
}

//Funktion um Erdfeuchte zu messen
//Sensor kalibirieren: siehe hier: https://wiki.dfrobot.com/Capacitive_Soil_Moisture_Sensor_SKU_SEN0193
int getHumidity() { //TODO Testen und evtl. callibriren
    int Hum;
    int rawHum = analogRead(sensorPin);// Reading potentiometer value
   // Hum = (rawHum * 100) / 4095; //10 ist feucht 0 ist trocken
    Hum = rawHum;
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

    

    if ((char)payload[0] == 'p') //pump befehl        
    {
        cStatus = 'p';//status Variable bei Command p ändern (Pumpbefehl)
    }
    if ((char)payload[0] == 's') //pump befehl
    {
        cStatus = 's';//status Variable bei Command p ändern (Pumpbefehl)
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

            int iHum = getHumidity(); //TODO Hum auslesen und als string in MQTT
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
    Serial.begin(115200);
    /* 1 tick take 1/(80MHZ/80) = 1us so we set divider 80 and count up */
   // timer = timerBegin(0, 80, true);
    /* Attach onTimer function to our timer */
  //  timerAttachInterrupt(timer, &onTimer, true);

    /* Set alarm to call onTimer function every second 1 tick is 1us
    => 1 second is 1000000us */
    /* Repeat the alarm (third parameter = true) */
  //  timerAlarmWrite(timer, 5000000, false);//pumpt für 5 Sekunden

 
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
    if (now - lastMsg > 15000) {
        lastMsg = now;
        
        int iHum = getHumidity(); //TODO Hum auslesen und als string in MQTT
        //TODO der adc ist nicht linear. es muss evtl. calibriert werden und evtl. werte mitteln (auch im Pi möglich)
        snprintf(msg, MSG_BUFFER_SIZE, "%ld", iHum);
#ifdef DEBUG
        Serial.print("Debug Hum:");
        Serial.println(msg);
#endif // DEBUG

        client.publish(MQTT_PATH_EARTH_HUMIDITY, msg);
        Serial.print("published in\n");
        Serial.print(MQTT_PATH_EARTH_HUMIDITY);
        if (cStatus == 'p') //pumpen
        {
            //TODO wird das relais high oder low gesteuert?
            
#ifdef DEBUG
            Serial.println("Pumpe an");
#endif
            digitalWrite(PUMP_PIN, LOW);
            digitalWrite(BUILTIN_LED, LOW);//zum veranschaulichen
            //timerAlarmEnable(timer);//Startet Timer um Pumpe wieder auszuschalten 
       
        }

        if (cStatus == 's')//statusvar auf stop
        {
#ifdef DEBUG
            Serial.println("Pumpe aus");
#endif
            digitalWrite(PUMP_PIN, HIGH);
            digitalWrite(BUILTIN_LED, HIGH);
        }

    }
}