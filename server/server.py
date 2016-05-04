#! /usr/bin/python
import socket #create socket module

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM) #create a socket module

port = 12345 #reserving a port on the local machine
s.bind(('', port)) # bind to the port

s.listen(5) #wating for client connection

while True:
	c,addr = s.accept() #establishing a connection with client
	print("Got connection from", addr)
	c.send(b"27")
	c.close()#close the connection 
