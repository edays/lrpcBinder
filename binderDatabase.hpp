#ifndef BINDERDATABASE_HPP_
#define BINDERDATABASE_HPP_

#include <vector>
#include <map>

#ifndef FUNCTION_HPP_
#define FUNCTION_HPP_

#include "function.hpp"

#endif

// forward declaration
//class Function;
class ServerInfo;

// Singleton class
class BinderDatabase {
public:
    static BinderDatabase* getInstance();

    void registerServer(ServerInfo info);
    int registerFunction(ServerInfo info, char *funcName, int *funcArgs);
    const ServerInfo * getServerToUse(char *funcName, int *argTypes);
    void closeConnections(); // terminate all connected servers

private:
    static BinderDatabase* instance;
    BinderDatabase() {};

    std::vector<ServerInfo> schedule {std::vector<ServerInfo> (5)}; // vector for round robin scheduling
    std::map<ServerInfo, std::vector<Function>> servers; // storing all the servers and functions
};

#endif
