/*======================================================== 
** University of Illinois/NCSA 
** Open Source License 
**
** Copyright (C) 2011,The Board of Trustees of the University of 
** Illinois. All rights reserved. 
**
** Developed by: 
**
**    Research Group of Professor Sam King in the Department of Computer 
**    Science The University of Illinois at Urbana-Champaign 
**    http://www.cs.uiuc.edu/homes/kingst/Research.html 
**
** Copyright (C) Sam King
**
** Permission is hereby granted, free of charge, to any person obtaining a 
** copy of this software and associated documentation files (the 
** Software), to deal with the Software without restriction, including 
** without limitation the rights to use, copy, modify, merge, publish, 
** distribute, sublicense, and/or sell copies of the Software, and to 
** permit persons to whom the Software is furnished to do so, subject to 
** the following conditions: 
**
** Redistributions of source code must retain the above copyright notice, 
** this list of conditions and the following disclaimers. 
**
** Redistributions in binary form must reproduce the above copyright 
** notice, this list of conditions and the following disclaimers in the 
** documentation and/or other materials provided with the distribution. 
** Neither the names of Sam King or the University of Illinois, 
** nor the names of its contributors may be used to endorse or promote 
** products derived from this Software without specific prior written 
** permission. 
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
** IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR 
** ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
** SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE. 
**========================================================== 
*/

#include "Cache.h"

#include <iomanip>
#include <iostream>

#include <assert.h>

using namespace std;

static Cache globalCache;

static string reply404 = "HTTP/1.1 404 Not Found\r\nServer: twproxy\r\nConnection: close\r\nContent-Length: 0\r\n\r\n";

static string CONNECT_REPLY = "HTTP/1.1 200 Connection Established\r\n\r\n";

Cache *cache()
{
    return &globalCache;
}

Cache::Cache()
{

}

void Cache::handleResponse(MySocket *browserSock, MySocket *replySock, string request)
{
    if(!replySock->write_bytes(request)) {
        // XXX FIXME we should do something other than 404 here
        browserSock->write_bytes(reply404);
        return;
    }

    unsigned char buf[1024];
    int num_bytes;
    bool ret;
    while((num_bytes = replySock->read(buf, sizeof(buf))) > 0) {        
        ret = browserSock->write_bytes(buf, num_bytes);
        if(!ret) {
            break;
        }
    }
}

bool Cache::copyNetBytes(MySocket *readSock, MySocket *writeSock)
{
    unsigned char buf[1024];
    int ret;

    ret = readSock->read(buf, sizeof(buf));
    if(ret <= 0)
        return false;

    return writeSock->write_bytes(buf, ret);
}

void Cache::handleTunnel(MySocket *browserSock, MySocket *replySock)
{
    if(!browserSock->write_bytes(CONNECT_REPLY))
        return;

    int bFd = browserSock->getFd();
    int rFd = replySock->getFd();    

    int ret;
    fd_set readSet;

    int maxFd = (bFd > rFd) ? bFd : rFd;
    while(true) {
        FD_ZERO(&readSet);

        FD_SET(rFd, &readSet);
        FD_SET(bFd, &readSet);

        ret = select(maxFd+1, &readSet, NULL, NULL, NULL);

        if(ret <= 0)
            break;

        if(FD_ISSET(rFd, &readSet)) {
            if(!copyNetBytes(replySock, browserSock)) {
                break;
            }
        }

        if(FD_ISSET(bFd, &readSet)) {
            if(!copyNetBytes(browserSock, replySock)) {
                break;
            }
        }
    }
}

void Cache::getHTTPResponse(string host, string request, string /*url*/, int /*serverPort*/, 
                            MySocket *browserSock, bool isTunnel)
{
    assert(host.find(':') != string::npos);
    assert(host.find(':') < (host.length()-1));
    string portStr = host.substr(host.find(':')+1);
    string hostStr = host.substr(0, host.find(':'));
    int port;
    int ret = sscanf(portStr.c_str(), "%d", &port);
    assert((ret == 1) && (port > 0));
    MySocket *replySock = NULL;
    try {
        //cout << "making connection to " << hostStr << ":" << port << endl;
        replySock = new MySocket(hostStr.c_str(), port);
    } catch(char *e) {
        cout << e << endl;
    } catch(...) {
        cout << "unknown exception type, pid = " << getpid() << endl;
    }

    if(replySock == NULL) {
        cout << "returning 404" << endl;
        browserSock->write_bytes(reply404);
        return;
    }

    if(isTunnel) {
        handleTunnel(browserSock, replySock);
    } else {
        handleResponse(browserSock, replySock, request);
    }

    delete replySock;
}


