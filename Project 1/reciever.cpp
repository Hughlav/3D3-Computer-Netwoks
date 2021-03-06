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

void writeToFile(char* characters){
    
    FILE *fp;
    fp = fopen("output.txt", "a+");
    char t;
    
    for(int i =0; i <PAYLOADLENGTH; i++){
        t = characters[i];
        fputc(t,fp);
    }

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
    int serverSocket, portNum, clientAddrLen, n, count =1, seqNumint =0, lengthNumint=0, QUE =0, crcNumint, checksumint;
    char buffer[256], tempcharArray[PAYLOADLENGTH], ack[3], nak[3];
    struct sockaddr_in serverAddr, clientAddr;
    string headerStr, seqNumStr, lengthNumStr, ACK, NAK, data, coredata, crcStr, coretempStr, characterTemp, checksum, reply;
    portNum = 12000;
    dataFrame *head = NULL;
    unsigned short crc;
    dataFrame *pTemp;
    
    FILE *fp;
    fp = fopen("output.txt", "w");
    
    ack[0] = 'A';
    ack[1] = 'C';
    ack[2] = 'K';
    
    nak[0] = 'N';
    nak[1] = 'A';
    nak[2] = 'K';
    
    //Preparing ACK and NAK replies
    for (int i=0; i<3;i++){
        bitset<8> ackBits(ack[i]);
        ACK += ackBits.to_string();
        bitset<8> nakBits(nak[i]);
        NAK += nakBits.to_string();
    }
   
    //Opening socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0){
        printf("Error: Could not open socket");
        return -1;
    }
    //Getting transmitter data
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
    //accepting incoming connect
    serverSocket = accept(serverSocket, (struct sockaddr*) &clientAddr, (socklen_t *)&clientAddr);
    
    count =1;
    while(count < 129){
        //Fill buffer
        while(QUE < 5){
            usleep(100);
            bzero(buffer, 256);
            printf("Waiting to recieve frame %i\n", count);
            //Recieving frame
            n = recvfrom(serverSocket, &buffer, sizeof(buffer), 0, (struct sockaddr*) &clientAddr, (socklen_t *)&clientAddr);
            if(n < 0)
            {
                printf("Error: Could not read from socket");
                return -1;
            }
            //Parsing Frame number from incoming header
            data = buffer;
            seqNumStr = data.substr(0,8);
            bitset<8> seqNum(seqNumStr);
            seqNumint =(int)(seqNum.to_ulong());
            
            //Parsing length of input data
            lengthNumStr = data.substr(8,8);
            bitset<8> lengthNum(lengthNumStr);
            lengthNumint = (int)(lengthNum.to_ulong());
            
            //data extraction
            coredata = data.substr(16,(lengthNumint));
            bitset<64> coreBits(coredata);
            
            //gremlin on data recieved
            coreBits[30] = gremlin(coreBits[30]);
            
            //crc extraction
            crcStr = data.substr((16+lengthNumint), 16);
            bitset<16> crcNum(crcStr);
            crcNumint = (int)crcNum.to_ulong();
            
            //store in buffer
            if (QUE == 0){
                head = new dataFrame;
                head->data = data;
                bitset<8> tempSeq(seqNum);
                bitset<8> tempLength(lengthNum);
                string headerT = tempSeq.to_string() + tempLength.to_string();
                bitset<16> headerBits(headerT);
                head->header = headerBits;
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
                bitset<8> tempSeq(seqNum);
                bitset<8> tempLength(lengthNum);
                string headerT = tempSeq.to_string() + tempLength.to_string();
                bitset<16> headerBits(headerT);
                temp->header = headerBits;
                temp->coredata = coreBits;
                temp->trailer = crcNum;
                temp->seq = seqNumint;
                temp->next = NULL;
                temp->sourceCRC = crcNumint;
                QUE++;
            }
        }
        
        //Find next Frame in sequence and ensure data integrity
        //before writing to output file (cheking head of linked list first)
        
        if(head->seq == count){
            coretempStr = head->coredata.to_string();
            for (int j =0; j<PAYLOADLENGTH; j++){
                characterTemp = coretempStr.substr(PAYLOADLENGTH*j, 8);
                bitset <8> tempchar(characterTemp);
                tempcharArray[j] = char(tempchar.to_ulong());
            }
            //Calculating CRC16 on core data and checking against
            //transmitters CRC16 value stored in the trailer
            coretempStr = tempcharArray;
            crc = (crc16(tempcharArray, 8));
            bitset <16> sourceCRCBits(crc);
            checksum = to_string(crc);
            checksumint = (int)sourceCRCBits.to_ulong();
           //if CRCs are the same send back ACK with Frame number
            if (checksumint == head->sourceCRC){
                printf("Success crcs are the same for frame %i \nSending ACK\n", count);
                bzero(buffer, 256);
                reply = "";
        
                bitset<8> countBits(count);
                reply = ACK + countBits.to_string();
                strcpy(buffer, reply.c_str());
                n = sendto(serverSocket, &buffer, sizeof(buffer), 0, (struct sockaddr*) &clientAddr, sizeof(clientAddr));
                
                if(n < 0)
                {
                    printf("Error: Could not write to socket");
                }
                //delete node from list
                dataFrame *p = head;
                head = head->next;
                delete p;
                QUE--;
                printf("writing to file\n\n");
                //Write data out to file
                writeToFile(tempcharArray);
                count++;
            }
            else{//crc's are not same so data is corrupted
                printf("Data has been corrupted. Sending NAK\n");
                bzero(buffer, 256);
                bitset<8> countBits(count);
                reply = NAK + countBits.to_string();
                strcpy(buffer, reply.c_str());
                n = sendto(serverSocket, &buffer, sizeof(buffer), 0, (struct sockaddr*) &clientAddr, sizeof(clientAddr));
                if(n < 0)
                {
                    printf("Error: Could not write to socket");
                }
                //delete node from list
                dataFrame *p = head;
                head = head->next;
                delete p;
                QUE--;
            }
            
        }
        else{// Check rest of list
            dataFrame* temp = head;
            //go to end of list
            while (temp->next && temp->seq != count) {
                pTemp = temp;
                temp = temp->next;
            }
            if(temp->seq == count){
                coretempStr = temp->coredata.to_string();
                for (int j =0; j<PAYLOADLENGTH; j++){
                    characterTemp = coretempStr.substr(PAYLOADLENGTH*j, 8);
                    bitset <8> tempchar(characterTemp);
                    tempcharArray[j] = char(tempchar.to_ulong());
                }
                coretempStr = tempcharArray;
                crc = (crc16(tempcharArray, 8));
                bitset <16> sourceCRCBits(crc);
                checksum = to_string(crc);
                checksumint = (int)sourceCRCBits.to_ulong();
                
                if (checksumint == temp->sourceCRC){
                    printf("Success crcs are the same for frame %i \nSending ACK\n", count);
                    //send back ack and correctly delete node from list
                    bzero(buffer, 256);
                    bitset<8> countBits(count);
                    reply = ACK + countBits.to_string();
                    strcpy(buffer, reply.c_str());
                    n = sendto(serverSocket, &buffer, sizeof(buffer), 0, (struct sockaddr*) &clientAddr, sizeof(clientAddr));
                    if(n < 0)
                    {
                        printf("Error: Could not write to socket");
                    }
                    //delete node from list
                    dataFrame *p = temp;
                    pTemp->next = temp->next;
                    delete p;
                    QUE--;
                    //Write out to file
                    printf("Writing to file\n\n");
                    writeToFile(tempcharArray);
                    count++;
                }
                else{//CRCs are not same so data is corrupted send NAK
                    printf("Data has been corrupted. Sending NAK\n");
                    bzero(buffer, 256);
                    bitset<8> countBits(count);
                    reply = NAK + countBits.to_string();
                    strcpy(buffer, reply.c_str());
                    n = sendto(serverSocket, &buffer, sizeof(buffer), 0, (struct sockaddr*) &clientAddr, sizeof(clientAddr));
                    if(n < 0)
                    {
                        printf("Error: Could not write to socket");
                    }
                    //delete node from list
                    dataFrame *p = temp;
                    pTemp->next = temp->next;
                    delete p;
                    QUE--;
                }
            }
            
        }
        
    }
    
    printf("Frames all recived and printed out in order\n\n");
    
    return 0;
    
}
