#ifndef _CACHE_H_
#define _CACHE_H_

#include <string>
#include "MySocket.h"

class Cache {
 public:
    Cache();

    void getHTTPResponse(std::string host, std::string request, std::string url, 
                         int serverPort, MySocket *browserSock, bool isTunnel);

 protected:
    void handleResponse(MySocket *browserSock, MySocket *replySock, std::string request);
    void handleTunnel(MySocket *browserSock, MySocket *replySock);
    bool copyNetBytes(MySocket *readSock, MySocket *writeSock);

};



Cache *cache();

#endif
