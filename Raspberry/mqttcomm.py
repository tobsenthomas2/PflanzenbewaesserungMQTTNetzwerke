#empfaengt Daten vom ESP und aktiviert abhaengig der Feuchtigkeit die Pumpe
#TODO:
#   Pumpe abschalten. Problem: Timer geht nicht, da wegen dem einlesen der Werte alles im loop l�uft
#   Ansaetze: Auswertung vom Daten erfassen trennen 
#           *zweites File und globale Variablen: hat nicht geklappt 
#           *publish p2000 an den ESP um den timer mit 2000 ms dort laufen zu lassen -> "p2000" symbolweise auslesen
#           *threading in python verwenden -> kompliziert


import paho.mqtt.client as mqtt

import time

# Don't forget to change the variables for the MQTT broker!
mqtt_topic = [("earth_humidity_channel",0),("command_channel",0)]
#mqtt_broker_ip = "192.168.10.58"
mqtt_broker_ip = "192.168.178.57"


client = mqtt.Client()

# These functions handle what happens when the MQTT client connects
# to the broker, and what happens then the topic receives a message
def on_connect(client, userdata, flags, rc):
    # rc is the error code returned when connecting to the broker
    print ("Connected!"), str(rc)
    
    # Once the client has connected to the broker, subscribe to the topic
    client.subscribe(mqtt_topic)
    
def on_message(client, userdata, msg):
    # This function is called everytime the topic is published to.
    # If you want to check each message, and do something depending on
    # the content, the code to do this should be run in this function
    
    print ("Topic: ", msg.topic + "\nMessage: " + str(msg.payload))   
    sub = str(msg.payload)
    print(sub)    
    
    #now = datetime.datetime.now()    
    
    if (sub < '840'):
        print("zu trocken")
        #starttime = now
        #stoptime = starttime + datetime.timedelta(seconds=10)
        client.publish("command_channel","p")
        print("Pumpe gestartet")
        #print(starttime, stoptime)
        print("Before the sleep statement")
        time.sleep(5)
        print("After the sleep statement")
        client.publish("command_channel","s")

    
   

    # The message itself is stored in the msg variable
    # and details about who sent it are stored in userdata

# Here, we are telling the client which functions are to be run
# on connecting, and on receiving a message
client.on_connect = on_connect
client.on_message = on_message

# Once everything has been set up, we can (finally) connect to the broker
# 1883 is the listener port that the MQTT broker is using
client.connect(mqtt_broker_ip, 1883)

# Once we have told the client to connect, let the client object run itself
client.loop_forever()
client.disconnect()

