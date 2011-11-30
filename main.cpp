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

#include <iostream>

#include <assert.h>
#include <pthread.h>
#include <signal.h>

#include "MyServerSocket.h"
#include "HTTPRequest.h"
#include "Cache.h"

using namespace std;

int serverPorts[] = {8000};
#define NUM_SERVERS (sizeof(serverPorts) / sizeof(serverPorts[0]))

static pthread_mutex_t logMutex = PTHREAD_MUTEX_INITIALIZER;
static unsigned long numThreads = 0;

struct client_struct {
    MySocket *sock;
    int serverPort;
};

void run_client(MySocket *sock, int serverPort)
{
    HTTPRequest *request = new HTTPRequest(sock, serverPort);
    if(!request->readRequest()) {
        cout << "did not read request" << endl;
    } else {    
        cache()->getHTTPResponse(request->getHost(), request->getRequest(),
                                 request->getUrl(), serverPort, sock, request->isConnect());
    }    

    sock->close();
    delete sock;
    delete request;
}

void *client_thread(void *arg) {
    struct client_struct *cs = (struct client_struct *) arg;
    MySocket *sock = cs->sock;
    int serverPort = cs->serverPort;

    delete cs;

    pthread_mutex_lock(&logMutex);
    numThreads++;
    //cout << "numThread = " << numThreads << endl;
    pthread_mutex_unlock(&logMutex);

    run_client(sock, serverPort);

    pthread_mutex_lock(&logMutex);
    numThreads--;
    //cout << "numThread = " << numThreads << endl;
    pthread_mutex_unlock(&logMutex);    

    return NULL;
}

void start_client(MySocket *sock, int serverPort)
{
    struct client_struct *cs = new struct client_struct;
    cs->sock = sock;
    cs->serverPort = serverPort;

    pthread_t tid;
    int ret = pthread_create(&tid, NULL, client_thread, cs);
    assert(ret == 0);
    ret = pthread_detach(tid);
    assert(ret == 0);
}

void start_server(int port)
{
    cerr << "starting server on port " << port << endl;
    
    MyServerSocket *server = new MyServerSocket(port);
    MySocket *client;

    while(true) {
        client = server->accept();
        start_client(client, port);
    }
}


int main(int /*argc*/, char */*argv*/[])
{
    signal(SIGPIPE, SIG_IGN);
    for(unsigned int idx = 0; idx < NUM_SERVERS; idx++) {
        start_server(serverPorts[idx]);
    }
    
    return 0;
}
