#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/socket.h>
#include "rpc.h"
#include "directive.h"

struct threadData {
    int *argTypes;
    void **args;
    int sockfd;
    skeleton f;
};

void* SkeletonExecutor(void *d) {
    struct threadData *data;
    data = (struct threadData *) d;
    int len = 0;
    int errorCode;

    // run skeleton
    int skelResult = data->f(data->argTypes, data->args);

    // get args length
    while(1) {
       if(data->argTypes[len] == 0) break;
       ++len;
    }

    if(skelResult == 0) {     // EXEC_SUCCESS
       // send exec success
       errorCode = send(data->sockfd, EXEC_SUCCESS, CMD_SIZE, 0);
       if(errorCode < 0) {
           errorCode = ERROR_SEND_CLIENT;  
           pthread_exit(&errorCode);
       }

       // send args
       for(int i = 0; i < len; ++i) {
          int out = data->argTypes[i] >> ARG_OUTPUT; // 1 for output
          int arrayLen = (data->argTypes[i] << 18) >> 18;   // indicate scalar/array
          int type = (data->argTypes[i] << 2) >> 18;        // data type
          if(arrayLen == 0) ++arrayLen;
          if(out == 0)  continue;
          switch(type) {
              case ARG_CHAR:
                  errorCode = send(data->sockfd, data->args[i], arrayLen * sizeof(char), 0);
                  break;
              case ARG_SHORT:
                  errorCode = send(data->sockfd, data->args[i], arrayLen * sizeof(short), 0);
                  break;
              case ARG_INT:
                  errorCode = send(data->sockfd, data->args[i], arrayLen * sizeof(int), 0);
                  break;
              case ARG_LONG:
                  errorCode = send(data->sockfd, data->args[i], arrayLen * sizeof(long), 0);
                  break;
              case ARG_DOUBLE:
                  errorCode = send(data->sockfd, data->args[i], arrayLen * sizeof(double), 0);
                  break;
              case ARG_FLOAT:
                  errorCode = send(data->sockfd, data->args[i], arrayLen * sizeof(float), 0);
                  break;
          }
          if(errorCode < 0) {
              errorCode = ERROR_SEND_CLIENT;
              pthread_exit(&errorCode);
          }
       }  
    } else  {  // EXEC_FAILURE
       errorCode = send(data->sockfd, EXEC_FAILURE, CMD_SIZE, 0);
       if(errorCode < 0) {
           errorCode = ERROR_SEND_CLIENT;
           pthread_exit(&errorCode);   
       }
       // send reasonCode
       errorCode = send(data->sockfd, &skelResult, sizeof(skelResult), 0);
       if(errorCode < 0) {
           errorCode = ERROR_SEND_CLIENT;
           pthread_exit(&errorCode);
       }
    }
    pthread_exit(0);
}

void executeSkeleton(int *argTypes, void **args, int sockfd, skeleton f) {
    struct threadData data;
    data.argTypes = argTypes;
    data.args = args;
    data.sockfd = sockfd;
    data.f = f;

    pthread_t thread;
    pthread_create(&thread, NULL, SkeletonExecutor, (void *) &data);
}
