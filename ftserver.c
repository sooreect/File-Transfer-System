/**************************************************************************************************
 * Author: Tida Sooreechine
 * Date: 6/5/2017
 * Program: CS372 Program 2, Part 1
 * Description: A simple file transfer server.
**************************************************************************************************/

//references: 
//OSU CS344 Block 4 Lecture 3, "Network Servers" - Professor B. Brewster
//note: other references are listed inside the program


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <dirent.h>
#include <fcntl.h>


//declare client struct
struct Client {
	int command;
	int dataPort;
	char fileName[999];
};


//function prototypes
int startUp(int serverPort);
void handleRequest(int socketFD, struct Client* c);
void listDirectory(int socketFD);
void sendFile(int socketFD, struct Client *c);


//main function
int main(int argc, char *argv[])
{
	int serverPort, listenSocketFD, controlSocketFD, dataSocketFD, charsSent, charsReceived;
	socklen_t clientInfoSize;
	struct sockaddr_in clientAddress;

	//check usage and command-line arguments
	if (argc != 2) {
		printf("Usage: ./ftserver [port]\n");
		exit(0);
	}
	serverPort = atoi(argv[1]);	//convert user-input port number from string to integer
	if (serverPort < 1024 || serverPort > 65535) {
		printf("ERROR: Server port number out of range (1024 - 65535).\n");
		exit(0);	
	}

	//create a TCP network socket to listen for connections
	listenSocketFD = startUp(serverPort);
	printf("File Transfer server started on port %d.\n", serverPort);

	//infinite loop
	while(1) {
		printf("Waiting for connections . . .\n");
		
		//accept control connection from client
		clientInfoSize = sizeof(clientAddress);	//get the address size for the connecting client
		controlSocketFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &clientInfoSize);	//accept connection
		if (controlSocketFD < 0) {
			printf("ERROR: Cannot accept connection.\n");
			exit(1);
		}
		printf("Connected with client.\n");
		
		//get command, data port number, and, if applicable, file name from client
		struct Client *c = malloc(sizeof(struct Client));
		handleRequest(controlSocketFD, c);

		//establish data connection with client if command is valid
		if (c->command == 1 || c->command == 2) {
			clientAddress.sin_port = htons(c->dataPort);	//save data port number in the client address struct

			//create a TCP socket
			dataSocketFD = socket(AF_INET, SOCK_STREAM, 0);
			if (dataSocketFD < 0) {
				printf("ERROR: Cannot create socket.\n");
				exit(1);
			}

			//connect with client
			if (connect(dataSocketFD, (struct sockaddr*)&clientAddress, sizeof(clientAddress)) < 0) {
				printf("ERROR: Cannot connect to client.\n");
				exit(1);
			}
			else
				printf("Data connection started on port %d.\n", c->dataPort);

			//proceed with client's request
			if (c->command == 1)
				listDirectory(dataSocketFD);	//list directory contents
			else 
				sendFile(dataSocketFD, c);	//send file contents
			
			close(dataSocketFD);	//close data socket connection with client
		}

		close(controlSocketFD);	//close control socket connection with client
	}

	close(listenSocketFD);	//close the listening socket
	return(0);
}


//function definitions

int startUp(int portNumber) {
	int socketFD;
	struct sockaddr_in serverAddress;

	//set up server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress));	//clear out struct
	serverAddress.sin_family = AF_INET;	//network-capable socket
	serverAddress.sin_port = htons(portNumber);	//store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY;	//allow connections from any IP address
	
	//create a TCP socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0);
	if (socketFD < 0) {
		printf("ERROR: Cannot create socket.\n");
		exit(1);
	}
	printf("Socket created . . .\n");

	//bind the socket to host and port
	if (bind(socketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
		printf("ERROR: Cannot bind socket.\n");
		exit(0);
	}
	printf("Socket binded to port . . .\n");

	//enable the socket to listen for connections
	listen(socketFD, 5);	//server can receive up to 5 connections
	
	return socketFD;
}

void handleRequest(int socketFD, struct Client* c) {
	char buffer[99999]; 
	int charsReceived, charsSent, position, length;
	int x = 0;

	//clear char arrays
	memset(buffer, '\0', sizeof(buffer));
	memset(c->fileName, '\0', sizeof(c->fileName));

	//get action command from client
	charsReceived = recv(socketFD, buffer, sizeof(buffer) - 1, 0);
	if (charsReceived < 0) {
		printf("ERROR: Cannot read from socket.\n");
		exit(1);
	}
	else {
		if (strcmp(buffer, "-l") == 0) {
			//send acknowledgment to client
			charsSent = send(socketFD, "1", 1, 0);
			if (charsSent < 0)
				printf("ERROR: Cannot write to socket.\n");
			
			c->command = 1;
		}
		else if (strcmp(buffer, "-g") == 0) {
			printf("Invalid command\n");
			//report invalid command error to client
			charsSent = send(socketFD, "Server: Invalid command received.", 33, 0);
			if (charsSent < 0)
				printf("ERROR: Cannot write to socket.\n");
		}
		else if (strncmp(buffer, "-g", 2) == 0) {
			//send acknowledgment to client
			charsSent = send(socketFD, "2", 1, 0);
			if (charsSent < 0)
				printf("ERROR: Cannot write to socket.\n");
	
			c->command = 2;

			//extract and store filename
			length = strlen(buffer) - 3;
			position = 4;
			while(x < length) {
				c->fileName[x] = buffer[position + x - 1];
				x++;
			}
		}
		else {
			printf("Invalid command\n");
			//report invalid command error to client
			charsSent = send(socketFD, "Server: Invalid command received.", 33, 0);
			if (charsSent < 0) {
				printf("ERROR: Cannot write to socket.\n");
				exit(1);
			}
		}
	}

	//get data port number from client - number should already be validated by client
	memset(buffer, '\0', sizeof(buffer));
	charsReceived = recv(socketFD, buffer, sizeof(buffer) - 1, 0);
	if (charsReceived < 0) {
		printf("ERROR: Cannot read from socket.\n");
		exit(1);
	}
	else 
		c->dataPort = atoi(buffer);
}

//source: https://stackoverflow.com/questions/12489/how-do-you-get-a-directory-listing-in-c
//source: https://stackoverflow.com/questions/4204666/how-to-list-files-in-a-directory-in-a-c-program
//source: http://pubs.opengroup.org/onlinepubs/009695399/functions/opendir.html
void listDirectory(int socketFD) {
	DIR *dir;
	struct dirent *dp;
	char curFile[999], buffer[999];
	int charsSent, charsReceived;

	printf("Sending directory contents . . .\n");	
	
	dir = opendir(".");	//open directory stream

	if (dir) {
		while ((dp = readdir(dir)) != NULL) {	
			//save current file name
			memset(curFile, '\0', sizeof(curFile));	 
			strcpy(curFile, dp->d_name);	

			//send file name over the connection
			charsSent = send(socketFD, curFile, strlen(curFile), 0);
			if (charsSent < 0) {
				printf("ERROR: Cannot write to socket.\n");
				exit(1);
			}

			//wait for acknowledgement from client before proceeding
			memset(buffer, '\0', sizeof(buffer));
			charsReceived = recv(socketFD, buffer, sizeof(buffer) - 1, 0);
			if (charsReceived < 0) {
				printf("ERROR: Cannot read from socket.\n");
				exit(1);
			}
		}

		//alert the client that the list is complete
		charsSent = send(socketFD, "@@@", 3, 0);	
		if (charsSent < 0) {
			printf("ERROR: Cannot write to socket.\n");
			exit(1);
		}

		closedir(dir);
		printf("Directory transfer completed.\n");
	}	
}


void sendFile(int socketFD, struct Client* c) {
	int fileFD, fileSize, charsSent, charsReceived, charsRead;
	char buffer[999999];
	char* fileContents;

	printf("Checking the directory for file %s . . .\n", c->fileName);

	//check that file exists by opening it for reading
	fileFD = open(c->fileName, O_RDONLY);	
	if (fileFD < 0)	{
		//notify client that file does not exist
		printf("File does not exist.\n");
		charsSent = send(socketFD, "Server: File does not exit.", 27, 0);
		if (charsSent < 0) {
			printf("ERROR: Cannot write to socket.\n");
			exit(1);
		}
		
		//wait for client's acknowledgement
		charsReceived = recv(socketFD, buffer, sizeof(buffer) - 1, 0);
		if (charsReceived < 0) {
			printf("ERROR: Cannot read from socket.\n");
			exit(1);
		}
	} 
	else {
		//notify client that file exists
		printf("File exists. Proceeding with file transfer . . .\n");
		charsSent = send(socketFD, "Sending", 7, 0);
		if (charsSent < 0) {
			printf("ERROR: Cannot write to socket.\n");
			exit(1);
		} 
		
		//wait for client's acknowledgement
		charsReceived = recv(socketFD, buffer, sizeof(buffer) - 1, 0);
		if (charsReceived < 0) {
			printf("ERROR: Cannot read from socket.\n");
			exit(1);
		}
		//send file size
		memset(buffer, '\0', sizeof(buffer));	//clear out buffer
		fileSize = lseek(fileFD, 0, SEEK_END);	//get character count
		sprintf(buffer, "%d", fileSize);	//convert size from integer to string
		charsSent = send(socketFD, buffer, strlen(buffer), 0);	//write to client
		if (charsSent < 0) {
			printf("ERROR: Cannot write to socket.\n");
			exit(1);
		}

		//get receipt from client
		memset(buffer, '\0', sizeof(buffer));	//clear out buffer
		charsReceived = recv(socketFD, buffer, sizeof(buffer) - 1, 0);
		if (charsReceived < 0) {
			printf("ERROR: Cannot read from socket.\n");
			exit(1);
		}

		//read file contents into buffer
		//source: https://stackoverflow.com/questions/14002954/c-programming-how-to-read-the-whole-file-contents-into-a-buffer
		lseek(fileFD, 0, SEEK_SET);	//relocate file pointer to beginning of file
		fileContents = malloc(fileSize + 1);	//allocate space to store file contents
		charsRead = read(fileFD, fileContents, fileSize);	//read file to buffer
		if (charsRead < 0) {
			printf("ERROR: Cannot read file.\n");
			exit(1);
		}

		int sendPosition = 0;
		//transfer file from buffer to client
		while (fileSize > 0) {
			memset(buffer, '\0', sizeof(buffer));	//clear out buffer
			if (fileSize > sizeof(buffer))
				memcpy(buffer, &fileContents[sendPosition], sizeof(buffer) - 1);
			else
				memcpy(buffer, &fileContents[sendPosition], fileSize);

			//write to client
			charsSent = send(socketFD, buffer, strlen(buffer), 0);
			if (charsSent < 0) {
				printf("ERROR: Cannot send file contents.\n");
				exit(1);
			}
			fileSize -= charsSent;
			sendPosition += charsSent;
		}
		printf("File transfer completed.\n");
	}
	
	close(fileFD);
}

