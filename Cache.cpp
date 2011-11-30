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

Cache *cache() {
    return &globalCache;
}

Cache::Cache() {

}

void Cache::getHTTPResponse(string host, string request, string /*url*/, int /*serverPort*/, 
                             MySocket *browserSock) {
    assert(host.find(':') != string::npos);
    assert(host.find(':') < (host.length()-1));
    string portStr = host.substr(host.find(':')+1);
    string hostStr = host.substr(0, host.find(':'));
    int port;
    int ret = sscanf(portStr.c_str(), "%d", &port);
    assert((ret == 1) && (port > 0));
    MySocket *replySock = NULL;
    try {
        replySock = new MySocket(hostStr.c_str(), port);
    } catch(char *e) {
        cout << e << endl;
    } catch(...) {
        cout << "unknown exception type, pid = " << getpid() << endl;
    }

    if(replySock == NULL) {
        browserSock->write_bytes(reply404);        
        return;
    }

    if(!replySock->write_bytes(request)) {
        // XXX FIXME we should do something other than 404 here
        browserSock->write_bytes(reply404);
        delete replySock;
        return;
    }

    unsigned char buf[1024];
    int num_bytes;
    while((num_bytes = replySock->read(buf, sizeof(buf))) > 0) {
        browserSock->write_bytes(buf, num_bytes);
    }

    delete replySock;
}


