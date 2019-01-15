#include <sys/socket.h>
#include <iostream>
#include <unistd.h>
#include <strings.h>
#include <netdb.h>
#include <stdlib.h>
#include <vector>
#include <string.h>
#include "directive.h"
#include "rpc.h"
#include "server_function_skels.h"
#include "serverDatabase.hpp"
#include <string>
#include "serverInfo.hpp"
#include "binderDatabase.hpp"

#ifdef __cplusplus
extern "C" {
#endif

// global identifier
int sockfd;      // listen to clients
int sock;        // register to binder
struct sockaddr_in bind_addr;
std::vector<int> connection;
fd_set read_fds;
ServerDatabase * db = ServerDatabase::getInstance(); // global server database instance
char serverHostname[256];
int serverPort;

void addTcpConnection() {
    int newSockFd;
    bool full = true;
    struct sockaddr_in connAddr;
    socklen_t connSize = sizeof(connAddr);

    newSockFd = accept(sockfd, (struct sockaddr *) &connAddr, &connSize);

    // add newSockFd to connection list
    for(std::vector<int>::iterator it = connection.begin(); 
                    it != connection.end(); ++it) {
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

void closeTcpConnection(int fd) {
    if(fd == 0) return;    // already closed and removed from list

    for(std::vector<int>::iterator it = connection.begin(); 
                     it != connection.end(); ++it) {
        if(*it == fd) {
            *it = 0;
            break;
        }
    }
}

int serve(int fd) {

    int errorCode;
    char cmd [CMD_SIZE];  

    // receive command
    errorCode = recv(fd, &cmd, CMD_SIZE, 0);
    if(errorCode < 0) return ERROR_RECV_CLIENT;

    if(strcmp(cmd, EXECUTE) == 0) {
        char name [BUFFER_LEN];
        int * argTypes;
        void ** args;
        int len;

        // receive name
        errorCode = recv(fd, name, BUFFER_LEN, 0);
        if(errorCode < 0) return ERROR_RECV_CLIENT;  

        // receive argTypes len
        errorCode = recv(fd, &len, sizeof(len), 0);
        if(errorCode < 0) return ERROR_RECV_CLIENT;        

        argTypes = new int[len];
        // receive argTypes
        errorCode = recv(fd, argTypes, len, 0);
        if(errorCode < 0) return ERROR_RECV_CLIENT;

        args = new void * [len-1];
        // receive args                      
        for(int i = 0; i < len-1; ++i) {
           // retrieve info from argTypes
           int in = argTypes[i] >> ARG_INPUT;
           int arrayLen = (argTypes[i] << 18) >> 18;  // 0 for scalar; array o/w
           int type = (argTypes[i] << 2) >> 18;
           int size;

           if(arrayLen == 0) ++arrayLen;
           switch(type) {
              case ARG_CHAR:
                  size = arrayLen * sizeof(char);
                  args[i] = new char [size]; 
                  if(in == 1) {
                     errorCode = recv(fd, args[i], size, 0);
                  }
                  break;
              case ARG_SHORT: 
                  size = arrayLen * sizeof(short);
                  args[i] = new short [size];
                  if(in == 1) { 
                     errorCode = recv(fd, args[i], size, 0);
                  }
                  break;
              case ARG_INT:
                  size = arrayLen * sizeof(int);
                  args[i] = new int [size];
                  if(in == 1) {
                     errorCode = recv(fd, args[i], size, 0);
                  }
                  break;
              case ARG_LONG:
                  size = arrayLen * sizeof(long);
                  args[i] = new long [size];
                  if(in == 0) {
                     errorCode = recv(fd, args[i], size, 0);
                  }
                  break;
              case ARG_DOUBLE:
                  size = arrayLen * sizeof(double);
                  args[i] = new double [size];
                  if(in == 0) {
                     errorCode = recv(fd, args[i], size, 0);
                  }
                  break;
              case ARG_FLOAT:
                  size = arrayLen * sizeof(float);
                  args[i] = new float [size];
                  if(in == 0) {
                     errorCode = recv(fd, args[i], size, 0);
                  }
                  break;
           }
           if(errorCode < 0) return ERROR_SEND_SERVER;
        }

        // pass the control to serverDatabase, which will communicate with client
        db->callSkeleton(name, argTypes, args, sockfd);
    }

    else if(strcmp(cmd, TERMINATE) == 0) {
        // check if terminate request comes from binder
        if (fd == sock) {
            return BINDER_SHUTDOWN;
        }
        // else do nothing
    }
    return 0;
}    


/*!
 * @brief Initiate connection to binder and create socket
 *          to welcome clients
 */
int rpcInit(void) {
    // create socket to listen to client requests
    struct sockaddr_in serv_addr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        return ERROR_OPEN_SOCKET;
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = 0;//htons(0);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    gethostname(serverHostname, 256);

    if(bind(sockfd, (struct sockaddr *) &serv_addr,
                sizeof(serv_addr)) < 0) {
        return ERROR_BIND_SOCKET;
    }
    struct sockaddr_in s;
    socklen_t sock_size= sizeof(s);
    getsockname(sockfd, (struct sockaddr *)&s, &sock_size);
    serverPort =  ntohs(s.sin_port);

    char * binderAddress = getenv("BINDER_ADDRESS");
    char * binderPort = getenv("BINDER_PORT");

    if (binderAddress == NULL || binderPort == NULL) {
        std::cerr << "Could not get binder address or port" << std::endl;
        return ERROR_CONN_BINDER;
    }

    // create a connection to binder
    int portno;
    struct hostent * binder;
    portno = atoi(binderPort);
    sock = socket(AF_INET,SOCK_STREAM,0);
    if(sock < 0) {
        return ERROR_OPEN_SOCKET;
    }
    binder = gethostbyname(binderAddress);
    bzero((char*) &bind_addr, sizeof(bind_addr));
    bind_addr.sin_family = AF_INET;
    bcopy((char *)binder->h_addr, (char *)&bind_addr.sin_addr.s_addr,
            binder->h_length);
    bind_addr.sin_port = htons(portno);
    if(connect(sock, (struct sockaddr *) &bind_addr, sizeof(bind_addr)) <0) {
        return ERROR_CONN_BINDER;
    }
    return 0;
}


/*!
 * @brief Register server functions to binder database
 *        and server lcoal database
 */
int rpcRegister(char* name, int * argTypes, skeleton f) {
    int  errorCode, reasonCode;
    char binderResponse[CMD_SIZE];

    // send register command
    errorCode = send(sock, REGISTER, CMD_SIZE, 0);
    if(errorCode < 0) return ERROR_SEND_BINDER;

    // send server address
    errorCode = send(sock, serverHostname, 256, 0);
    if(errorCode < 0) return ERROR_SEND_BINDER;

    // send server port
    errorCode = send(sock, &serverPort, sizeof(serverPort), 0);
    if(errorCode < 0) return ERROR_SEND_BINDER;

    // send name
    errorCode = send(sock, name, BUFFER_LEN, 0);
    if(errorCode < 0) return ERROR_SEND_BINDER;

    // send argTypes len and body
    int len = 0;
    while(1) {
        if(argTypes[len] == 0) break;
        ++len;
    }
    ++len;
    errorCode = send(sock, &len, sizeof(len), 0);
    if(errorCode < 0) return ERROR_SEND_BINDER;

    errorCode = send(sock, argTypes, len, 0);
    if(errorCode < 0) return ERROR_SEND_BINDER;  
   
    // receive register response
    errorCode = recv(sock, binderResponse, CMD_SIZE, 0);
    if(errorCode < 0) return ERROR_RECV_BINDER;

    if(strcmp(binderResponse, REG_FAILURE) == 0) { 
        reasonCode = ERROR_RECV_BINDER;
        recv(sock, &reasonCode, sizeof(reasonCode), 0);
        return reasonCode; 
    } else {  
      // create an entry to serverDB
      db->addSkeleton(name, argTypes, f);
      return 0;
    }
}

/*!
 * @brief Server starts listening and handling requests
 */
int rpcExecute() {
    int errorCode;      // error code during transmission           
    int maxfd = sockfd;

    listen(sockfd, 5); // 5?

    // if there is no entry in serverDB
    // return ERROR_EMPTY_DB;

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(sockfd,&read_fds);

        for(std::vector<int>::iterator it = connection.begin(); 
                    it != connection.end(); ++it) {
            if(*it != 0) {
                FD_SET(*it, &read_fds);
                if(maxfd < *it) {
                    maxfd = *it;
                }
            }
        }

        errorCode = select(maxfd+1, &read_fds, NULL, NULL, NULL); 
        if(errorCode < 0) return ERROR_SELECT;

        if(FD_ISSET(sockfd, &read_fds)) {
            // server socket is signaled
            addTcpConnection();
        } else {  
            // client readable 
            for(std::vector<int>::iterator it = connection.begin(); 
                      it != connection.end(); ++it) {
                if(FD_ISSET(*it, &read_fds)) {
                    errorCode = serve(*it);
                    if(errorCode == BINDER_SHUTDOWN) 
                      return 0;
                }
            }
        }
    }
}


#ifdef __cplusplus
}
#endif


      
