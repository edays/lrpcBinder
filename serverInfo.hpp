class ServerInfo {
public:
    ServerInfo();
    ServerInfo(char *hostname, int port, int sockfd);
    bool operator==(const ServerInfo &rhs);
    char * getHostname() const ;
    int getPort() const ;
    int getSockfd() const ;

    bool operator <(const ServerInfo& rhs) const
    {
        return hostname < rhs.hostname;
    }
private:
    char *hostname;
    int port;
    int sockfd;
};
