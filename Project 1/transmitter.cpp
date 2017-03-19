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


unsigned short crc16(char *data_p, unsigned short length){
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
    
    if (num == 5){
        data = ' ';
    }
    return data;
}


int main(){
    char data[INPUT], buffer[256], tempchararray[3], temp[PAYLOADLENGTH];
    FILE *fp;
    int clientSocket, portNum, n, QUE = 0, sentSuccesfully =0, i=0;
    string tempChar, core, checksum, headerT;;
    unsigned short crc;
    dataFrame* head = NULL;
    dataFrame* tempFrame = NULL;

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
        
        //fill the buffer
        while(QUE < 5){
            //split data into frame to be sent
            for(int j=0; j < PAYLOADLENGTH; j++){
                temp [j] = data[(PAYLOADLENGTH*i)+ j];
                bitset<8> coreFrame(temp[j]);
                core += coreFrame.to_string();
            }
            string tempstr = temp;
            
            //Calculate CRC16
            crc = (crc16(temp, 8));
            bitset<16> tempTrailer(crc);
            checksum = to_string(crc);
            
            
            //Prepare data to be sent
            bitset <8*PAYLOADLENGTH> tempCore(core);
            bitset<8> tempHeader1(i+1);
            bitset<8> tempHeader2(64);
            headerT = tempHeader1.to_string() + tempHeader2.to_string();
            bitset<16> tempHeader(headerT);
           
            //store in buffer until ACK recieved
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
            
            //apply gremlin function on random bit
            tempCore[18] = gremlin(tempCore[18]);
            
            //send the data to the reciever
            printf("Sending Frame: %i \n", i+1);
            
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
        
        //When the reciever sends an ACK back for a particular frame
        //remove it from the buffer or send back frame if NAK recieved
        printf("waiting for ACK/NAK\n");
        bzero(buffer,256);
        n = recvfrom(clientSocket,&buffer,sizeof(buffer),0,(struct sockaddr*) &serverAddr, (socklen_t *)&serverAddr);
        if (n < 0)
        {
            printf("ERROR reading from socket\n");
            return -1;
        }
        
        //Parse the reply
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
        
        //remove frame acknowleged from the buffer
        if(reply == "ACK"){
            if(head->seq == frameNum){
                printf("frame %i has been ACKed \n", frameNum);
                QUE--;
                sentSuccesfully++;
                //delete node from linked list
                dataFrame* p = head;
                head = head->next;
                delete p;
            }
            else{
                //check rest of linked list for frame
                tempFrame = head;
                dataFrame* t = NULL;
                while(tempFrame->seq != frameNum){
                    t = tempFrame;
                    tempFrame = tempFrame->next;
                    
                }
                if(tempFrame->seq == frameNum){
                    printf("frame %i has been ACKed \n", frameNum);
                    QUE--;
                    sentSuccesfully++;
                    //delete node
                    dataFrame* tp = tempFrame;
                    t->next = tempFrame->next;
                    delete tp;
                }
                else{
                    printf("ERROR: Frame %i Not found in list\n", frameNum);
                }
            }
        }
        //resend frame that was corrupted
        else{
            printf("Frame %i has been NAKed\n", frameNum);
            if(head->seq == frameNum){
                string buff = head->header.to_string() + head->coredata.to_string() + head->trailer.to_string();
                bzero(buffer,256);
                strcpy(buffer, buff.c_str());
                printf("Resending frame\n");
                n = sendto(clientSocket, &buffer, sizeof(buffer), 0,(struct sockaddr*) &serverAddr, sizeof(serverAddr));
                if (n < 0)
                {
                    printf("ERROR writing to socket\n");
                    return -1;
                }
            }
            else{
                tempFrame = head;
                while(tempFrame->seq != frameNum){
                    tempFrame = tempFrame->next;
                }
                if(tempFrame->seq != frameNum){
                    printf("error not in resend list\n");
                }
                else{
                    string buff = tempFrame->header.to_string() + tempFrame->coredata.to_string() + tempFrame->trailer.to_string();
                    bzero(buffer,256);
                    strcpy(buffer, buff.c_str());
                    printf("Resending frame\n");
                    n = sendto(clientSocket, &buffer, sizeof(buffer), 0,(struct sockaddr*) &serverAddr, sizeof(serverAddr));
                    if (n < 0)
                    {
                        printf("ERROR writing to socket\n");
                        return -1;
                    }
                }
            }
        }
        reply = "";
        
        
    }
    
    printf("All frames have been acknowledged\n\n");
    close(clientSocket);
    
    
    
    return 0;
}
