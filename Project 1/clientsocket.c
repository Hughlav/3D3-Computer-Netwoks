//
//  clientsocket.c
//  
//
//  Created by Hugh Lavery on 12/02/2017.
//
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> // Defines the hostent struct

int main()
{
    int clientSocket, portNum, n;
    char buffer[256];
    
    struct sockaddr_in serverAddr;
    struct hostent *serverName;
    
    portNum = 12000;
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    
    if (clientSocket < 0)
    {
        printf("Error: Could not open socket");
        return -1;
    }
    
    serverName = gethostbyname("localhost"); // host name
    
    bzero((char *) &serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    
    bcopy(
          (char *)serverName->h_addr,
         	(char *)&serverAddr.sin_addr.s_addr,
         	serverName->h_length
          );
    
    serverAddr.sin_port = htons(portNum);
    
    if (connect(clientSocket,(struct sockaddr *) &serverAddr,sizeof(serverAddr)) < 0)
    {
        printf("ERROR connecting");
        return -1;
    }
    
    
    while(1)
    {
        printf("Please enter the message: ");
        bzero(buffer,256);
        fgets(buffer,255,stdin);
        
        n = write(clientSocket, buffer, strlen(buffer));
        if (n < 0)
        {
            printf("ERROR writing to socket\n");
            return -1;
        }
        
        bzero(buffer,256);
        n = read(clientSocket,buffer,255);
        if (n < 0)
        {
            printf("ERROR reading from socket\n");
            return -1;
        }
        printf("%s\n",buffer);
    }
    
    close(clientSocket);
    return 0;
}
