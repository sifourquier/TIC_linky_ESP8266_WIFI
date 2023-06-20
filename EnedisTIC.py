#!/usr/bin/env python3

import serial, requests,sys
from datetime import datetime

url = "http://serveur.local:8080/rest/"
ser = serial.Serial('/dev/ttyEnedisTIC',1200,7)
#ser = serial.Serial('/dev/ttyEnedisTIC',1200)
#ser = serial.Serial('/dev/ttyEnedisTIC',9600)

base = 0
papp = 0

lf = False
label = b""
value = b""
data = b""

error = 1

while 1 :
    try :
        b = ser.read(1)
        if lf :
            if b == b'\r' :
                
                #~ print ((sum(data[:-2]) & 0x3f) + 0x20 , data[-1])
                if (sum(data[:-2]) & 0x3f) + 0x20 == data[-1] :
                    #print ("sum ok - data: ",data, " - sum:", (sum(data[:-2]) & 0x3f) + 0x20," == ", data[-1])
                    label,value = data[:-2].split(b' ',2)
                    #~ print (label,value)
                    if label == b"BASE" and value != base :
                        base = value
                        print (datetime.isoformat(datetime.now()), "BASE", base)
                        requests.post(url+"items/CompteurElecBase",base[:-3].lstrip(b"0")+b"."+base[-3:])
                    if label == b"PAPP" and value != papp :
                        papp = value
                        print (datetime.isoformat(datetime.now()), "PAPP", papp)
                        requests.post(url+"items/CompteurElecPApp",papp.lstrip(b"0"))
                else :
                    print ("checksum error ", error, "/20 - data: \"",data, "\" - sum:", (sum(data[:-2]) & 0x3f) + 0x20," != ", data[-1])
                    error += 1
                lf = False
                data = b""
            else :
                data += b
        else :
            if b == b'\n' :
                lf = True
                data = b""
        if error >= 20 :
            print ("to many error reset serial port")
            error = 1
            ser.close()
            ser.open()
    except Exception as e :
        print(e.message, e.args)

