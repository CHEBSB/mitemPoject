import numpy as np
import serial
import time

waitTime = 0.02

# generate the waveform table
signal1 = "ccggaabsffeeddasggffeedsggffeedsccggaabsffeeddas"
signal2 = "geesfddscdefgggdgeesfddsceggcsssdddddefseeeeefgs"
signal3 = "eaagecdegagdedecdbagageeaagecdegagdedecdbagageas"

# output formatter
formatter = lambda x: "%c" % x

# send the waveform table to K66F
serdev = '/dev/ttyACM0'
s = serial.Serial(serdev)
print("Sending signal ...")
print("It may take about %d seconds ..." % (int(144 * waitTime)))
for data in signal1:
  s.write(bytes(formatter(data), 'UTF-8'))
  time.sleep(waitTime)
for data in signal2:
  s.write(bytes(formatter(data), 'UTF-8'))
  time.sleep(waitTime)
for data in signal3:
  s.write(bytes(formatter(data), 'UTF-8'))
  time.sleep(waitTime)
s.close()
print("Signal sended")