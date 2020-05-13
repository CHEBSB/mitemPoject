import serial
import time

# generate the song List
song0 = "cxcxgxgxaxaxgyswfxfxexexdxdxcyswswswswsw\n"
song1 = "gxexeyswfxdxdyswcxdxexfxgxgxgyswswswswsw\n"
song2 = "ewfwgwcxcwcwdwewfxawCwawfwexgwfwdwgwcxsw\n"
song3 = "LwKwLwLwBwLwKxJwEwLwLwEwDwEyEwswswswswsw\n"
song4 = "awbwCybwCxExbzsxexaygwaxCxgzsxexfxCwbysw\n"
Songlist = [song0, song1, song2, song3, song4]

serdev = '/dev/ttyACM0'
s = serial.Serial(serdev)
 
line = s.read() # Read 
print("Here PC receive: ", line)
if line[0] <= 4 and line[0] >= 0:
    print("Sending new song: Song ", line[0])
    print("It may take about 1 second ..." )
    s.write(Songlist[line[0]].encode())
    time.sleep(waitTime)
    s.close()
    print("New Song arrived!")
else:
    print("Sending basic songlist ...")
    print("It may take about 3 seconds ...")
    s.write(Songlist[0].encode())
    time.sleep(waitTime)
    s.write(Songlist[1].encode())
    time.sleep(waitTime)
    s.write(Songlist[2].encode())
    time.sleep(waitTime)
    s.close()
    print("Signal sended")