//
//  test.cpp
//  
//
//  Created by Hugh Lavery on 13/02/2017.
//
//

//
//  transmitter.c
//
//
//  Created by Hugh Lavery on 11/02/2017.
//
//

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <iostream>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define CRC16 0x8005;

const int INPUT = 1024;
const int PAYLOADLENGTH = 8;
const int MAXPACKETS = INPUT/PAYLOADLENGTH;

using namespace std;

uint16_t CRC16alg(const uint8_t *data, uint16_t size){
    
    uint16_t out =0;
    int bitsRead = 0, bitFlag =0;
    
    if(!data){
        return 0;
    }
    
    while (size >0){
        
        bitFlag = out >> 15;
        
        //read next bit
        out <<=1;
        out |= (*data >> (7 - bitsRead)) & 1;  // |= bitwise or
        
        //increment bit counter
        bitsRead++;
        
        if(bitsRead > 7){
            bitsRead =0;
            data++;
            size--;
        }
        
        if(bitFlag){
            out ^= CRC16;
        }
    }
    return out;
}

char gremlin(char data){
    int num = 0;
    num = rand()%20;
    
    if (num < 5){
        data = ' ';
    }
    return data;
}


int main(){
    char data[INPUT]; //int / bit array?
    FILE *fp;
    int clientSocket, portNum, n;
    char buffer[256];
    char rawdata[PAYLOADLENGTH];
    uint16_t crc;
    string datatosend;
    
    
    struct sockaddr_in serverAddr;
    struct hostent *serverName;
    
    portNum = 12000;
    
    //reading in data to be transmitted
    fp = fopen("ASCIIdata.txt", "r");
    
    for (int i = 0; i<1024; i++){
        data[i] = fgetc(fp);
    }
    
    //connecting to socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0)
    {
        printf("Error: Unable to open the socket");
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
    
    //conecting to reciever
    serverAddr.sin_port = htons(portNum);
    
    /*
     if (connect(clientSocket,(struct sockaddr *) &serverAddr,sizeof(serverAddr)) < 0)
     {
     printf("ERROR connecting");
     return -1;
     }
     */
    
    // prepare data to send
    
    for (int i = 0; i < MAXPACKETS; i++){
        
        //split data into frame to be sent
        for(int j=0; j < PAYLOADLENGTH; j++){
            rawdata[j] = data[(PAYLOADLENGTH*i)+ j];
        }
        
        //create checksum
        crc = (int)CRC16alg((uint8_t*)rawdata, PAYLOADLENGTH);
        printf("crc %i\n", crc);
        
        datatosend = string(rawdata);
        datatosend = datatosend; //'crc'; how do i store chars and ints in one buffer to send
        
        cout << "raw data" << datatosend << "\n";
    }
    
    
    
    return 0;
}
