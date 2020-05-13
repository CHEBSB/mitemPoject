import serial
import time

# generate the song List
song0 = "cxcxgxgxaxaxgyswfxfxexexdxdxcyswswswswsw\n"
song1 = "gxexeyswfxdxdyswcxdxexfxgxgxgyswswswswsw\n"
song2 = "ewfwgwcxcwcwdwewfxawCwawfwexgwfwdwgwcxsw\n"
song3 = "axaxbyswaxaxbyswaxbxCxDwEwDxaxDyswaxexsw\n"
song4 = "awbwCybwCxExbzsxexaygwaxCxgzsxexfxCwbysw\n"
song5 = "LwMwAwAwBwAwMxJwEwLwLwEwDwEyswEwDwDwCwsw\n"
Songlist = [song0, song1, song2, song3, song4, song5]

serdev = '/dev/ttyACM0'
s = serial.Serial(serdev)
 
line = s.readline() # Read 
print("Here PC receive:", line)
sA = line.split()
i = len(sA) - 2
if i >= 0:
    k = int(sA[i])
    print("k:", k)
    time.sleep(1.0)
    if k <= 5 and k >= 0:
        print("Sending new song: Song ", k)
        print("It may take about 1 second ..." )
        s.write(Songlist[k].encode())
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