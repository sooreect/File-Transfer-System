#!/usr/bin/env python3

#--------------------------------------------------------------------------------------------------
# Author: Tida Sooreechine
# Date: 6/5/2017
# Program: CS372 Program 2, Part 2
# Description: A simple file transfer client
#--------------------------------------------------------------------------------------------------

#references:
#OSU CS344 Block 4 Lecture 2, "Network Clients"
#OSU CS372 Lecture 12, "The Application Layer: File Transfer Protocol (ftp) & Email Protocols"
#OSU CS372 Lecture 15, "Socket Programming"
#https://www.tutorialspoint.com/python3/index.htm
#https://docs.python.org/3.3/howto/sockets.html
#https://docs.python.org/2/howto/sockets.html
#http://www.binarytides.com/python-socket-programming-tutorial/


import socket	#import socket module
import sys		#import sys module
import os		#import os module


def createControlSocket(hostName, portNumber):
	#create a client socket object using TCP connection
	socketFD = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	print("Control socket created . . .")
		
	#connect socket to server
	socketFD.connect((hostName, portNumber))
	print("You are now connected to server.")

	return socketFD


def createDataSocket(portNumber):
	#create a server socket object using TCP connection
	socketFD = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	print("Data socket created . . .")

	#bind the socket to host and port
	HOST = ''	#socket is reachable by any address the machine has
	PORT = portNumber #data port number
	try:
		socketFD.bind((HOST, PORT))
	except socket.error:
		print("ERROR: Cannot bind data socket")
		exit(1)
	print("Data socket binded . . .")
	
	#enable the socket to listen for connection
	socketFD.listen(1)
	
	return socketFD


def makeRequest(socketFD, command, portNumber):
	#send command to server
	#3 possible commands: list directory, get file, and invalid request
	sendString = command.encode("utf-8")	
	socketFD.send(sendString)

	#receive acknowledgement from server
	recvByte = socketFD.recv(100)
	recvString = recvByte.decode("utf-8") 
	if "1" in recvString:
		command = 1
	elif "2" in recvString:
		command = 2
	else:
		print(recvString)
		command = -1

	#send data port number to server
	portString = str(portNumber);
	sendByte = portString.encode("utf-8")
	socketFD.send(sendByte);

	return command


def getDirectory(socketFD):
	print("Directory contents: ")
	
	while 1:
		#receive message from server 
		recvByte = socketFD.recv(1000) 
		recvString = recvByte.decode("utf-8")

		#break out of loop if message contains terminating characters @@@
		#otherwise, print received data to screen and send acknowledgement to server
		if "@@@" in recvString:
			break
		else:
			#print data
			print(recvString)
		
			#send receipt acknowledgement
			sendString = "OK"
			sendByte = sendString.encode("utf-8")
			socketFD.send(sendByte)
				
	print("Directory listing completed.")	


def getFile(socketFD, fileName):
	#get server notification of file existence
	recvByte = socketFD.recv(1000)
	recvString = recvByte.decode("utf-8")
	if "Server" in recvString:
		print(recvString)

		#send receipt acknowledgement
		sendString = "OK"
		sendByte = sendString.encode("utf-8")
		socketFD.send(sendByte)
	else:
		#send receipt acknowledgement
		sendString = "OK"
		sendByte = sendString.encode("utf-8")
		socketFD.send(sendByte)
		
		#get file size from server
		recvByte = socketFD.recv(1000)
		recvString = recvByte.decode("utf-8")
		fileLength = int(recvString)

		#send receipt acknowledgement
		sendString = "OK"
		sendByte = sendString.encode("utf-8")
		socketFD.send(sendByte)

		#set up file to write received contents to
		#source: https://docs.python.org/2/tutorial/inputoutput.html
		#source: https://www.tutorialspoint.com/python/os_listdir.htm
		dir = os.listdir(".")
		for file in dir:
			if (file == fileName):
				fileName = "new" + fileName
		fileHandle = open(fileName, 'wb')		

		#receive file contents and write to file	
		#source: https://stackoverflow.com/questions/27241804/sending-a-file-over-tcp-sockets-in-python
		fc = socketFD.recv(1024)
		while (fc):
			fileHandle.write(fc)
			fc = socketFD.recv(1024)

		fileHandle.close()
		print("File transfer completed.")


def main(argv):
	#check usage and command line arguments
 	#note: client can specify either 5 or 6 arguments
	#if command is -l (list), there should be a total of 5 arguments
	#if command is -g (get), there should be a total of 6 arguments, with extra argument being the filename 
	#source: http://www.pythonforbeginners.com/system/python-sys-argv
	if len(sys.argv) < 5 or len(sys.argv) > 6: 
		print("Usage: python3 ftclient.py [server_host] [server_port] [command] [filename] [data_port]")
		exit(1)
	
	serverName = sys.argv[1]
	serverPort = int(sys.argv[2])
	if len(sys.argv) == 5:
		command = sys.argv[3]
		dataPort = int(sys.argv[4])
	else:
		command = sys.argv[3] + " " + sys.argv[4]
		dataPort = int(sys.argv[5])
		fileName = sys.argv[4]

	if serverPort < 1024 or serverPort > 65535: 
		print("ERROR: Server port number out of range (1024-65535).")
		exit(1)
	if dataPort < 1024 or dataPort > 65535:
		print("ERROR: Data port number out of range (1024-65535).")
		exit(1)
	if dataPort == serverPort:
		print("ERROR: Data port number cannot be equal to server port number.")
		exit(1)

	#create a network socket and connect to server	
	controlSocketFD = createControlSocket(serverName, serverPort)

	#send user command (list/get file) to server through control connection
	action = makeRequest(controlSocketFD, command, dataPort)
 	
	if action == 1 or action == 2:
		#create a new socket to listen for data connection request from server
		listenSocketFD = createDataSocket(dataPort)
		print("Data connection started on port " + str(dataPort))
		print("Waiting for connection from server . . .")

		running = 1
		while running == 1: 
			#establish data connection with server
			dataSocketFD, serverAddress = listenSocketFD.accept()
		
			if action == 1:
				#list directory contents received sent by server
				print("Requesting directory contents . . .")
				getDirectory(dataSocketFD)
				running = 0
			else:
				#get file from server
				print("Requesting file contents . . .")
				getFile(dataSocketFD, fileName)
				running = 0

			dataSocketFD.close()
		listenSocketFD.close()
	controlSocketFD.close()


if __name__ == "__main__":
	main(sys.argv)

