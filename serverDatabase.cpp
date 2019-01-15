#include <iostream>
#include "serverDatabase.hpp"
#include "rpc.h"
#include "skeletonCallThread.cpp"
#include "directive.h"

ServerDatabase* ServerDatabase::instance = 0;

ServerDatabase* ServerDatabase::getInstance() {
    if (instance == 0) {
        instance = new ServerDatabase();
    }
    return instance;
}

// add skeleton to db
void ServerDatabase::addSkeleton(char *name, int *argTypes, skeleton f) {
    Function *func = new Function(name, argTypes);
//    auto pair = std::make_pair(*func, f);
    skeletons.insert(std::make_pair(*func, f));
}

// calls the corresponding skeleton function, returns success/fail
void ServerDatabase::callSkeleton(char *name, int *argTypes, void **args, int sockfd) {
    Function *funcToFind = new Function(name, argTypes);

    // get the skeleton to use
    std::map<Function, skeleton>::iterator it = skeletons.find(*funcToFind);
 
    executeSkeleton(argTypes, args, sockfd, it->second);
}