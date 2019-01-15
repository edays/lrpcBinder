#include "serverInfo.hpp"
#include <string.h>

ServerInfo::ServerInfo() {
    // empty
}

ServerInfo::ServerInfo(char *hostname, int port, int sockfd) {
    this->hostname = hostname;
    this->port = port;

    this->sockfd = sockfd;
}


bool ServerInfo::operator==(const ServerInfo &rhs) {
    return (strcmp(this->hostname, rhs.hostname) == 0) &&
            (this->port == rhs.port) &&
            (this->sockfd == rhs.sockfd);
}

char * ServerInfo::getHostname() const {
    return hostname;
}

int ServerInfo::getPort() const {
    return port;
}

int ServerInfo::getSockfd() const {
    return sockfd;
}
