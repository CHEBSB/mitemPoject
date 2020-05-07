import numpy as np
import serial
import time

waitTime = 1.0

# generate the song List
song0 = "cxcxgxgxaxaxgyswfxfxexexdxdxcyswswswswsw\n"
song1 = "gxexeyswfxdxdyswcxdxexfxgxgxgyswswswswsw\n"
song2 = "ewfwgwcxcwcwdwewfxawCwawfwexgwfwdwgwcxsw\n"
song3 = "LwKwLwLwBwLwKxJwEwLwLwEwDwEyEwswswswswsw\n"
song4 = "awbwCybwCxExbzsxexaygwaxCxgzsxexfxCwbysw\n"
Songlist = [song0, song1, song2, song3, song4]

serdev = '/dev/ttyACM0'
s = serial.Serial(serdev)
 
line = s.readline() # Read an echo string from K66F terminated with '\n'
index = int(line[0])

if index == 9:
    print("Sending basic songlist ...")
    print("It may take about %d seconds ..." % (int(len(Songlist) * waitTime)))
    s.write(Songlist[0].encode())
    time.sleep(waitTime)
    s.write(Songlist[1].encode())
    time.sleep(waitTime)
    s.write(Songlist[2].encode())
    time.sleep(waitTime)
    s.close()
    print("Signal sended")
elif index <= 4 and index >= 0:
    print("Sending new song ...")
    print("It may take about 1 second ..." )
    s.write(Songlist[index].encode())
    time.sleep(waitTime)
    s.close()
    print("New Song arrived!")
else:
    print("Error. Unrecognizable input.")
    print("Terminate this python program." )
    s.close()