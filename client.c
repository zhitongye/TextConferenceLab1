#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/time.h>
#include <pthread.h>

#include "packet.h"
#include "user.h"

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

char net_buf[NET_BUF_SIZE];
bool insession = false;


//multi-threading function: handle the message from server
//handle JN_ACK JN_NAK NS_ACK QU_ACK MESSAGE
//Reference: https://www.ibm.com/docs/en/zos/2.3.0?topic=functions-pthread-create-create-thread
void* server_handler(void* socketfd1){
    //convert the type
    int* socketfd = (int *)socketfd1;
    int numBytes;
    char recbuf[NET_BUF_SIZE];
    Packet packet;
    struct sockaddr_storage sender_addr;
    socklen_t addr_size;
    addr_size = sizeof(sender_addr);
    while(1){
        //receive from the 
        numBytes = recvfrom(socketfd, net_buf,
                          NET_BUF_SIZE, 0,
                          (struct sockaddr*)&sender_addr, &addr_size);
        if(!numBytes){
            printf("Fail receive.\n");
            return NULL;
        }
        stringToPacket(recbuf,&packet);
        if(packet.type == JN_ACK){
            printf("Acknowledge successful conference session join\n");
            printf("conference session ID: %s\n",packet.data);
            insession = true;
            //JN_ACK <session ID> Acknowledge successful conference session join
        }else if(packet.type == JN_NAK){
            printf("Negative acknowledgement of joining the session\n");
            printf("Failed conference session ID and fail reason: %s\n",packet.data);
            insession = false;
            //<session ID, reason for failure>
        }else if(packet.type == NS_ACK){
            printf("Acknowledge new conference session\n");
            insession = true;
        }else if(packet.type == QU_ACK){
            printf("Reply followed by a list of users online\n");
            printf("list of online users and available sessions: %s\n",packet.data);
            
            //<users and sessions>
        }else if(packet.type == MESSAGE){
            printf("MESSAGE from other users in the coference session\n");
            printf("%s\n",packet.data);
            //<users and sessions>
        }else if(packet.type == UNKNOWN){
            printf("Unknown packet type received\n");
            return NULL;
        }else{
            printf("Wrong packet type received\n");
            return NULL;
        }
        
    }
    return NULL;
}


void login(char* client_ID,char* password,char* server_IP,char* server_port, int* socketfd,pthread_t* new_thread){
//Log into the server at the given address and port. The IP 
//address is specified in the dotted decimal format
    
    //check argument valid
    if (client_ID == NULL || password == NULL || server_IP == NULL || server_port == NULL) {
		printf("Wrong command\n");
		exit(0);
	}
    
    
    // code for a client connecting to a server
    // namely a stream socket to server_IP on port server_port
    // either IPv4 or IPv6
    
    struct addrinfo hints, *servinfo, *p;
    int rv;
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(server_IP, server_port, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    exit(1);
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((*socketfd = socket(p->ai_family, p->ai_socktype,
    p->ai_protocol)) == -1) {
    perror("socket");
    continue;
    }
    if (connect(socketfd, p->ai_addr, p->ai_addrlen) == -1) {
        perror("connect");
        //freed up the socket and never used it again
        close(*socketfd);
        continue;
    }

    break; // if we get here, we must have connected successfully
    }

    if (p == NULL) {
        // looped off the end of the list with no connection
        fprintf(stderr, "failed to connect\n");
        //freed up the socket
        close(*socketfd);
        exit(0);
    }

    freeaddrinfo(servinfo); // all done with this structure
    
    //create a new packet to send
    Packet packet;
    packet.type = LOGIN;
    strcpy(packet.data, password);
    packet.size = strlen(packet.data);
    strcpy(packet.source, client_ID);
    packetToString(&packet, net_buf);
    int numbytes;
    //send(int s, const void *buf, size_t len, int flags);
    if ((numbytes = send(*socketfd, net_buf, NET_BUF_SIZE-1, 0)) == -1) {
			printf("Client send failed\n");
            //freed up and initialize it back to -1
			close(*socketfd);
            *socketfd = -1;
			exit(0);
		}

    if ((numbytes = recv(*socketfd, net_buf, NET_BUF_SIZE-1, 0)) == -1) {
			printf("Client recv failed\n");
			close(*socketfd);
            //freed up and initialize it back to -1
            *socketfd = -1;
			exit(0);
		}
    
    stringToPacket(net_buf, &packet);
    //check success of login
    
    if (packet.type == LO_ACK) {
        //Acknowledge successful login
        //a new thread is needed to handle the message from the server 
        numbytes = pthread_create(new_thread, NULL, server_handler,(void *) socketfd);
        if(numbytes == 0)
            printf("login successful.\n"); 
        else{
            exit(0);
        }
    } else if (packet.type == LO_NAK) {
      printf("login failed.\n");
      close(*socketfd);
      *socketfd = -1;
      exit(0);
    } else {
        printf("unknown packet type received\n");
        close(*socketfd);
        *socketfd = -1;
        exit(0);
    } 
	
}



//Join the conference session with the given session ID
//joinsession(sessionID, &socketfd);
void joinsession(char* sessionID, int* socketfd){
    if(socketfd == -1){
        printf("You mush login first before join coference session\n");
        exit(0);
    }
    
    if(insession == true){
        printf("Currently you're in another session so failed join session\n");
        exit(0);
    }
    
    //check currently in another session or not!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    
    
    
    //JOIN <session ID> Join a conference session
    //create a control packet to send to server
    Packet packet;
    int numBytes;
    packet.type = JOIN;
    //packet.data = sessionID;
    packet.size = strlen(sessionID);
    strcpy(packet.data,sessionID);
    packetToString(&packet,net_buf);
    //send(int s, const void *buf, size_t len, int flags);
    numBytes = send(*socketfd,net_buf,NET_BUF_SIZE-1,0);
    if(numBytes == -1){
        printf("Failed send packect join session\n");
        return;
     }
}

void logout(int *socketfd, pthread_t *new_thread) {
	if(*socketfd == -1) {
		printf("You mush login first before join coference session\n");
		return;
	} 

	
    Packet packet;
    packet.type = EXIT;
    packet.size = 0;
    packetToString(&packet, net_buf);
	int numBytes = send(*socketfd, net_buf,NET_BUF_SIZE - 1, 0);
	if(numBytes == -1) {
		printf("Send of client logout packet failed\n");
		return;
	}
    //pthread_cancel(thread1);
    //Reference:https://www.geeksforgeeks.org/pthread_cancel-c-example/
    //shut down the thread
	if(pthread_cancel(*new_thread) != 0) {
			printf("Fail to shut down the thread\n");
	} else{
			printf("Client logout successfully\n");	
	} 
    //freed up the socket(current client) and set it back to invalid
	close(*socketfd);
	*socketfd = -1;
    insession = false;
}

void leavesession(int* socketfd){
	if(socketfd == -1) {
		printf("You mush login first before leave coference session\n");
        return;
	} 
    if(insession != 1) {
        printf("No session to leave currently\n");
        return;
    }

	//create a control packet to send the leavesession message to server
	Packet packet;
    packet.type = LEAVE_SESS;
    packet.size = 0;
    packet.data = NULL;
    packetToString(&packet,net_buf);
    int numBytes = send(*socketfd,net_buf,NET_BUF_SIZE - 1, 0);
	if (numBytes == -1) {
		printf("Fail to send the leavesession packet\n");
		return; 
	}
    insession = false;
}


//
void createsession(char* sessionID,int* socketfd){
    if(socketfd == -1) {
		printf("You mush login first before leave coference session\n");
        return;
	} 
    if(insession){
        printf("Currently you're in another session so failed create new session\n");
        return;
    }
    
    //check currently in another session or not!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    
	//create a control packet to send the createsession message to server
	Packet packet;
    packet.type = NEW_SESS;
    packet.size = 0;
    strcpy(packet.data,sessionID);
    packetToString(&packet,net_buf);
    int numBytes = send(*socketfd,net_buf,NET_BUF_SIZE - 1, 0);
	if (numBytes == -1) {
		printf("Fail to send the createsession packet\n");
		return; 
	}
    insession = true;
}

void list(int* socketfd){
    if(socketfd == -1) {
		printf("You mush login first before get a list of coference session\n");
        return;
	} 
   //create a control packet to send the list message to server
	Packet packet;
    packet.type = QUERY;
    packet.size = 0;
    packet.data = NULL;
    packetToString(&packet,net_buf);
    int numBytes = send(*socketfd,net_buf,NET_BUF_SIZE - 1, 0);
	if (numBytes == -1) {
		printf("Fail to send the list packet\n");
		return; 
	}
    
}

void send_text(int* socketfd){
    if(socketfd == -1) {
		printf("You mush login first before send text\n");
        return;
	} 
    if(!insession){
		printf("You are not in any session\n");
        return;
	}    
   //create a control packet to send the list message to server
	Packet packet;
    packet.type = MESSAGE;
    strcpy(packet.data,net_buf);
    packet.size = strlen(packet.data);
    packetToString(&packet,net_buf);
    int numBytes = send(*socketfd,net_buf,NET_BUF_SIZE - 1, 0);
	if (numBytes == -1) {
		printf("Fail to send the text\n");
		return; 
	}
    
}




int main() {
	char* pch;
	int toklen;
	int socketfd;
	pthread_t new_thread; //using multi-thread
    //Reference:https://www.cplusplus.com/reference/cstring/strtok/
	while(1){
		fgets(net_buf, sizeof(net_buf), stdin);
		pch = net_buf;
    
        if (*pch == NULL) {
            continue;
        } 
		pch = strtok(net_buf, ":");
		//check what is the command
		if (strcmp(pch, "/login") == 0) {
            //call the login function
            //use multithreading
            int count = 0;
            char* client_ID, password, server_IP, server_port;
            while (pch != NULL && count < 4){
                if(count == 0){
                    pch = strtok(NULL, ":");
                    strcpy(client_ID,pch);
                    count++;
                }else if(count == 1){
                    pch = strtok(NULL, ":");
                    strcpy(password,pch);
                    count++;
                }else if(count == 2){
                    pch = strtok(NULL, ":");
                    strcpy(server_IP,pch);
                    count++;
                }else if(count == 3){
                    pch = strtok(NULL, ":");
                    strcpy(server_IP,pch);
                    count++;
                }
                
            }
            login(client_ID, password, server_IP, server_port, &socketfd, &new_thread);
		} else if (strcmp(pch, "/logout") == 0) {
            logout(&socketfd, &new_thread);
		} else if (strcmp(pch, "/joinsession") == 0) {
            char* sessionID;
            pch = strtok(NULL, ":");
            strcpy(sessionID,pch);
			joinsession(sessionID, &socketfd);
		} else if (strcmp(pch, "/leavesession") == 0) {
			leavesession(socketfd);
		} else if (strcmp(pch, "/createsession") == 0) {
            char* sessionID;
            pch = strtok(NULL, ":");
            strcpy(sessionID,pch);
			createsession(sessionID,&socketfd);
		} else if (strcmp(pch, "/list") == 0) {
			list(&socketfd);
		} else if (strcmp(pch, "/quit") == 0) {
			logout(&socketfd, &new_thread);
			break;
		} else {
			//Send a message to the current conference session. 
			send_text(&socketfd);
		}
	}
	printf("Terminate the program.\n");
	return 0;
}
