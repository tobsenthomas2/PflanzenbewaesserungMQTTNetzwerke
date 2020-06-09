
    #empfaengt Daten vom ESP und aktiviert abhaengig der Feuchtigkeit die Pumpe
#TODO:
#   Pumpe abschalten. Problem: Timer geht nicht, da wegen dem einlesen der Werte alles im loop laeuft
#   Ansaetze: Auswertung vom Daten erfassen trennen 
#           *zweites File und globale Variablen: hat nicht geklappt 
#           *publish p2000 an den ESP um den timer mit 2000 ms dort laufen zu lassen -> "p2000" symbolweise auslesen
#           *threading in python verwenden -> kompliziert


import paho.mqtt.client as mqtt

import time

# Don't forget to change the variables for the MQTT broker!
#mqtt_topic = [("earth_humidity_channel",0),("command_channel",0)]
#mqtt_broker_ip = "192.168.10.58"
mqtt_broker_ip = "192.168.178.57"


client = mqtt.Client()


# x teilnehmer koennen mit dieser for unterschieden werden
#es wird nach jeder topic eine Zahl geschrieben

members=2    #hier muss die richtige Anzahl an Teilnemhern -1 stehen

humidityMin=840#ab diesem Sensor wert wird gepumpt

MQTT_PATH_COMMAND=["command_channel"]
MQTT_PATH_EARTH_HUMIDITY=["earth_humidity_channel"]

def getMemberTopics(topic):
    i=1
    if topic==["command_channel"]:
        while i<members:
            print("topicNr:"+str(i)+" created")
            topic=topic+["command_channel"] 
            print (topic)
            i+=1
    else: 
        while i<members:
            print("topicNr:"+str(i)+" created")
            topic=topic+["earth_humidity_channel"] 
            print (topic)
            i+=1

    i=0
    while i<members:
    
        topic[i]=topic[i]+str(i)
        print ("topic list: ")
        print ( topic)
        i+=1
    return(topic)
    
    

MQTT_PATH_COMMAND=getMemberTopics(MQTT_PATH_COMMAND)
MQTT_PATH_EARTH_HUMIDITY=getMemberTopics(MQTT_PATH_EARTH_HUMIDITY)
i=0
while i<members:
    print (MQTT_PATH_COMMAND[i])
    print (MQTT_PATH_EARTH_HUMIDITY[i])
    i+=1

# These functions handle what happens when the MQTT client connects
# to the broker, and what happens then the topic receives a message
def on_connect(client, userdata, flags, rc):
    # rc is the error code returned when connecting to the broker
    print ("Connected!"), str(rc)
    
    # Once the client has connected to the broker, subscribe to the topic
    #client.subscribe(mqtt_topic)
    
   # client.subscribe("earth_humidity_channel0")
   # print ("subscribed")
    p=0
    while p<members: 
        client.subscribe(MQTT_PATH_COMMAND[p])
        print ("subscribed")
        client.subscribe(MQTT_PATH_EARTH_HUMIDITY[p])
        print ("subscribed")
        p+=1
    
def on_message(client, userdata, msg):
    # This function is called everytime the topic is published to.
    # If you want to check each message, and do something depending on
    # the content, the code to do this should be run in this function
    
    print ("Topic: ", msg.topic + "\nMessage: " + str(msg.payload))   
    data = str(msg.payload)
    print(data)    
    
    #now = datetime.datetime.now()    
    n=0
    while n<members:
        if msg.topic==MQTT_PATH_EARTH_HUMIDITY[n]:
            print ("\n")
            print(MQTT_PATH_EARTH_HUMIDITY[n])
            print(" wird aufgerufen")
            humidity = int(msg.payload)
            print("humidity was published")
            print(humidity)

            if (humidity < humidityMin):
                print("zu trocken")
                #starttime = now
                #stoptime = starttime + datetime.timedelta(seconds=10)
                print(MQTT_PATH_COMMAND)
                client.publish(MQTT_PATH_COMMAND[n],"p")
                print("Pumpe gestartet")
                #print(starttime, stoptime)
                print("Before the sleep statement")
                time.sleep(5)
                print("After the sleep statement")
                client.publish(MQTT_PATH_COMMAND[n],"s")
        n+=1


    
   

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

