#include <sys/socket.h>
#include <iostream>
#include <unistd.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>

#include "directive.h"
#include "rpc.h"

#ifdef __cplusplus
extern "C" {
#endif

// global identifier 
int sockfd = -1;

/*!
 * @brief 
 */
int rpcCall(char* name, int* argTypes, void** args) { 
    // set up connection with binder
    int portno;
    struct sockaddr_in bind_addr;
    struct hostent * binder;
    char * binderAddress;
    char * binderPort;
    char binderResponse [CMD_SIZE];
    char serverResponse [CMD_SIZE];
    char serverAddr [256];
    int serverPort;

    // check if connection with binder has already be initiated.
    if(sockfd == -1) {
        // get address and port from env
        binderAddress = getenv("BINDER_ADDRESS");
        binderPort = getenv("BINDER_PORT");
        portno = atoi(binderPort);
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if(sockfd < 0) {
            return ERROR_OPEN_SOCKET;
        }
        binder = gethostbyname(binderAddress);
        bzero((char*) &bind_addr, sizeof(bind_addr));
        bind_addr.sin_family = AF_INET;
        bcopy((char *)binder->h_addr, (char *)&bind_addr.sin_addr.s_addr,
                binder->h_length);
        bind_addr.sin_port = htons(portno);
        if(connect(sockfd, (struct sockaddr *) &bind_addr, sizeof(bind_addr)) <0) {
            return ERROR_CONN_BINDER;
        }
    }

    int errorCode, reasonCode;
    int len = 0;

    // Communicate with binder
    // send LOC_REQUEST
    errorCode = send(sockfd, LOC_REQUEST, CMD_SIZE, 0);
    if(errorCode < 0) return ERROR_SEND_BINDER;

    // send name
    errorCode = send(sockfd, name, BUFFER_LEN, 0);
    if(errorCode < 0) return ERROR_SEND_BINDER;

    // send argTypes len
    while(1) {
        if(argTypes[len] == 0) break;
        ++len;
    }
    ++len;
    errorCode = send(sockfd, &len, sizeof(len), 0);
    if(errorCode < 0) return ERROR_SEND_BINDER;

    // send argTypes
    errorCode = send(sockfd, argTypes, len, 0);
    if(errorCode < 0)  return  ERROR_SEND_BINDER;

    // receive LOC_RESPONSE
    errorCode = recv(sockfd, binderResponse, CMD_SIZE, 0);
    if(errorCode < 0) return ERROR_RECV_BINDER;
 
    // case LOC_FAILURE (eg: no server is available for this function)
    if(strcmp(binderResponse, LOC_FAILURE)== 0)  {
 
        // listen to and return reasonCode
        reasonCode = ERROR_RECV_BINDER;
        recv(sockfd, &reasonCode, sizeof(reasonCode), 0);
        return reasonCode;
    }

    // case success
    // receive server information
    errorCode = recv(sockfd, serverAddr, 256, 0);
    if(errorCode < 0) return ERROR_RECV_BINDER;

    errorCode = recv(sockfd, &serverPort, sizeof(serverPort), 0);
    if(errorCode < 0) return ERROR_RECV_BINDER;

    // Communicate with server
    int sock_server;
    struct sockaddr_in serv_addr;
    struct hostent * server;

    sock_server = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_server < 0) return ERROR_OPEN_SOCKET;

    server = gethostbyname(serverAddr);
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,
                    server->h_length);
    serv_addr.sin_port = htons(serverPort);
    if(connect(sock_server, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) <0) 
        return  ERROR_CONN_SERVER;
    // send EXECUTE
    errorCode = send(sock_server, EXECUTE, CMD_SIZE, 0);
    if(errorCode < 0) return ERROR_SEND_SERVER;

    // send name
    errorCode = send(sock_server, name, BUFFER_LEN, 0);
    if(errorCode < 0) return  ERROR_SEND_SERVER;
    
    // send argTypes length
    errorCode = send(sock_server, &len, sizeof(len), 0);
    if(errorCode < 0) return ERROR_SEND_SERVER;

    // send argTypes
    errorCode = send(sock_server, argTypes, len, 0);
    if(errorCode < 0) return ERROR_SEND_SERVER;

    // send args one by one
    for(int i = 0; i < len-1; ++i) {
        // retrieve info from input
        int in = argTypes[i] >> ARG_INPUT;
        int arrayLen = (argTypes[i] << 18) >> 18;  // 0 for scalar; array o/w
        int type = (argTypes[i] << 2) >> 18;

        if(arrayLen == 0) ++arrayLen;
        if(in == 0) continue;   // not an input
        switch(type) {
           case ARG_CHAR: 
               errorCode = send(sock_server, args[i], arrayLen * sizeof(char), 0);
               break;
           case ARG_SHORT: 
               errorCode = send(sock_server, args[i], arrayLen * sizeof(short), 0);
               break;
           case ARG_INT:
               errorCode = send(sock_server, args[i], arrayLen * sizeof(int), 0);
               break;
           case ARG_LONG:
               errorCode = send(sock_server, args[i], arrayLen * sizeof(long), 0);
               break;
           case ARG_DOUBLE:
               errorCode = send(sock_server, args[i], arrayLen * sizeof(double), 0);
               break;
           case ARG_FLOAT:
               errorCode = send(sock_server, args[i], arrayLen * sizeof(float), 0);
               break;
       }
       if(errorCode < 0) return ERROR_SEND_SERVER;
    }

    // receive server response
    errorCode = recv(sock_server, serverResponse, CMD_SIZE, 0);
    if(errorCode < 0) return ERROR_RECV_SERVER;
  
    // case EXEC_FAILURE
    if(strcmp(serverResponse, EXEC_FAILURE) == 0) {
        // listen to and return reasonCode
        reasonCode = ERROR_RECV_SERVER;
        recv(sock_server, &reasonCode, sizeof(reasonCode), 0);
        return reasonCode;
    }
    else {
        // exec success
        // get name back
        //errorCode = recv(sock_server, name, BUFFER_LEN, 0);
        //if(errorCode < 0) return ERROR_RECV_SERVER;

        //errorCode = recv(sock_server, argTypes, BUFFER_LEN, 0);
       // if(errorCode < 0) return ERROR_RECV_SERVER;
        
       // errorCode = recv(sock_server, args, BUFFER_LEN, 0);
      //  if(errorCode < 0) return ERROR_RECV_SERVER;
      for(int i = 0; i < len-1; ++i) {
         int out = argTypes[i] >> ARG_OUTPUT; // 1 for output
         int arrayLen = (argTypes[i] << 18) >> 18;
         int type = (argTypes[i] << 2) >> 18;
         if(arrayLen == 0) ++arrayLen;
         if(out == 0)  continue;
         switch(type) {
             case ARG_CHAR:
                 errorCode = recv(sock_server, args[i], arrayLen * sizeof(char), 0);
                 break;
             case ARG_SHORT:
                 errorCode = recv(sock_server, args[i], arrayLen * sizeof(short), 0);
                 break;
             case ARG_INT:
                 errorCode = recv(sock_server, args[i], arrayLen * sizeof(int), 0);
                 break;
             case ARG_LONG:
                 errorCode = recv(sock_server, args[i], arrayLen * sizeof(long), 0);
                 break;
             case ARG_DOUBLE:
                 errorCode = recv(sock_server, args[i], arrayLen * sizeof(double), 0);
                 break;
             case ARG_FLOAT:
                 errorCode = recv(sock_server, args[i], arrayLen * sizeof(float), 0);
                 break;
        }
        if(errorCode < 0) return ERROR_RECV_SERVER;
      }
    }
    return 0;   
}    
      

//extern int rpcCacheCall(char* name, int* argTypes, void** args);    // bonus

int rpcTerminate() {
    int errorCode;
    errorCode = send(sockfd, TERMINATE, CMD_SIZE, 0);
    if(errorCode < 0) return ERROR_SEND_BINDER;

    return 0;
}

#ifdef __cplusplus
}
#endif

