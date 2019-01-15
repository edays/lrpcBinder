#include <vector>
#include <sys/time.h>
#include <stdlib.h>
#include "binderDatabase.hpp"

class Binder {
public:
    Binder();
    ~Binder();
    int startRoutine();
private:
    int sockfd;
    fd_set read_fds;
    BinderDatabase * db;

    /*!
     * @brief Connection fd vector 
     *        0: no connection
     *        active o/w.
     */
    std::vector<int> connection {std::vector<int> (5)}; //not necessarily 5

    void addTcpConnection();
    void closeTcpConnection(int fd);
    int serve(int fd);
};
