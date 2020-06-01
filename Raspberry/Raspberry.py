#
#
#Raspberry: 
# vorbereitungen (mosquitto, mqtt) siehe link https://tutorials-raspberrypi.de/datenaustausch-raspberry-pi-mqtt-broker-client/
#Datei erstellen mit sudo nano DATEINAME.py
#ausfuehren mit sudo python DATEINAME.py
#Nachrichten schicken mit mosquitto_pub -h localhost -t test_channel -m "Hello Raspberry Pi"
#Die nachricht wird dann in der console ausgegeben und beim dem befehl:mosquitto_pub -h localhost -t test_channel -m "antworte"
#antwortet der Pi
#
#
#
import paho.mqtt.client as mqtt
import paho.mqtt.publish as publish
 
#MQTT_SERVER = "192.168.178.57" #ip adresse des Raspberrys
#MQTT_PATH = "test_channel"

MQTT_SERVER = "localhost"
#MQTT_PATH = "test_channel"
MQTT_PATH_COMMAND="command_channel"
MQTT_PATH_EARTH_HUMIDITY="earth_humidity_channel"
MINHUM=50 
#sobald dieser Feuchtigkeitswert unterschritten ist wird der Giessbefehl gesendet
#The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))
 
    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe(MQTT_PATH_EARTH_HUMIDITY)
   
# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
   
    humidity = int (msg.payload)#-48
    print(msg.topic+" "+str(msg.payload)) 
    print(" humidity: "+str(humidity)) 
    print(" humidity als zahl: %d"%humidity) 
    # more callbacks, etc
    if humidity <= MINHUM:
        print(" humidity < 50. Pumpbefehl wird gesendet ") 
        publish.single(MQTT_PATH_COMMAND, "p", hostname=MQTT_SERVER)
    #if str(msg.payload) =="antworte":
        
     #   publish.single(MQTT_PATH, "geantwortet und empfangen", hostname=MQTT_SERVER)
 
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

 
client.connect(MQTT_SERVER, 1883, 60)
 
# Blocking call that processes network traffic, dispatches callbacks and
# handles reconnecting.
# Other loop*() functions are available that give a threaded interface and a
# manual interface.
client.loop_forever()


 

 

