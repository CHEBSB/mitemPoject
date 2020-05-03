import numpy as np
import serial
import time

waitTime = 0.2

# generate the waveform table
#signal1 = "ccggaabsffeeddasggffeedsggffeedsccggaabsffeeddas"
signal1 = "cdefgabagfedc"
#signal2 = "geesfddscdefgggdgeesfddsceggcsssdddddefseeeeefgs"
signal2 = "cceeggaacceeg"
#signal3 = "eaagecdegagdedecdbagageeaagecdegagdedecdbagageas"
signal3 = "abcabcabcabca"
# output formatter
formatter = lambda x: "%c\r\n" % x

# send the waveform table to K66F
serdev = '/dev/ttyACM0'
s = serial.Serial(serdev)
print("Sending signal ...")
print("It may take about %d seconds ..." % (int(len(signal1) * 3 * waitTime)))
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