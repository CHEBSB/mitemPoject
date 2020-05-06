import numpy as np
import serial
import time

waitTime = 1.0

# generate the song List
signal1 = "cxcxgxgxaxaxgyswfxfxexexdxdxcyswswswswsw\n"
signal2 = "gxexeyswfxdxdyswcxdxexfxgxgxgyswswswswsw\n"
signal3 = "ewfwgwcxcwcwdwewfxawCwawfwexgwfwdwgwcxsw\n"

# send the waveform table to K66F
serdev = '/dev/ttyACM0'
s = serial.Serial(serdev)
print("Sending signal ...")
print("It may take about %d seconds ..." % (int(3 * waitTime)))

s.write(signal1.encode())
time.sleep(waitTime)
s.write(signal2.encode())
time.sleep(waitTime)
s.write(signal3.encode())
time.sleep(waitTime)
s.close()
print("Signal sended")