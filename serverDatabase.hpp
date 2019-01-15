#include <map>
#include "rpc.h"

#ifndef FUNCTION_HPP_
#define FUNCTION_HPP_

#include "function.hpp"

#endif

// forward declaration
//class Function;

// Singleton class
// contains data about skeletons, so we don't have to send those to the binder
class ServerDatabase {
public:
    static ServerDatabase* getInstance();

    // add skeleton to db
    void addSkeleton(char *name, int* argTypes, skeleton f);

    // calls the corresponding skeleton function, returns success/fail
    void callSkeleton(char *name, int* argTypes, void** args, int sockfd);

private:
    static ServerDatabase* instance;
    ServerDatabase() {};

    std::map<Function, skeleton> skeletons;
};
