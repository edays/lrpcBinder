#include <sys/socket.h>
#include <vector>
#include <iostream>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include "binder.hpp"
#include "directive.h"
#include "binderDatabase.hpp"
#include "serverInfo.hpp"

#ifdef __cplusplus
extern "C" {
#endif

Binder::Binder() {
  db = BinderDatabase::getInstance();
}

Binder::~Binder() {
    // close all TCP connections
    for(auto it = connection.begin(); it != connection.end(); ++it) {
        closeTcpConnection(*it);
    }
}

void Binder::addTcpConnection() {
    int newSockFd;
    bool full {true};
    struct sockaddr_in connAddr;
    socklen_t connSize = sizeof(connAddr);

    newSockFd = accept(sockfd, (struct sockaddr *) &connAddr, &connSize);

    // add newSockFd to connection list
    for(auto it = connection.begin(); it != connection.end(); ++it) {
       if(*it == 0) {
          *it = newSockFd;
          full = false;
          break;
       }
    }

    if(full) {
       connection.push_back(newSockFd);
    }        
}

void Binder::closeTcpConnection(int fd) {
    if(fd == 0) return;    // already closed and removed from list

    for(auto it = connection.begin(); it != connection.end(); ++it) {
        if(*it == fd) {
            *it = 0;
            break;
        }
    }
}

int Binder::serve(int fd) { 
    // int warning {0};
    int errorCode;
    char cmd [CMD_SIZE];
    errorCode = recv(fd, cmd, CMD_SIZE, 0); 
    if(errorCode < 0) return ERROR_RECV_SERVER;

    char serverAddr [256];
    int serverPort;
    char name [BUFFER_LEN];
    int * argTypes;
    int len;

    if(strcmp(cmd, REGISTER) == 0) {
        // recv name length
        // errorCode = recv(fd, &len, sizeof(len), 0);

        // receive server address
        errorCode = recv(fd, serverAddr, 256, 0);
        if(errorCode < 0) return ERROR_RECV_SERVER;

        // receive server port and make an entry
        errorCode = recv(fd, &serverPort, sizeof(serverPort), 0);
        if(errorCode < 0) return ERROR_RECV_SERVER;
        ServerInfo * serverInfo = new ServerInfo(serverAddr, serverPort, fd); 

        // recv name 
        errorCode = recv(fd, name, BUFFER_LEN, 0);
        if(errorCode < 0) return ERROR_RECV_SERVER;

        // recv argTypes length
        errorCode = recv(fd, &len, sizeof(len), 0);
        if(errorCode < 0) return ERROR_RECV_SERVER;

        argTypes = new int [len];
        // recv argTypes
        errorCode = recv(fd, argTypes, len, 0);
        if(errorCode < 0) return ERROR_RECV_SERVER;

        db->registerServer(*serverInfo);
        int regResult = db->registerFunction(*serverInfo, name, argTypes);

        if (regResult < 0) {   
            errorCode = send(fd, REG_FAILURE, CMD_SIZE, 0);
            if(errorCode < 0) return ERROR_SEND_SERVER;
            return regResult;
        } else {        // register successfully        
            errorCode = send(fd, REG_SUCCESS, CMD_SIZE, 0);
            if(errorCode < 0) return ERROR_SEND_SERVER;
            return regResult; // Note: regResult can be 0 on success
                              //  or positive on warning
        }
    } 
    else if (strcmp(cmd, LOC_REQUEST) == 0) {
        // recv name
        errorCode = recv(fd, name, BUFFER_LEN, 0);
        if(errorCode < 0) return ERROR_RECV_CLIENT;
    
        // recv argTypes len
        errorCode = recv(fd, &len, sizeof(len), 0);
        if(errorCode < 0) return ERROR_RECV_CLIENT;

        argTypes = new int [len];
        // recv argTypes
        errorCode = recv(fd, argTypes, len, 0);
        if(errorCode < 0) return ERROR_RECV_CLIENT;
      
        // locate the server running the procedure
        const ServerInfo * serverInfo = db->getServerToUse(name, argTypes);
 
        if (serverInfo == NULL) {
            errorCode = send(fd, LOC_FAILURE, CMD_SIZE, 0);
            if(errorCode < 0) return ERROR_SEND_CLIENT;
            int error = ERROR_EMPTY_DB;
            errorCode = send(fd, &error, sizeof(int), 0);
            if(errorCode < 0) return ERROR_SEND_CLIENT;

        } else {
            errorCode = send(fd, LOC_SUCCESS, CMD_SIZE, 0);
            if(errorCode < 0) return ERROR_SEND_CLIENT;
            const char *hostname = serverInfo->getHostname();
            errorCode = send(fd, &hostname, sizeof(hostname), 0);
            if(errorCode < 0) return ERROR_SEND_CLIENT;
            const int portNum = serverInfo->getPort();
            errorCode = send(fd, &portNum, sizeof(portNum), 0);
            if(errorCode < 0) return ERROR_SEND_CLIENT;
        }
        return 0;
    }
    else if(strcmp(cmd, TERMINATE) == 0) {
        // for all servers in db, close their connections
        db->closeConnections();
        return BINDER_SHUTDOWN;
    }
    return ERROR_INVALID_CMD;
}

int Binder::startRoutine() {
    int warning {0};
    char hostname[256];
    struct sockaddr_in serv_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) return ERROR_OPEN_SOCKET;

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = 0;//htons(0);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    gethostname(hostname, 256);
    std::cout << "BINDER_ADDRESS " << hostname << std::endl;
    
    if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        return ERROR_BIND_SOCKET;
    }

    struct sockaddr_in sock;
    socklen_t sock_size= sizeof(sock);
    getsockname(sockfd, (struct sockaddr *)&sock, &sock_size);
    std::cout << "BINDER_PORT " << ntohs(sock.sin_port) << std::endl;

    listen(sockfd, 5); 
    int maxfd = sockfd;   
    while(1) {
        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);

        // find max socket_fd
        for(auto it = connection.begin(); it != connection.end(); ++it) {
            if(*it != 0) {
                FD_SET(*it, &read_fds);
                if(maxfd < *it) {
                    maxfd = *it;
                }
            }
        }
        int readResult = select(maxfd+1, &read_fds, NULL, NULL, NULL); 
        if(readResult < 0) return ERROR_SELECT;

        if(FD_ISSET(sockfd, &read_fds)) {
            // server socket is signaled
            addTcpConnection();
        } else {  
            // connected socket readable 
            for(auto it = connection.begin(); it != connection.end(); ++it) {
                if(FD_ISSET(*it, &read_fds)) {
                    warning = serve(*it);
                    if(warning == BINDER_SHUTDOWN) return 0;
                }
            }
        }
    }
}

int main () {
    Binder binder;
    binder.startRoutine();
}

#ifdef __cplusplus
}
#endif




















