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
 
sin = s.read() # Read 
print("Here PC receive: ", sin)
if sin <= 4 and sin >= 0:
    print("Sending new song: Song ", sin)
    print("It may take about 1 second ..." )
    s.write(Songlist[sin].encode())
    time.sleep(1.0)
    s.close()
    print("New Song arrived!")
else:
    print("Sending basic songlist ...")
    print("It may take about 3 seconds ...")
    s.write(Songlist[0].encode())
    time.sleep(1.0)
    s.write(Songlist[1].encode())
    time.sleep(1.0)
    s.write(Songlist[2].encode())
    time.sleep(1.0)
    s.close()
    print("Signal sended")