//
//  reciever.cpp
//
//
//  Created by Hugh Lavery on 19/02/2017.
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

#define CRC16 0x8005;
#define POLY 0x8408

const int PAYLOADLENGTH = 8;

using namespace std;

struct dataFrame{
    int seq;
    string data;
    bitset <8*2> header; //sequence number, Payload Length
    bitset <PAYLOADLENGTH*8> coredata;
    bitset <16> trailer; //checksum
    dataFrame* next;
    int sourceCRC;
};
/*
uint16_t CRC16alg(char* data, int size){
    
    cout << "input crc data: " << data << endl;
    uint16_t out =0;
    int bitsRead = 0, bitFlag;
    
    if(!data){
        return 0;
    }
    
    while (size >0){
        
        bitFlag = out >> 15;
        
        //read next bit
        out <<= 1;
        out |= (*data >> bitsRead) & 1;  // |= bitwise or
        
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
  
    return crc;
}
*/
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

int main(){
    int serverSocket, serverConnectionSocket, portNum, clientAddrLen, n;
    char buffer[256];
    char ack[3];
    char nak[3];
    struct sockaddr_in serverAddr, clientAddr;
    int count =1;
    int seqNumint =0;
    int lengthNumint=0;
    string headerStr;
    string seqNumStr;
    string lengthNumStr;
    string ACK = "ACK";
    string NAK = "NAK";
    portNum = 12000;
    string data;
    int QUE =0;
    dataFrame* head = NULL;
    string coredata;
    string crcStr;
    int crcNumint;
    string coretempStr;
    char tempcharArray[PAYLOADLENGTH];
    string characterTemp;
    unsigned short crc;
    string checksum;
    int checksumint;
    
    strcpy(ack, ACK.c_str());
    strcpy(nak, NAK.c_str());
    
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0)
    {
        printf("Error: Could not open socket");
        return -1;
    }
    
    bzero((char*) &serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(portNum);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    
    if(bind(serverSocket, (struct sockaddr*) &serverAddr, sizeof(serverAddr)) < 0)
    {
        printf("Error: Socket is in use\n");
        return -1;
    }
    listen(serverSocket, 5);
    
    clientAddrLen = sizeof(clientAddr);
    
    serverSocket = accept(serverSocket, (struct sockaddr*) &clientAddr, (socklen_t *)&clientAddr);
    
    cout << "test" << endl;
    while(count < 129)
    {
        while(QUE < 5){
            usleep(1000);
            bzero(buffer, 256);
            n = recvfrom(serverSocket, &buffer, sizeof(buffer), 0, (struct sockaddr*) &clientAddr, (socklen_t *)&clientAddr);
            if(n < 0)
            {
                printf("Error: Could not read from socket");
                return -1;
            }
            
            //printf("Frame: %i \n", count);
            //cout << buffer << endl;
            
            data = buffer;
            //seq num
            seqNumStr = data.substr(0,8);
            bitset<8> seqNum(seqNumStr);
            seqNumint =(int)(seqNum.to_ulong());
            cout << seqNumint <<endl;
            
            //length of input data
            lengthNumStr = data.substr(8,8);
            bitset<8> lengthNum(lengthNumStr);
            lengthNumint = (int)(lengthNum.to_ulong());
            cout << "Length: " << lengthNumint << endl;
            
            //data extraction
            coredata = data.substr(16,(lengthNumint));
            bitset<64> coreBits(coredata);
            cout << "Data: " << coredata << endl;
            
            //crc extraction
            crcStr = data.substr((16+lengthNumint), 16);
            bitset<16> crcNum(crcStr);
            crcNumint = (int)crcNum.to_ulong();
            cout << "crc: " << crcNumint << endl << endl;
            
            //store in buffer
            if (QUE == 0){
                head = new dataFrame;
                head->data = data;
              //  head->header = seqNum + lengthNum;
                head->coredata = coreBits;
                head->trailer = crcNumint;
                head->seq = seqNumint;
                head->next = NULL;
                head->sourceCRC = crcNumint;
                QUE++;
            }
            
            else if(QUE <= 5){
                dataFrame* temp = head;
                //go to end of list
                while (temp->next) {
                    temp = temp->next;
                }
                temp->next = new dataFrame;
                temp = temp->next;
                temp->data = data;
              // temp->header = seqNum + lengthNum;
                temp->coredata = coreBits;
                temp->trailer = crcNum;
                temp->seq = seqNumint;
                temp->next = NULL;
                QUE++;
            }
        }
        
        //calculate crc on next bit to be written to file
        if(head->seq == count){
            coretempStr = head->coredata.to_string();
            for (int j =0; j<PAYLOADLENGTH; j++){
                characterTemp = coretempStr.substr(PAYLOADLENGTH*j, 8);
                bitset <8> tempchar(characterTemp);
                tempcharArray[j] = char(tempchar.to_ulong());
            }
            coretempStr = tempcharArray;
            cout << "the first 8 characters are: " << coretempStr << endl;
           // unsigned char* buf= (unsigned char *) tempcharArray;
            //bitset <16> sourceCRCBits;
            crc = (crc16(tempcharArray, 8));
            bitset <16> sourceCRCBits(crc);
            checksum = to_string(crc);
            checksumint = (int)sourceCRCBits.to_ulong();
           
            printf("source: %i \ncalculated here: %d", head->sourceCRC, crc);
            if (checksumint == head->sourceCRC){
                cout << "success crcs are the same for frame" <<endl;
            }
        }
        
        //if correct send back ack
        
        
        //if not send back NAK and wait for correct frame
        
        
        //write to file
        
        count++;
        bzero(buffer, 256);
        strcpy(buffer, ACK.c_str());
        n = sendto(serverSocket, &buffer, sizeof(buffer), 0, (struct sockaddr*) &clientAddr, sizeof(clientAddr));
        
        if(n < 0)
        {
            printf("Error: Could not write to socket");
        }
        
        
    }
    cout << endl << head->seq << ", " << head->next->seq << ", " << head->next->next->seq << endl;
    return 0;
    
}
