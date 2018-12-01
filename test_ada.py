import time
import serial
import numpy as np
import matplotlib.pyplot as plt
#might work to play sound on speaker, sound is raw int16 or maybe .wav file, graphs nice
import simpleaudio as sa

MOTORS_RIGHT_30 	= b'\x08'
MOTORS_LEFT_30      = b'\x09'
MOTORS_RIGHT        = b'\x10'
MOTORS_LEFT         = b'\x11'
MOTORS_FORWARD      = b'\x12'
MOTORS_BACKWARD     = b'\x13'
MOTORS_STOP         = b'\x14'
STOP_TURNING        = b'\x15'
MOTORS_SLEEP        = b'\x16'
MOTORS_SPEED        = b'\x50'
PLAY_BUZZER         = b'\x17'

DEC_STEP_MODE       = b'\x18'
INC_STEP_MODE       = b'\x19'
GET_AMBIENT         = b'\x21'
GET_DISTANCE        = b'\x22'
CONNECT_DISCONNECT  = b'\x23'
RECORD_SOUND        = b'\x30'
INCREASE_GAIN       = b'\x31'
DECREASE_GAIN       = b'\x32'
RECORD_SOUND_PI     = b'\x33'
ROVER_MODE          = b'\x40'
FOTOV_MODE          = b'\x41'
ROVER_MODE_REV      = b'\x42'

#these addresses aren't used
#addr0 = 'dd:45:d3:20:c8:1b'
#addr1 = 'f8:f7:81:eb:0f:66'
#puck = "f3:2a:d6:d5:d3:9f"
comport = 'COM19'
ser = 0

done_op_flag = 0
data_ready = 0
return_data = 0

def distance_measure():
   ser.write(GET_DISTANCE);   
   outs = 0
   while True:
     outs = ser.read(1)
     if len(outs) == 1:
       break
   return outs

def ambient_measure():
   ser.write(GET_AMBIENT);   
   outs = 0
   while True:
     outs = ser.read(1)
     if len(outs) == 1:
       break
   return outs
 
#The timings are kind of tough to get right and your robot may vary 
def get_sound():
   ser.write(RECORD_SOUND);   
   time.sleep(1)
   file = open("testfile.wav","w+b")
   outs = 0
   #wait until read returns 20 bytes, that is first notify
   while True:
     outs = ser.read(20)
     if len(outs) == 20:
       break
   
   #get all notify 20 byte packets, then timeout on serial port, and graph sound
   cnts = 0
   while True:		
     file.write(outs);
     cnts += 1
     if cnts % 100 == 0:
       print('cnt is ',cnts)
     outs = ser.read(20)
     if len(outs) != 20:
       break
   file.close()
   datas = np.memmap("testfile.wav", dtype='int16', mode='r')
   plt.ylim(-32000.0, 32000.0);
   x = np.linspace(0, cnts*10, cnts*10)
   plt.plot(x, datas)
   plt.show()


ser = serial.Serial(
    port=comport,
    baudrate=115200,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    bytesize=serial.EIGHTBITS
)
ser.timeout = 1          #this is timeout on read.
ser.writeTimeout = 2     #timeout for write
print('Open')
if ser.isOpen():

    try:
        ser.flushInput() #flush input buffer, discarding all its contents
        ser.flushOutput()
    except Exception as e1:
        print("error communicating...: " + str(e1))

else:
    print("cannot open serial port ")
    exit()
	
while ser.is_open == False:
	time.sleep(1)
#this is a rough timer for the Skoobot serial port to get ready
time.sleep(5)

print('Get sound')
get_sound()
print('Buzzer')	
cnt = 0	 
#play buzzer 10 times and see if evenly spaced beeps to test interface for timing
while cnt < 10:
   ser.write(PLAY_BUZZER)
   time.sleep(0.5)
   cnt += 1
  
#draw out square  
print('Forward .6')		 
ser.write(MOTORS_FORWARD)
time.sleep(0.600)
print('Left .3')		 
ser.write(MOTORS_LEFT)
time.sleep(0.300)
print('Forward .6')		 
ser.write(STOP_TURNING)
time.sleep(0.600)
print('Left .3')		 
ser.write(MOTORS_LEFT)
time.sleep(0.300)
print('Forward .6')		 
ser.write(STOP_TURNING)
time.sleep(0.600)
print('Left .3')		 
ser.write(MOTORS_LEFT)
time.sleep(0.300)
print('Forward .6')		 
ser.write(STOP_TURNING)
time.sleep(0.600)
print('Left .3')		 
ser.write(MOTORS_LEFT)
time.sleep(0.300)
print('Stop')		 
ser.write(MOTORS_STOP)
time.sleep(0.300)
ser.close()
print('Exit')
  





	