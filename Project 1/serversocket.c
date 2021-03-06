//
//  serversocket.c
//  
//
//  Created by Hugh Lavery on 12/02/2017.
//
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// contains definitions of a number of data types used in system calls. These types are used in the next two include files
#include <sys/types.h>
// includes a number of definitions of structures needed for sockets
#include <sys/socket.h>
// contains constants and structures needed for internet domain addresses.
#include <netinet/in.h>

int main()
{
    // serverSocket and serverConnectionSocket are file descriptors, more information: http://www.linuxhowtos.org/data/6/fd.txt
    // n is the return value for read() and write() ie how many characters have been read or written
    int serverSocket, serverConnectionSocket, portNum, clientAddrLen, n;
    char buffer[256];
    // sockaddr_in is a struct defined in in.h, it contains internet address information
    struct sockaddr_in serverAddr, clientAddr;
    
    portNum = 12000;
    
    // creates a new socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0)
    {
        printf("Error: Could not open socket");
        return -1;
    }
    
    // zeroes out the pointed to buffer, will need to change this to bit* instead of char*?
    bzero((char*) &serverAddr, sizeof(serverAddr));
    
    // filling in the fields for the server address. htons() is carrying out little endian to big endian conversion.
    // more information: http://www.linuxhowtos.org/data/6/byteorder.html
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(portNum);
    // INADDR_ANY is the IP address of the server
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    
    // binds the server socket to the server address. serverAddr of type sockaddr_in is cast to sockaddr
    if(bind(serverSocket, (struct sockaddr*) &serverAddr, sizeof(serverAddr)) < 0)
    {
        printf("Error: Socket is in use");
        return -1;
    }
    
    // listens for activty on server socket. 5 is the number of connections allowed to wait while
    // a connection is begin processed i.e. backlog queue
    listen(serverSocket, 5);
    
    clientAddrLen = sizeof(clientAddr);
    
    serverConnectionSocket = accept(serverSocket, (struct sockaddr*) &clientAddr, &clientAddrLen);
    
    if(serverConnectionSocket < 0)
    {
        printf("Error: Could not accept");
        return -1;
    }
    
    while(1)
    {
        bzero(buffer, 256);
        n = read(serverConnectionSocket, buffer, 255);
        if(n < 0)
        {
            printf("Error: Could not read from socket");
            return -1;
        }
        
        printf("Here is the message %s", buffer);
        
        n = write(serverConnectionSocket, "Got your message", 18);
        
        if(n < 0)
        {
            printf("Error: Could not write to socket");
        }
    }
    
    return 0;
}
