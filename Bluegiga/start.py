import pygatt
import logging

CMD_STARTRIGHT    = 0x10
CMD_STARTLEFT     = 0x11
CMD_STOPTURN 	  = 0x15

CMD_RIGHT_30 	  = 0x08
CMD_LEFT_30 	  = 0x09

CMD_RIGHT    = 0x10
CMD_LEFT     = 0x11
CMD_FORWARD  = 0x12
CMD_BACKWARD = 0x13
CMD_STOP     = 0x14
CMD_SLEEP    = 0x15

#    cmd_step_mode = 0x16
#    cmd_buzzer = 0x17
#    cmd_get_ambient = 0x21
CMD_GET_DISTANCE = 0x22
#    cmd_record = 0x30
#    cmd_increase_gain = 0x31
#    cmd_decrease_gain = 0x32
CMD_ROVER_MODE = 0x40

logging.basicConfig()
logging.getLogger('pygatt').setLevel(logging.DEBUG)

adapter = pygatt.BGAPIBackend()

try:
	adapter.start()
	adapter.scan()
	device = adapter.connect('ef:28:40:06:7c:b3',5,0,10,40,4000,0)
	device.char_write("00001525-1212-efde-1523-785feabcd123",CMD_RIGHT)
finally:
	adapter.stop()
	