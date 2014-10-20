#!/usr/bin/python
import subprocess
import time
import datetime

def server_cycle():
	while True:
		args = ['./pp_server_static', '-v=4']
		p = subprocess.Popen(args)
		time.sleep(2)
		#st = datetime.datetime.fromtimestamp(time.time()).strftime('%Y-%m-%d %H:%M:%S')
		#print st
		p.send_signal(2)
		#time.sleep(2)

if __name__ == '__main__':
	server_cycle()
