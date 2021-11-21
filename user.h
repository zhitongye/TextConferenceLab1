#ifndef USER_H_
#define USER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>



#define USERNAME 10
#define PASSWORD 10

//struct to hold info of user
typedef struct user {
    //ID and password of client
    char username[USERNAME];   
    char password[PASSWORD];       
    //record of info of client
    //file descriptor and new thread for the user
    int socketfd;         
    pthread_t p;        
    bool inSession;
    Session *sessJoined;    
    struct user *next;     

}User;

//struct to hold info of session
typedef struct session {
    int sessionId;
    //all users in the session currently
    struct User *userloggedin;
    //next session in the session list
    struct Session *next;
    	
}Session;


//return user database from a text file
User* init_userlist() {
    User* root = NULL;
    FILE* fptr;
    char oneuser[30];
    
    if ((fptr = fopen("userdatabase.txt", "r")) == NULL) {
        printf("Error! File cannot be opened.");
        return NULL;
    }
    
    
    
    //printf("Data from the file:\n%s", oneuser);
    
    while(1) {
        User* user1 = malloc(sizeof(User));
        fscanf(fptr, "%[^\n]", oneuser);
        if(strcpy(oneuser,"END") == 0){
            free(user1);
            break;
        }
        strcpy(user1 -> username, strtok(oneuser, " "));
        strcpy(user1 -> password = strtok(NULL, " "));
        
             
        root = add_user(root, user1);
    }
    fclose(fptr);
    return root;
}



// Add a new user login to the userList
User* add_user(User* userList, User *newuser) {
    if(newuser == NULL) return NULL;
    newuser -> next = userList;
    return newuser;
}

// Remove user from a list based on username
User* remove_user(User *userList, User *userlogin) {
    //check if there's nothing in the current user list
    if(userList == NULL) return NULL;
    if(strcmp(userList -> username, userlogin -> username) == 0) {
        User *cur = userList -> next;
        //deallocate
        free(userList);
        return cur;
    }

    User *cur = userList;
    User *prev = NULL;
    while(cur != NULL) {
        if(strcmp(cur ->username, userlogin ->username) == 0) {
            prev -> next = cur -> next;
            free(cur);
            break;
        } else{
                prev = cur;
                cur = cur -> next;
        } 
    }
    return userList;
}

// delete linked list of users
void delete_userlist(User* root) {
    User* cur = root;
    User* next = cur;
    while(cur != NULL) {
        next = cur -> next;
        free(cur);
        cur = next;
    }
}

//from the textfile
bool is_valid(const User* userList, const User* user1) {
    User *current = userList;
    while(current != NULL) {
        if(strcmp(current -> username, user1 -> username) == 0 && strcmp(current -> password, user1 -> password) == 0) {
            return 1;    
        }
        current = current -> next;
    }
    return 0;
}

// Check if user is in list (matches username)
bool search_user(const User *userList, const User *user1) {
    User* current = userList;
    while(current != NULL) {
        if(strcmp(current -> username, user1 -> username) == 0) {
            return 1;    
        }
        current = current -> next;
    }
    return 0;
}

Session* create_session(Session* sessionList, int sessionId) {
    Session* newSession = malloc(sizeof(Session));
    newSession -> sessionId = sessionId;
    newSession -> next  = sessionList;
    newSession -> userloggedin = NULL;
    return newSession;
}

Session* isValidSession(Session* sessionList, int sessionId) {
	Session* cur = sessionList;
    if(sessionList == NULL) return NULL;
	while(cur != NULL) {
		if(cur -> sessionId == sessionId) {
			return cur;
		}
        else
		cur = cur -> next;
	}
	return NULL;
}

bool inSession(Session* sessionList, int sessionId, User* user) {
	Session *session = isValidSession(sessionList, sessionId);
	if(session != NULL){
        //check the exit in userlist 
        User *current = session -> userloggedin;
        while(current != NULL) {
            if(strcmp(current -> username, user -> username) == 0) {
                return 1;    
            }
            current = current -> next;
        }
        return 0;
    }
	else 
            return 0;
	
}

//insert new node into linked list
Session *join_session(Session *sessionList, int sessionId, const User *user1) {
    
	Session *cur = isValidSession(sessionList, sessionId);
	if(cur == NULL || user1 == NULL) return NULL;
    cur -> userloggedin = add_user(cur -> userloggedin, user1);
    return sessionList;
}

//delete a list node from a linked list
Session* remove_session(Session* sessionList, int sessionId) {
    
	if(sessionList == NULL) return NULL;
    
	if(sessionList -> sessionId == sessionId) {
		Session *cur = sessionList -> next;
		free(sessionList);
		return cur;
	}

	
    Session *cur = sessionList;
    Session *prev;
    while(cur != NULL) {
        if(cur -> sessionId == sessionId) {
            prev -> next = cur -> next;
            free(cur);
            break;
        } else {
            prev = cur;
            cur = cur -> next;
        }
    }
    return sessionList;
}

//remove user from current session
Session *leave_session(Session *sessionList, int sessionId, User *user1) {
	Session *current = isValidSession(sessionList, sessionId);
	if(current == NULL) return NULL;

	
	current -> userloggedin = remove_user(current -> userloggedin, user1);

	if(current -> userloggedin == NULL) {
		sessionList = remove_session(sessionList, sessionId);
	}
	return sessionList;
}


void delete_session_list(Session* sessionList) {
	Session* current = sessionList;
	Session* next = current;
	while(current != NULL) {
		delete_userlist(current -> user);
		next = current -> next;
		free(current);
		current = next;
	}
}


#endif
