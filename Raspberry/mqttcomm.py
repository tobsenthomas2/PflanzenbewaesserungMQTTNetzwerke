
	#empfaengt Daten vom ESP und aktiviert abhaengig der Feuchtigkeit die Pumpe
#TODO:
#   Pumpe abschalten. Problem: Timer geht nicht, da wegen dem einlesen der Werte alles im loop laeuft
#   Ansaetze: Auswertung vom Daten erfassen trennen 
#           *zweites File und globale Variablen: hat nicht geklappt 
#           *publish p2000 an den ESP um den timer mit 2000 ms dort laufen zu lassen -> "p2000" symbolweise auslesen
#           *threading in python verwenden -> kompliziert


import paho.mqtt.client as mqtt

import time

import csv


# Einstellen MQTT broker! (IP-Adresse des Raspberrys
#mqtt_broker_ip = "192.168.10.58"
mqtt_broker_ip = "192.168.178.57"

command=0
client = mqtt.Client()

#Import der member-Einstellung ueber die Datei input.csv 
durations=[]
with open("input.csv", "r") as f_input:
	csv_input = csv.DictReader(f_input)
	members=sum(1 for row in f_input)-1 #ermittelt wie viele Zeilen es gibt(Kopfzeile ausgenommen) = Anzahl MCUs
	print("Anzahl Teilnehmer: ", members)
	f_input.seek(0) #Pointer an File-Beginn setzen
	durations=[]
	frequencys=[]
	humMins=[]
	names=[]
	for row in csv_input:
		id = row['$id']
		name = row['name']
		frequency = row['freq']
		duration = row['dur']
		humMin = row['hum']

		exec "name%s=name" % (id) #erstellt variablen fuer alle MCUs
		exec "frequency%s=frequency" % (id)
		exec "duration%s=duration" % (id)
		exec "humMin%s=humMin" % (id)

		#erstellt Eine Liste fuer die bessere Weiterverarbeitung
		print(id, name, frequency, duration,humMin)
		durations=durations+[duration]
		frequencys=frequencys+[frequency]
		humMins=humMins+[humMin]
		names=names+[name]
print(durations)
print(frequencys)
print(humMins)
print (names)
# x teilnehmer koennen mit dieser for unterschieden werden
#es wird nach jeder topic eine Zahl geschrieben

counter=[0]
i=0
#nach 5 Messungen wird der esp32 schlafen gelegt
while i<members:
			counter=counter+[0] 
			print (counter)
			i+=1
MQTT_PATH_COMMAND=["command_channel"]
MQTT_PATH_EARTH_HUMIDITY=["earth_humidity_channel"]
#gibt die richten topics zurueck
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
	
	p=0
	while p<members: 
		#client.subscribe(MQTT_PATH_COMMAND[p])
		client.subscribe(MQTT_PATH_EARTH_HUMIDITY[p])
		
		p+=1
	print ("subscribed to all chans")
	

	#wird aufgerufen wenn eine neue Nachricht in einer subscripten Topic ist
def on_message(client, userdata, msg):
	# This function is called everytime the topic is published to.
	# If you want to check each message, and do something depending on
	# the content, the code to do this should be run in this function
	
	print ("Topic: ", msg.topic + "\nMessage: " + str(msg.payload))   
	data = str(msg.payload)
	print(data)    
	p=0
	n=0
	while p<members:
		if (msg.topic==MQTT_PATH_EARTH_HUMIDITY[p]):
			n=p
		p+=1
	
	
	#print(n)
	if msg.topic==MQTT_PATH_EARTH_HUMIDITY[n]:
		print ("\n")
		print(MQTT_PATH_EARTH_HUMIDITY[n])
		print(" wird aufgerufen")
		humidity = int(msg.payload)
		print("humidity was published")
		print(humidity)
		if (humidity=="y"):
		
			global command
			global q
			command="y"#esp32 ist schlafen gegangen
			
			
			
		elif (humidity > int( humMins[n])):
			print("zu trocken")
			print (humidity)
			print(">")
			print(int(humMins[n]))
			global command
			global q
			command="p"
			counter[n]=0
			print(command)
		else:
			
			counter[n]=counter[n]+1
			print(counter[n])
			if (counter[n]==5): #wenn 5 Werte gesendet wurden und noch genug feuchtigkeit, dann schlafen schicken
				global commandTwo
				global activeMember
				commandTwo="s"
				counter[n]=0
				activeMember=n


	q=n		
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

global command
global activeMember
global commandTwo
commandTwo=0
command=0
activeMember="a"
# Once we have told the client to connect, let the client object run itself
print("vor loop start")
client.loop_start()
print("loop gestartet ab in die while")

#hier werden die commandos, welche im callback in die variablen gespeichert wurden gepublished
while 1:
	global command
	global activeMember
	global commandTwo
	if(command=="y"):#benachrichtigung, dass der NodeMCU schlaeft --> noch gesendetet Befehle werden geloescht
		wait=client.publish(MQTT_PATH_COMMAND[activeMember],"0")#leerer Befehl
		print("esp schlaeft --> letzten befehl ueberschreiben")
		print("warten bis published")
		wait.wait_for_publish()

	if(commandTwo=="s"):
		global activeMember
		global commandTwo
		wait=client.publish(MQTT_PATH_COMMAND[activeMember],"s")#stopbefehl
		print("stopbefehl")
		print("warten bis published")
		wait.wait_for_publish()
		client.publish(MQTT_PATH_COMMAND[activeMember],"r"+frequencys[activeMember])#rest befehl
		print("restbefehl")
		print(names[activeMember]+" wurde gegossen")
		print("Pumpe gestoppt")
		commandTwo=0
		#client.loop_start()
	if(command=="p"):
		global command
		global activeMember
		global commandTwo
		print("command zu p")
		waitTest=client.publish(MQTT_PATH_COMMAND[q],"p")#pumpbefehl
		command=0
		commandTwo="s"
		activeMember=q
		print("warten auf publish")
		waitTest.wait_for_publish()
		print("Pumpe gestartet fuer ")
		print(durations[activeMember])
		time.sleep(int(durations[activeMember]))#die richtige pumpdauer aus dem config file inputcsv
		

client.disconnect()

