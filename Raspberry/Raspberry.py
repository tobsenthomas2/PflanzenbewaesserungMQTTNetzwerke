#
#
#Raspberry: 
# vorbereitungen (mosquitto, mqtt) siehe link https://tutorials-raspberrypi.de/datenaustausch-raspberry-pi-mqtt-broker-client/
#Datei erstellen mit sudo nano DATEINAME.py
#ausführen mit sudo python DATEINAME.py
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
MQTT_PATH = "test_channel"
 
# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))
 
    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe(MQTT_PATH)
 
# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    print(msg.topic+" "+str(msg.payload)) 
    # more callbacks, etc
    if str(msg.payload) =="antworte":
        
        publish.single(MQTT_PATH, "geantwortet und empfangen", hostname=MQTT_SERVER)
 
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message
 
client.connect(MQTT_SERVER, 1883, 60)
 
# Blocking call that processes network traffic, dispatches callbacks and
# handles reconnecting.
# Other loop*() functions are available that give a threaded interface and a
# manual interface.
client.loop_forever()


 

 

