import numpy as np
import serial
import time

waitTime = 1.0

# generate the song List
song1 = "cxcxgxgxaxaxgyswfxfxexexdxdxcyswswswswsw\n"
song2 = "gxexeyswfxdxdyswcxdxexfxgxgxgyswswswswsw\n"
song3 = "ewfwgwcxcwcwdwewfxawCwawfwexgwfwdwgwcxsw\n"
song4 = "LwKwLwLwBwLwKxJwEwLwLwEwDwEyEwswswswswsw\n"
song5 = "awbwCybwCxExbzsxexaygwaxCxgzsxexfxCwbysw\n"
Songlist = [song1, song2, song3, song4, song5]

serdev = '/dev/ttyACM0'
s = serial.Serial(serdev)
 
line = s.readline() # Read an echo string from K66F terminated with '\n'
# print line

if line[0] == '9':
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
else:
    print("Sending new song ...")
    print("It may take about 1 second ..." )
    s.write(Songlist[int(line[0])].encode())
    time.sleep(waitTime)
    s.close()
    print("New Song arrived!")