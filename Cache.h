#ifndef _CACHE_H_
#define _CACHE_H_

#include <string>
#include "MySocket.h"

class Cache {
 public:
    Cache();
        //XXX: Note that host and url for https are NOT real ones here.
        //They are the one in CONNECT message, thus only a domain name for url.
        //I need real one for voting purpose, I can get them after enableSSL
        //functions are called, by calling request->getUrl() again.
    void getHTTPResponse(std::string host, std::string request, std::string url, 
                         int serverPort, MySocket *browserSock, bool isTunnel);

 protected:
    void handleResponse(MySocket *browserSock, MySocket *replySock, std::string request);
    void handleTunnel(MySocket *browserSock, MySocket *replySock);
    bool copyNetBytes(MySocket *readSock, MySocket *writeSock);

};



Cache *cache();

#endif
