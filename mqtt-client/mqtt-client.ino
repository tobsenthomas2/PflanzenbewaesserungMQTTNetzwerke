

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
#include "driver/adc.h"
#include "esp_adc_cal.h"

#define ESP32
//#define ESP8266
#ifdef ESP8266
#include <ESP8266WiFi.h> //Für ESP8266
#endif

#ifdef ESP32
#include <WiFi.h> //FÜR ESP32
#define BUILTIN_LED 2 //Für ESP32
#endif


//DeppSleep
#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  5        /* Time ESP32 will go to sleep (in seconds) */
RTC_DATA_ATTR int bootCount = 0;

//giessen
#define PUMP_PIN 4
#define MQTT_PATH_COMMAND  "command_channel"
#define MQTT_PATH_EARTH_HUMIDITY  "earth_humidity_channel"
// Update these with values suitable for your network.

//Bedingte Compilierung zum Debuggen
#define DEBUG


const char* ssid = " hier ssid einfuegen ";
const char* password = " hier passwort einfuegen ";
const char* mqtt_server = "192.168.10.58";


//const char* ssid = "o2-WLAN80";
//const char* password = "9YMT8F7E84L867";//4 3
//const char* mqtt_server = "192.168.178.57";

//Timer
hw_timer_t* timer = NULL;


// Potentiometer is connected to GPIO 34 (Analog ADC1_CH6) 
const int potPin = 34;

// variable for storing the potentiometer value
int potValue = 0;

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];

char cStatus = 's';//anfangswertStop

//Timer um pumpen abzubrehcne
void IRAM_ATTR onTimer() {
#ifdef DEBUG
    Serial.println("TimInt");
#endif
    cStatus = 's'; //stopstatus der variable
}

//Funktion um Erdfeuchte zu messen
int getHumidity() { //TODO Testen und evtl. callibriren
    int Hum;
    int rawHum = analogRead(potPin);// Reading potentiometer value
    Hum = (rawHum * 100) / 4095; //10 ist feucht 0 ist trocken
    return Hum;
}


void print_wakeup_reason() {
    esp_sleep_wakeup_cause_t wakeup_reason;

    wakeup_reason = esp_sleep_get_wakeup_cause();

    switch (wakeup_reason)
    {
    case ESP_SLEEP_WAKEUP_EXT0: Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1: Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER: Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD: Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP: Serial.println("Wakeup caused by ULP program"); break;
    default: Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason); break;
    }
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
    timer = timerBegin(0, 80, true);
    /* Attach onTimer function to our timer */
    timerAttachInterrupt(timer, &onTimer, true);

    /* Set alarm to call onTimer function every second 1 tick is 1us
    => 1 second is 1000000us */
    /* Repeat the alarm (third parameter = true) */
    timerAlarmWrite(timer, 5000000, false);//pumpt für 5 Sekunden

    setup_wifi();
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);

    //Increment boot number and print it every reboot
    ++bootCount;
    Serial.println("Boot number: " + String(bootCount));

    //Print the wakeup reason for ESP32
    print_wakeup_reason();
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +
        " Seconds");


    Serial.println("Going to sleep now");
    Serial.flush();
    //esp_deep_sleep_start();

}

void loop() {

    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    unsigned long now = millis();
    if (now - lastMsg > 2000) {
        lastMsg = now;
        
        int iHum = getHumidity(); //TODO Hum auslesen und als string in MQTT
        //TODO der adc ist nicht linear. es muss evtl. calibriert werden und evtl. werte mitteln (auch im Pi möglich)
        snprintf(msg, MSG_BUFFER_SIZE, "%ld", iHum);
#ifdef DEBUG
        Serial.print("Debug Hum:");
        Serial.println(msg);
#endif // DEBUG

        client.publish(MQTT_PATH_EARTH_HUMIDITY, msg);
        if (cStatus == 'p') //pumpen
        {
            //TODO wird das relais high oder low gesteuert?
            
#ifdef DEBUG
            Serial.println("Pumpe an");
#endif
            digitalWrite(PUMP_PIN, HIGH);
            digitalWrite(BUILTIN_LED, HIGH);//zum veranschaulichen
            timerAlarmEnable(timer);//Startet Timer um Pumpe wieder auszuschalten 
            esp_deep_sleep_start();
       
        }
        if (cStatus == 's')//statusvar auf stop
        {
#ifdef DEBUG
            Serial.println("Pumpe aus");
#endif
            digitalWrite(PUMP_PIN, LOW);
            digitalWrite(BUILTIN_LED, LOW);
            esp_deep_sleep_start();
        }
    }
}

