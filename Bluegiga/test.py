import bluetooth
from threading import Thread

def main_loop(bt,comport,action):

	v = bt.openConnection(comport)
	print(v)
	#b.main_loop(comport,action)
	

b = bluetooth
t = Thread(target=main_loop, args=(b,'COM14','scan'))
t.start()


	