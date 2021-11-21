/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   server.c
 * Author: yezhiton
 *
 * Created on November 17, 2021, 3:14 PM
 */

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

#define ULISTFILE "userlist.txt"
/*
 * 
 */
User* userLoggedin = NULL;          //store the users that have logged in
User* userList = NULL;              //the whole user list
Session* sessionList = NULL;        //the whole session list

pthread_mutex_t userLoggedin_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sessionList_mutex = PTHREAD_MUTEX_INITIALIZER;
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
void *newClient(void *arg){
    char buffer[NET_BUF_SIZE];          //for recv()
    char name[MAX_NAME];            //username
    User* client = (User*)arg;
    //stores the session list the user has joined
    Packet packetRecv;              //convert data into packet
    Packet packetSend;  
    int byteSend;
    int byteRecv;
    bool loggedIn = false;
    //recv()
    while(1){
        memset(buffer, 0, sizeof(char) * NET_BUF_SIZE);         //reset the buffer
        memset(&packetRecv, 0, sizeof(Packet)); 
        memset(&packetSend, 0, sizeof(Packet));
        
        byteRecv = recv(client ->socketfd, buffer, NET_BUF_SIZE - 1, 0);
        if(byteRecv == -1){
            printf("Error in receive the data.\n");
            exit(1);
        }
        buffer[byteRecv] = '\0';
        printf("Receive message: \"%s\"\n", buffer);
        stringToPacket(buffer, &packetRecv);
        
        if(!loggedIn){
            if(packetRecv.type == LOGIN){
                strcpy(client -> username, (char *)(packetRecv.source));
                strcpy(client -> password, (char *)(packetRecv.data));
                printf("User Name: %s\n", client -> username);
                printf("User Password: %s\n", client -> password);
                if(search_user(userLoggedin, client)){
                    packetSend.type = LO_NAK;
                    strcpy((char *)(packetSend.data), "User has logged in");
                    
                    memcpy(packetSend.source, client -> username, MAX_NAME);
                    packetSend.size = strlen((char *)(packetSend.data));
                    memset(buffer, 0, NET_BUF_SIZE);
                    packetToString(&packetSend, buffer);
                    if((byteRecv = send(client -> socketfd, buffer, NET_BUF_SIZE - 1, 0)) == -1) {
                        printf("fail to send\n");
                    }
                    
                    printf("User has logged in, error.\n");
                    exit(1);
                }
                if(is_valid(userList, client) == 0){
                    packetSend.type = LO_NAK;
                    strcpy((char *)(packetSend.data), "Invalid user name and password");
                    
                    memcpy(packetSend.source, client -> username, MAX_NAME);
                    packetSend.size = strlen((char *)(packetSend.data));
                    memset(buffer, 0, NET_BUF_SIZE);
                    packetToString(&packetSend, buffer);
                    if((byteRecv = send(client -> socketfd, buffer, NET_BUF_SIZE - 1, 0)) == -1) {
                        printf("fail to send\n");
                    }
                    
                    printf("User name and password are invalid, error.\n");
                    exit(1);
                }
                //memset(client -> password, 0, PASSWORD);
                //clear the password buffer
                //now the user is able to log in
                packetSend.type = LO_ACK;
                loggedIn = true;
                
                memcpy(packetSend.source, client -> username, MAX_NAME);
                packetSend.size = strlen((char *)(packetSend.data));
                memset(buffer, 0, NET_BUF_SIZE);
                packetToString(&packetSend, buffer);
                if((byteRecv = send(client -> socketfd, buffer, NET_BUF_SIZE - 1, 0)) == -1) {
                    printf("fail to send\n");
                }
                    
                //now add the user name to the userLoggedin list
                
                pthread_mutex_lock(&userLoggedin_mutex);
                userLoggedin = add_user(userLoggedin, client);
                pthread_mutex_unlock(&userLoggedin_mutex);

                printf("User %s has successfully logged in!\n", client->username);

            }
            else{
                packetSend.type = LO_NAK;
                strcpy((char *)(packetSend.data), "Before do something please log in first.");
                
                memcpy(packetSend.source, client -> username, MAX_NAME);
                packetSend.size = strlen((char *)(packetSend.data));
                memset(buffer, 0, NET_BUF_SIZE);
                packetToString(&packetSend, buffer);
                if((byteRecv = send(client -> socketfd, buffer, NET_BUF_SIZE - 1, 0)) == -1) {
                    printf("fail to send\n");
                }
                    
            }
        }
        //now the user wants to join session
        else if(packetRecv.type == JOIN){
            int session_ID = atoi((char*)packetRecv.data);
            printf("User %s is going to join session %d.\n", client->username, session_ID);
            //session does not exist
            if(isValidSession(sessionList, session_ID) == NULL){
                packetSend.type = JN_NAK;
                char* i = (char*)packetRecv.data;
                strcat((char*)i, " Session does not exist.");
                strcpy((char *)(packetSend.data), (char*)i);
                
                memcpy(packetSend.source, client -> username, MAX_NAME);
                packetSend.size = strlen((char *)(packetSend.data));
                memset(buffer, 0, NET_BUF_SIZE);
                packetToString(&packetSend, buffer);
                if((byteRecv = send(client -> socketfd, buffer, NET_BUF_SIZE - 1, 0)) == -1) {
                    printf("fail to send\n");
                }
                    
                printf("User %s has failed to join session %d.\n", client->username, session_ID);
            }
            //user has already joined the session
            else if(inSession(sessionList, session_ID, client)){
                packetSend.type = JN_NAK;
                char* i = (char*)packetRecv.data;
                strcat((char*)i, " Session is joined by the user.");
                strcpy((char *)(packetSend.data), (char*)i);
                
                memcpy(packetSend.source, client -> username, MAX_NAME);
                packetSend.size = strlen((char *)(packetSend.data));
                memset(buffer, 0, NET_BUF_SIZE);
                packetToString(&packetSend, buffer);
                if((byteRecv = send(client -> socketfd, buffer, NET_BUF_SIZE - 1, 0)) == -1) {
                    printf("fail to send\n");
                }
                    
                printf("User %s has already joined session %d.\n", client->username, session_ID);
            }
            //successfully joined the session
            else{
                packetSend.type = JN_ACK;
                sprintf((char *)(packetSend.data), "%d", session_ID);
                //now add the user into session list
                pthread_mutex_lock(&sessionList_mutex);
                sessionList = join_session(sessionList, session_ID, client);
                pthread_mutex_unlock(&sessionList_mutex);
                
                memcpy(packetSend.source, client -> username, MAX_NAME);
                packetSend.size = strlen((char *)(packetSend.data));
                memset(buffer, 0, NET_BUF_SIZE);
                packetToString(&packetSend, buffer);
                if((byteRecv = send(client -> socketfd, buffer, NET_BUF_SIZE - 1, 0)) == -1) {
                    printf("fail to send\n");
                }
                    
                printf("User %s has successfully joined session %d.\n", client->username, session_ID);
                //now add the current session id to the sessionList the user has joined
                //update user status
                pthread_mutex_lock(&userLoggedin_mutex);
                User *user = userLoggedin;
                while(user != NULL) {
                    if(strcmp(user -> username, packetRecv.source) == 0) {
                        user -> inSession = 1;
                        user -> sessJoined = create_session(user -> sessJoined, session_ID);
                    }
                    user = user ->next;
                }
                pthread_mutex_unlock(&userLoggedin_mutex);
                
            }
        }
        //the user wants to leave session
        else if(packetRecv.type == LEAVE_SESS){
            printf("User %s is going to leave the sessions.\n", client->username);
            int stored_ID = client ->sessJoined ->sessionId;
            free(client ->sessJoined);
            //update session list
            pthread_mutex_lock(&sessionList_mutex);
            sessionList = leave_session(sessionList, stored_ID, client);
            pthread_mutex_unlock(&sessionList_mutex);
            printf("User left the session.\n");
            //update userLoggedin status
            pthread_mutex_lock(&userLoggedin_mutex);
            User* user = userLoggedin;
            while(user != NULL) {
                if(strcmp(user -> username, packetRecv.source) == 0) {
                    delete_session_list(user -> sessJoined);
                    user -> sessJoined = NULL;
                    user -> inSession = 0;
                    break;
                }
            }
            pthread_mutex_unlock(&userLoggedin_mutex);
        }
        //User creates a new session
        else if(packetRecv.type == NEW_SESS){
            printf("User %s is going to create a new session.\n", client ->username);
            packetSend.type = NS_ACK;
            packetToString(&packetSend, buffer);
            
            memcpy(packetSend.source, client -> username, MAX_NAME);
            packetSend.size = strlen((char *)(packetSend.data));
            memset(buffer, 0, NET_BUF_SIZE);
            packetToString(&packetSend, buffer);
            if((byteRecv = send(client -> socketfd, buffer, NET_BUF_SIZE - 1, 0)) == -1) {
                printf("fail to send\n");
            }
                    
            //update session list state
            pthread_mutex_lock(&sessionList_mutex);
            sessionList = create_session(sessionList, atoi((char*)packetRecv ->data));
            pthread_mutex_unlock(&sessionList_mutex);
            
            //user join this created session
            pthread_mutex_lock(&sessionList_mutex);
            client ->sessJoined = join_session(client ->sessJoined, atoi((char*)packetRecv ->data), client);
            pthread_mutex_unlock(&sessionList_mutex);
            
            //update the user state
            pthread_mutex_lock(&userLoggedin_mutex);
            User* user = userLoggedin;
            while(user != NULL){
                if(strcmp(user ->username, packetRecv.source) == 0){
                    user ->inSession = 1;
                    user ->sessJoined = create_session(user ->sessJoined, atoi((char*)packetRecv ->data));
                }
            }
            pthread_mutex_unlock(&userLoggedin_mutex);
       
            printf("User %s has successfully created session %d\n", client -> username, atoi((char*)packetRecv ->data);
          
        }
        //user is going to send message
        else if(packetRecv.type == MESSAGE){
            printf("User %s is sending message \"%s\"\n", client ->username, packetRecv.data);
            packetSend.type = MESSAGE;
            //send the message to session
            strcpy((char *)(packetSend.source), client -> username);
            strcpy((char *)(packetSend.data), (char *)(packetRecv.data));
            packetSend.size = strlen((char *)(packetSend.data));
            
            memset(buffer, 0, sizeof(char) * NET_BUF_SIZE);
            packetToString(&packetSend, buffer);
            
            memcpy(packetSend.source, client -> username, MAX_NAME);
            packetSend.size = strlen((char *)(packetSend.data));
            memset(buffer, 0, NET_BUF_SIZE);
            packetToString(&packetSend, buffer);
            if((byteRecv = send(client -> socketfd, buffer, NET_BUF_SIZE - 1, 0)) == -1) {
                printf("fail to send\n");
            }
                    
            printf("Server is broadcasting message %s to session:", buffer);
            //loop through every session the user joins
            Session* session_send = isValidSession(sessionList, client ->sessJoined ->sessionId);
            if(session_send == NULL){
                exit(1);
            }else{ 
                User* user = session_send ->userloggedin;
                while(user != NULL){
                    byteSend = send(user -> socketfd, buffer, NET_BUF_SIZE - 1, 0);
                    if(byteSend == -1){
                        printf("Message fails to send to user.\n");
                        exit(1);
                    }
                    user = user ->next;
                }
            }
            
        }
        //user needs to know the online users and available sessions
        else if(packetRecv.type == QUERY){
            printf("User %s is getting query.\n", client ->username);
            packetSend.type = QU_ACK;
            int i = 0;
            User* user = userLoggedin;
            char* next_line = '\n';
            for(Session *cur = sessionList; cur != NULL; cur = cur -> next) {
                if(isValidSession(sessionList, cur -> sessionId) == NULL) continue;
                char* str;
                
                sprintf(str, "%d", cur ->sessionId);
                strcpy((char*)packetSend.data, str);
                for(User *usr = cur ->userloggedin; usr != NULL; usr = usr -> next) {
                    
                    strcat(str, usr ->username);
                }
                strcat(str,next_line);
                //store users in one session into the packetSen
                strcat(packetSend->data,str);
            }
            memcpy(packetSend.source, client -> username, MAX_NAME);
            packetSend.size = strlen((char *)(packetSend.data));
            memset(buffer, 0, NET_BUF_SIZE);
            packetToString(&packetSend, buffer);
            if((byteRecv = send(client -> socketfd, buffer, NET_BUF_SIZE - 1, 0)) == -1){
                printf("fail to send\n");
            }
        }
        
    }
    close(client -> socketfd);
    //if previously successfully logged in, then clear up all info
    if(loggedIn){
        Session* cur = client ->sessJoined;
        pthread_mutex_lock(&sessionList_mutex);
        sessionList = leave_session(sessionList, cur -> sessionId, client);
        pthread_mutex_unlock(&sessionList_mutex);
        
        userLoggedin = remove_user(userLoggedin, client);
        free(client);
        printf("User is leaving.\n");
    }
}

char input[NET_BUF_SIZE] = {0}; 
int main(int argc, char** argv) {
    //clear the buffer
    char port_num[10] = argv[1];
    memset(input, 0, NET_BUF_SIZE * sizeof(char));
    fgets(input, NET_BUF_SIZE, stdin);
    char* i;
    i = strstr(input, "server ");
    if(i == NULL){
        printf("Server is not working.\n");
        exit(1);
    }else{
        memcpy(port_num, i + 7, sizeof(char) * strlen(i + 7) - 1);
    }
    printf("Server works on port %s.\n", port_num);
    
    FILE *fp;
    if((fp = fopen(ULISTFILE, "r")) == NULL) {
        fprintf(stderr, "Can't open input file %s\n", ULISTFILE);
    }
    userList = init_userlist(fp);
    fclose(fp);
    printf("Server: Userdata loaded\n");
    //load the user information data
    
    int socketfd;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage client_addr;
    socklen_t client_size;
    struct sigaction sa;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;    // Use TCP
    hints.ai_flags = AI_PASSIVE;    
    char s[INET_ADDRSTRLEN];
    int yes = 1;
    if(getaddrinfo(NULL, port_num, &hints, &servinfo) == -1){
        printf("Error in get address information.\n");
        exit(1);
    }
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((socketfd = socket(p->ai_family, p->ai_socktype,
                        p->ai_protocol)) == -1) {
            perror("Server: socket");
            continue;
        }
        if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                    sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }
        if (bind(socketfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(socketfd);
            perror("Server: bind");
            continue;
        }
        break;
    }
    freeaddrinfo(servinfo);
    if (p == NULL)  {
        fprintf(stderr, "Server: failed to bind\n");
        exit(1);
    }
    if (listen(socketfd, 10) == -1) {
        printf("Fail to listen,\n");
        exit(1);
    }
    //do accept()
    while(1){
        User* user = malloc(sizeof(User));
        client_size = sizeof(client_addr);
        user ->socketfd = accept(socketfd, (struct sockaddr *)&client_addr, &client_size);
        if (user -> socketfd == -1) {
            printf("Fail to accept.\n");
            exit(1);
        }
        inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *)&client_addr), s, sizeof(s));
        printf("Connected from %s\n", s);
        pthread_create(&(user -> p), NULL, newClient, (void *)user);
    }
    delete_userlist(userList);
    delete_userlist(userLoggedin);
    delete_session_list(sessionList);
    close(socketfd);
    printf("Terminate server.\n");
    return (EXIT_SUCCESS);
}

