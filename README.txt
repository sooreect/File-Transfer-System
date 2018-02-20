Author: Tida Sooreechine
Date: 6/6/2017
Program: CS372 Program 2
Description: A simple file transfer system, consisting of a server program, written in C, and a 
	client program, written in Python3

————————————————————————————————————————————————————————————————————————————————————————————————————————————————

Steps to get the file transfer system started:

1.	To get the file transfer server running, activate the makefile with the following command:
		make

2.	Execute the program with the following command: 
		./ftserver [serverPort]

3. 	After the server has successfully started running, on a separate terminal window, run
	the client program with the following command:
		python3 ftclient.py [serverHost] [serverPort] [command] [fileName - optional] [dataPort]

4.	To quit the program, press CTRL+Z into either terminal.

