
/*
Dieses Programm wurde im Rahmen des Moduls "Netzwerke" geschrieben. 
Das Programm ist Teil einer Pflanzenbewässerungsanlage.
Der ESP32 ließt über Pin 34 den Spannungswert eines Feuchtesensors der in der Pflanzenerde steckt.
An Pin 4 ist eine Pumpe bzw. das Relais ür die Pumpe angeschlossen
Alle 15 Sekunden wird der aktuelle Sensorwert in die entsprechende Topic geschickt, wo sie von einem Raspberry ausgewerteet wird 
es wird ein entsprechender Befehl zurück gesendet.
Mögliche Befehle:
p : pumpen
s : Pumpe stoppen
r : Rest (DeepSleep) mit nachfolgender Dauer

Ersteller: Simon Kuhlmann, Tobias Thomas
Datum: 20.06.2020


*/


#include <Arduino.h>

#include <PubSubClient.h>


//Welchen Chip benutzt du?
#define ESP32
//#define ESP8266//wurde nicht mehr gewartet sollte aber funktionieren;evtl. Timer auskommentieren

//Wie wird das Relais geschaltet
#define LowIstAus//Sonst wird high aus sein

//Bedingte Compilierung zum Debuggen
#define DEBUG
#define DEEPSLEEP 

//////////////////////////////////////////////////////
//Je nach Teilnehmer die Nummer hinter den MQTT_PATH ändern ( bei 3 teilnehmer 0-2)
////////////////////////////////////////////////////
#define MQTT_PATH_COMMAND  "command_channel2"
#define MQTT_PATH_EARTH_HUMIDITY  "earth_humidity_channel2"

//#define MQTT_PATH_COMMAND  "command_channel1"
//#define MQTT_PATH_EARTH_HUMIDITY  "earth_humidity_channel1"


//#define MQTT_PATH_COMMAND  "command_channel2"
//#define MQTT_PATH_EARTH_HUMIDITY  "earth_humidity_channel2"

#ifdef ESP8266
#include <ESP8266WiFi.h> //Für ESP8266
#define PUMP_PIN 4 //Pin D2
#define sensorPin 0
#endif

#ifdef ESP32
#include <WiFi.h> //FÜR ESP32
#define BUILTIN_LED 2 //Für ESP32 mit verbauter UserLED
#define PUMP_PIN 4//giessen
#define sensorPin 34//FeuchteSensor
#define maxTimeToPump 30000000 //in us//ein timer wird beim pumpbefehl gestartet, welcher das pumpen abbricht, fall es diese zeit überschreitet
#endif

#ifdef DEEPSLEEP
RTC_DATA_ATTR int bootCount = 0; //Zaehler fuer erfolge Neustarts durch DeepSleep
#endif

// Hier Netzwerkdaten Eintragen

//const char* ssid = "wlankuhl";
//const char* password = "leer";
//const char* mqtt_server = "192.168.10.58";


const char* ssid = "o2-WLAN80";
const char* password = "49YMT8F7E84L8673";//4 3
const char* mqtt_server = "192.168.178.57";



#ifdef ESP32
//Timer
hw_timer_t* timer = NULL;
#endif

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];

char cStatus = 's';//anfangswertStop


//Timer Interrupt. Der Timer wird beim Pupbefehl gestartet um eine 
//Notfallabschaltung zu haben falls die Kommunikation abbricht
#ifdef ESP32
void IRAM_ATTR onTimer() {
#ifdef DEBUG
    Serial.println("TimInt");
#endif
    cStatus = 's'; //stopstatus der variable
}
#endif

//Funktion um Erdfeuchte zu messen
//Sensor kalibirieren: siehe hier: https://wiki.dfrobot.com/Capacitive_Soil_Moisture_Sensor_SKU_SEN0193
int getHumidity() { //TODO Testen und evtl. callibriren
    int Hum;
    int rawHum = analogRead(sensorPin);// Reading potentiometer value
   // Hum = (rawHum * 100) / 4095; //100 ist feucht 0 ist trocken
   // Hum = (rawHum * 100) / 4095; //100 ist feucht 0 ist trocken
    Hum = rawHum;
    return Hum;
}


//gibt den Grund an warum der DeepSleep unterbrochen wurde
#ifdef DEEPSLEEP
int sleepTime = 0;
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
#endif

//nach dem pumpbefehl wird eine Timer gestartet um nach maxTimeToPump Sekunden
//eine Notabschaltung der Pumpe zu bewirken
void EmergencyTimerStart(void);


//Stellt Wlan verbindung her
void setup_wifi() {

    delay(10);
    // We start by connecting to a WiFi network
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);


    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    int h = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        h++;
        if (h > 8)
        {
            esp_deep_sleep(5);
        }
    }

    randomSeed(micros());

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}


//wird ausgeführt, falls neue Nachricht in der subscripten Topic vorhanden ist
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
    if ((char)payload[0] == 's') //pump befehl stop
    {
        cStatus = 's';//status Variable bei Command s ändern (stopbefehl)
    }
#ifdef DEEPSLEEP
    if ((char)payload[0] == 'r') //deepSleep
    {
        cStatus = 'r';//status Variable bei Command r ändern (restbefehl)
        char c = payload[1];
        sleepTime = c - '0'; //Wandelt ASCII-Wert um und speicher den Wert in int sleepTimer
    }
#endif
}

//reconnect zum MQTT Broker
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
    pinMode(PUMP_PIN, OUTPUT);//Pin 
#ifdef LowIstAus
    digitalWrite(PUMP_PIN, LOW);
#else
    digitalWrite(PUMP_PIN, HIGH);
#endif
    Serial.begin(115200);
#ifdef ESP32
    /* 1 tick take 1/(80MHZ/80) = 1us so we set divider 80 and count up */
    timer = timerBegin(0, 80, true);
    /* Attach onTimer function to our timer */
    timerAttachInterrupt(timer, &onTimer, true);

    /* Set alarm to call onTimer function every second 1 tick is 1us
    => 1 second is 1000000us */
    /* Repeat the alarm (third parameter = true) */
    timerAlarmWrite(timer, maxTimeToPump, false);
#endif

    setup_wifi();
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
#ifdef DEEPSLEEP
    //Increment boot number and print it every reboot
    ++bootCount;
    Serial.println("Boot number: " + String(bootCount));

    //Print the wakeup reason for ESP32
    print_wakeup_reason();
#endif
}

void loop() {

    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    unsigned long now = millis();
    if (now - lastMsg > 15000) {  //wenn wach, alle 15000 ms einen Messwert senden
        lastMsg = now;

        int iHum = getHumidity(); 
        snprintf(msg, MSG_BUFFER_SIZE, "%ld", iHum);
#ifdef DEBUG
        Serial.print("Debug Hum:");
        Serial.print(msg);
#endif // DEBUG

        client.publish(MQTT_PATH_EARTH_HUMIDITY, msg);
        Serial.print(" published in ");
        Serial.println(MQTT_PATH_EARTH_HUMIDITY);
    }

    if (cStatus == 'p') //pumpen
    {
        cStatus = 0;
#ifdef DEBUG
        Serial.println("Pumpe an");
#endif


#ifdef LowIstAus
        digitalWrite(PUMP_PIN, HIGH);

#else
        digitalWrite(PUMP_PIN, LOW);

#endif
        digitalWrite(BUILTIN_LED, HIGH);//zum veranschaulichen
        Serial.println("Timer Starten");
#ifdef ESP32
        EmergencyTimerStart();
#endif
    }

    if (cStatus == 's')//statusvar auf stop
    {
        cStatus = 0;
#ifdef DEBUG
        Serial.println("Pumpe aus");
#endif
#ifdef LowIstAus
        digitalWrite(PUMP_PIN, LOW);

#else
        digitalWrite(PUMP_PIN, HIGH);
#endif
        digitalWrite(BUILTIN_LED, LOW);
#ifdef ESP32
        timerAlarmDisable(timer);//Schaltet den Timer aus wenn stop befehl von raspPi kommt
#endif

    }

#ifdef DEEPSLEEP
    if (cStatus == 'r')//status schlafen mit gegebener Zeit
    {
        client.publish(MQTT_PATH_EARTH_HUMIDITY, "y");//gibt dem Pi bescheid, dass er schläft und keine Befehle senden soll bis er wieder wach ist
        Serial.print("jetzt wird geschlafen: ");
        Serial.println(sleepTime * 1000000);
        esp_deep_sleep(sleepTime * 1000000);  //wird in us angegeben
        cStatus = 0;
    }
#endif
    


}


//Funktion um eine Notfallabschaltung zu Gewährleisten
void EmergencyTimerStart(void)
{
    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, maxTimeToPump, false);
    timerAlarmEnable(timer);

}