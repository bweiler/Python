import bluetooth
import time


MOTORS_RIGHT_30 	= 0x08
MOTORS_LEFT_30      = 0x09
MOTORS_RIGHT        = 0x10
MOTORS_LEFT         = 0x11
MOTORS_FORWARD      = 0x12
MOTORS_BACKWARD     = 0x13
MOTORS_STOP         = 0x14
STOP_TURNING        = 0x15
MOTORS_SLEEP        = 0x16
MOTORS_SPEED        = 0x50
PLAY_BUZZER         = 0x17

DEC_STEP_MODE       = 0x18
INC_STEP_MODE       = 0x19
GET_AMBIENT         = 0x21
GET_DISTANCE        = 0x22
CONNECT_DISCONNECT  = 0x23
RECORD_SOUND        = 0x30
INCREASE_GAIN       = 0x31
DECREASE_GAIN       = 0x32
RECORD_SOUND_PI     = 0x33
ROVER_MODE          = 0x40
FOTOV_MODE          = 0x41
ROVER_MODE_REV      = 0x42

comport = 'COM14'
#action = 'info'
#action = 'scan'
action = 'connect'
addr0 = 'dd:45:d3:20:c8:1b'
addr1 = 'f8:f7:81:eb:0f:66'
puck = "f3:2a:d6:d5:d3:9f"

done_op_flag = 0
data_ready = 0
return_data = 0


def distance_measure(b):
   global data_ready, return_data
   data_ready = 0
   b.sendCMD(GET_DISTANCE);   
   while True:
      time.sleep(0.010)
      if data_ready == 1:
         break  
   return return_data

def ambient_measure(b):
   global data_ready, return_data
   data_ready = 0
   b.sendCMD(GET_AMBIENT);   
   while True:
      time.sleep(0.010)
      if data_ready == 1:
         break  
   return return_data
   

def blue_callback(lib_flag):
    global done_op_flag
    if lib_flag == 1:
      done_op_flag = 1

def data_callback(data):
    global data_ready, return_data
    return_data  = data
    data_ready = 1

b = bluetooth

print('Open connection, start receieve thread')
v = b.openConnection(comport,action,addr1,blue_callback,data_callback)

if v == 0:
   done_op_flag = 0
   while True:
      time.sleep(0.020)
      if done_op_flag == 1:
         break  
   print('Distance = ',ambient_measure(b))
   b.sendCMD(MOTORS_RIGHT_30)
   time.sleep(0.1)
   b.sendCMD(MOTORS_STOP);
   time.sleep(2)
   b.closeConnection()
   print('Called close')
   b.exitLibrary()
   print('Exit')
  
else:
   print('Open returned ',v)





	