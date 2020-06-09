import time
import datetime

  
now = datetime.datetime.now()
        
if (sub < '840'):
    print("zu trocken")
    starttime = now
    stoptime = starttime + datetime.timedelta(seconds=10) //man kann auch minutes, hours etc angeben; falls nicht angegeben -> 0
    pub = "p"
    print("Pumpe gestartet")
    print(starttime, stoptime)
        
if (now > stoptime):
    pub = "s"
    print("Pumpe gestoppt")



    
        
    
        
        
    
