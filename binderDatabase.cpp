#include <vector>
#include <map>
#include <sys/socket.h>
#include <iostream>
#include <algorithm>
#include "binderDatabase.hpp"
#include "serverInfo.hpp"
#include <unistd.h>
#include "directive.h"

using namespace std;

BinderDatabase* BinderDatabase::instance = 0;

BinderDatabase* BinderDatabase::getInstance() {
    if (instance == 0) {
        instance = new BinderDatabase;
    }
    return instance;
}

void BinderDatabase::registerServer(ServerInfo info) {
    if (servers.find(info) == servers.end()) {
        vector<Function> funcs;
        servers.insert(make_pair(info, funcs));
        schedule.push_back(info);
    } else {
//        cerr << "Error: server already exists when registering" << endl;
    }
}

int BinderDatabase::registerFunction(ServerInfo info, char *funcName, int *funcArgs) {
    if (servers.find(info) == servers.end()) {
        cerr << "Error: server does not exist when registering function" << endl;
        return -1;
    } else {
        Function *func = new Function(funcName, funcArgs);
        vector<Function> *v = &(servers.find(info)->second);
        if (find(v->begin(), v->end(), *func) == v->end()) {
            v->push_back(*func);
        }
        return 0;
    }
}

const ServerInfo * BinderDatabase::getServerToUse(char *funcName, int *argTypes) {
    Function *f = new Function(funcName, argTypes);
    for (unsigned int i = 0; i < schedule.size(); i++) {
        auto serversIter = servers.find(schedule[i]);
        if (serversIter != servers.end()) {
            vector<Function> funcs = serversIter->second;
            cout << "funcs size: " << funcs.size() << endl;
            for(auto funcIter = funcs.begin(); funcIter != funcs.end(); funcIter++) {
                cout << "comparing func: " << funcIter->getName() << endl;
                if (*funcIter == *f) {
                    cout << "equal" << endl;
                    vector<ServerInfo>::iterator scheduleIter = schedule.begin() + i;
                    rotate(scheduleIter, scheduleIter + 1, schedule.end()); // move server to the end of schedule list
                    const ServerInfo* ptr = &(serversIter->first);
                    return ptr;
                }
            }
        }
    }
    cerr << "Error: no such function registered" << endl;
    return NULL;
}

void BinderDatabase::closeConnections() {
    int errorCode;
    // send terminate message to each socket
    for(auto iter = servers.begin(); iter != servers.end(); iter++) {
        ServerInfo info =  iter->first;
        const int sockfd = info.getSockfd();
        errorCode = send(sockfd, TERMINATE, CMD_SIZE, 0);
        if(errorCode < 0) {
            cerr << "Error sending terminate" << endl;
            return;
        }

        sleep(10);
//        close(info.getSockfd()); // don't think we need this if socket is closing from server's side?
    }
}
