#ifndef _CACHE_H_
#define _CACHE_H_

#include <string>
#include "MySocket.h"

class Cache {
 public:
    Cache();

    void getHTTPResponse(std::string host, std::string request, std::string url, 
                         int serverPort, MySocket *browserSock);

};



Cache *cache();

#endif
