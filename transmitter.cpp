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
#include <string>
#include <bitset>
#include <iostream>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

#define POLY 0x8408

const int INPUT = 1024;
const int PAYLOADLENGTH = 8; //number of bytes per payload
const int MAXPACKETS = INPUT/PAYLOADLENGTH;

using namespace std;

struct dataFrame{
    bitset <8*2> header; //sequence number, Payload Length
    bitset <PAYLOADLENGTH*8> coredata;
    bitset <16> trailer; //checksum
    dataFrame* next;
    int seq;
};


unsigned short crc16(char *data_p, unsigned short length)
{
    unsigned char i;
    unsigned int data;
    unsigned int crc = 0xffff;
    
    if (length == 0)
        return (~crc);
    
    do
    {
        for (i=0, data=(unsigned int)0xff & *data_p++;
             i < 8;
             i++, data >>= 1)
        {
            if ((crc & 0x0001) ^ (data & 0x0001))
                crc = (crc >> 1) ^ POLY;
            else  crc >>= 1;
        }
    } while (--length);
    
    crc = ~crc;
    data = crc;
    crc = (crc << 8) | (data >> 8 & 0xff);
    
    return (crc);
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
    char tempchararray[3];
    string tempChar;
    unsigned short crc;
    //bitset<8*PAYLOADLENGTH> coreFrame;
    char temp[PAYLOADLENGTH];
    string core;
    string checksum;
    string headerT;
    int QUE = 0;
    int sentSuccesfully =0;
    int i=0;
    unsigned short temper;
    dataFrame* head = NULL;
    dataFrame* tempFrame = NULL;
    
    string testing = "helloooo";
    char tester[8];
    strcpy(tester, testing.c_str());
    temper = crc16(tester,8);
    printf("helloooo crc: %hu\n\n", temper);
    
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
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    
    
    if (connect(clientSocket,(struct sockaddr *) &serverAddr,sizeof(serverAddr)) < 0)
    {
        printf("ERROR connecting");
        return -1;
    }
    
    
    // prepare data to send
    
    while(sentSuccesfully < MAXPACKETS){
        
        while(QUE < 5){
            //split data into frame to be sent
            for(int j=0; j < PAYLOADLENGTH; j++){
                temp [j] = data[(PAYLOADLENGTH*i)+ j];
                bitset<8> coreFrame(temp[j]);
                core += coreFrame.to_string();
            }
            string tempstr = temp;
            cout << tempstr << endl;
            //strcpy(rawdata, core.c_str());
            
            //generate checksum for data using crc16 algorithm
           // unsigned char* buf= (unsigned char *) temp;
            //bitset<16> tempTrailer;
            
            crc = (crc16(temp, 8));
            bitset<16> tempTrailer(crc);
            //crc = (int)tempTrailer.to_ulong();
            checksum = to_string(crc);
            
            cout << "Frame" << i+1 << ": " << core << "\n";
            printf("Checksum %i\n", crc);
            printf("chars %s\n", temp);
            
            bitset <8*PAYLOADLENGTH> tempCore(core);
            //headerT1 = to_string(i+1);
            //headerT2 = to_string(64);
            
            bitset<8> tempHeader1(i+1);
            bitset<8> tempHeader2(64);
            headerT = tempHeader1.to_string() + tempHeader2.to_string();
            
            bitset<16> tempHeader(headerT);
            cout << tempHeader << "\n";
            
            
            cout << tempTrailer<< "\n";
            /*
            dataFrame dataPacket;
            dataPacket.coredata = tempCore;
            dataPacket.header = tempHeader;
            dataPacket.trailer = tempTrailer;
            dataPacket.seq = i+1;
            dataPacket.next = NULL;
            */
            
            //store in buffer
            if(QUE == 0){
                head = new dataFrame;
                head->coredata = tempCore;
                head->header = tempHeader;
                head->trailer = tempTrailer;
                head->seq = i+1;
                head->next = NULL;
                QUE++;
            }
            else{
                dataFrame* temp = head;
                //go to end of list
                while (temp->next) {
                    temp = temp->next;
                }
                temp->next = new dataFrame;
                temp = temp->next;
                temp->coredata = tempCore;
                temp->header = tempHeader;
                temp->trailer = tempTrailer;
                temp->seq = i+1;
                temp->next = NULL;
                QUE++;
            }
            
            core = "";
            checksum= "";
            crc = 0;
            
            //send the data
            string buff = headerT + tempCore.to_string() + tempTrailer.to_string();
            bzero(buffer,256);
            strcpy(buffer, buff.c_str());
            n = sendto(clientSocket, &buffer, sizeof(buffer), 0,(struct sockaddr*) &serverAddr, sizeof(serverAddr));
            if (n < 0)
            {
                printf("ERROR writing to socket\n");
                return -1;
            }
            
            i++;
            cout << "\n\n";
        }
        
        printf("waiting for ACK/NAK\n");
        bzero(buffer,256);
        n = recvfrom(clientSocket,&buffer,sizeof(buffer),0,(struct sockaddr*) &serverAddr, (socklen_t *)&serverAddr);
        if (n < 0)
        {
            printf("ERROR reading from socket\n");
            return -1;
        }
        printf("Recieved ACK/NAK\n");
        string recieved(buffer);
        string reply ="";
        bitset<24> replyBits(recieved.substr(0,24));
        bitset<8> frameNumBit(recieved.substr(24,8));
        int frameNum = (int)frameNumBit.to_ulong();
        reply = replyBits.to_string();
        
        for(int i=0; i <3; i++){
            tempChar = reply.substr(i*8,8);
            bitset<8> replyChar(tempChar);
            tempchararray[i] = char(replyChar.to_ulong());
        }
        
        reply = tempchararray;
        
        printf("reply frame: %s \n", reply.c_str());
        printf("frame number %d \n", frameNum);
        
        
        //wait for ACK / NAK and deal with accordingly correctly delete /resend node
        
        printf("%s frame: %i\n", reply.c_str(), frameNum);
        
        if(reply == "ACK"){
            printf("head seq num %i \n", head->seq);
            if(head->seq == frameNum){
                printf("frame %i has been ACKed \n", frameNum);
                QUE--;
                sentSuccesfully++;
                //delete node
            }
            else{
                printf("not at head\n");
                tempFrame = head;
                while(tempFrame->seq != frameNum){
                    tempFrame = tempFrame->next;
                }
                if(tempFrame->seq == frameNum){
                    printf("frame %i has been ACKed \n", frameNum);
                    QUE--;
                    sentSuccesfully++;
                    //delete node
                }
                else{
                    printf("Frame %i Not found in list\n", frameNum);
                }
            }
        }
        else{
            printf("reply not ACK\n");
        }
        reply = "";
        
        
    }
    
    close(clientSocket);
    
    
    
    return 0;
}
