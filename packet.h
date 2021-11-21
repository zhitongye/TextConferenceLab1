/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   packet.h
 * Author: yezhiton
 *
 * Created on November 17, 2021, 3:15 PM
 */

#ifndef PACKET_H
#define PACKET_H
#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif /* PACKET_H */
#include <stdio.h>
#include <string.h> 
#include <stdlib.h>
#include <regex.h>
#define UNKNOWN 0
#define LOGIN 1
#define LO_ACK 2
#define LO_NAK 3
#define EXIT 4
#define JOIN 5
#define JN_ACK 6
#define JN_NAK 7
#define LEAVE_SESS 8
#define NEW_SESS 9
#define NS_ACK 10
#define MESSAGE 11
#define QUERY 12
#define QU_ACK 13

#define MAX_NAME 32
#define MAX_DATA 100

#define NET_BUF_SIZE 500




typedef struct message{
    unsigned int type;
    unsigned int size;
    unsigned char source[MAX_NAME];
    unsigned char data[MAX_DATA];
}Packet;

void packetToString(const Packet *packet, char *result) {
    
    // clean string buffer
    memset(result, 0, NET_BUF_SIZE);
    int i = 0;
    //load total number of segments into the result string
    //and not print it out
    i += sprintf(result, "%u", packet -> type);
    //use memcpy to copy the binary data
    memcpy(result + i, ":", sizeof(char));
    i++;
    //load the size of the data of current packet
    i+=sprintf(result + i, "%u", packet -> size);
    //i = strlen(result);
    memcpy(result + i, ":", sizeof(char));
    i++;
    sprintf(result + i, "%s", packet -> source);
    i += strlen(packet -> source);
    memcpy(result + i, ":", sizeof(char));
    i++;
    memcpy(result + i, packet -> data, sizeof(char) * MAX_DATA);
}
void stringToPacket(char* rec_buf, Packet* packet){
    int type1 = 0;
    int size1 = 0;
    int source1 = 0;
    int data1 = 0;
    int cursor = 1;
    int j;
    for(int i = 0; i < NET_BUF_SIZE; i++){
        if(rec_buf[i] != ':' && cursor == 1){
            type1++;
        }
        else if(rec_buf[i] == ':'&& cursor == 1){
            cursor++;
        }
        else if(rec_buf[i] != ':'&& cursor == 2){
          size1++; 
        }
        else if(rec_buf[i] == ':'&& cursor == 2){
            cursor++;
        }
        else if(rec_buf[i] != ':'&& cursor == 3){
          source1++; 
        }
        else if(rec_buf[i] == ':'&& cursor == 3){
            cursor++;
        }
        else if(rec_buf[i] != ':'&& cursor == 4){
          data1++; 
        }
        else if(rec_buf[i] == ':'&& cursor == 4){
            cursor++;
            j = i+1;
            break;
        }
     }
    //assign the value to total number of fragments
    char buffer[NET_BUF_SIZE];
    memset(buffer,0,NET_BUF_SIZE * sizeof(char));
    memcpy(buffer, rec_buf, type1 * sizeof(char));
    packet->type = atoi(buffer);
    //assign the value to size of data
    memset(buffer,0,NET_BUF_SIZE * sizeof(char));
    memcpy(buffer, rec_buf+ type1 +1, type1  * sizeof(char));
    packet->size = atoi(buffer);
    //assign the value to file name
    memcpy(packet->source, rec_buf + type1 + size1 + 2, source1 * sizeof(char));
    //assign the value to file data
    memcpy(packet->data , rec_buf + j, packet->size);   
}
