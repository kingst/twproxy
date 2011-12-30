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
** Copyright (C) Sam King and Hui Xue
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
#include <openssl/err.h>
#include <openssl/ssl.h>

#include "MyServerSocket.h"
#include "HTTPRequest.h"
#include "Cache.h"
#include "dbg.h"
#include "time.h"

using namespace std;

int serverPorts[] = {8808, 8809, 8810};
#define NUM_SERVERS (sizeof(serverPorts) / sizeof(serverPorts[0]))

static string CONNECT_REPLY = "HTTP/1.1 200 Connection Established\r\n\r\n";

struct client_struct {
        MySocket *sock;
        int serverPort;
};

struct server_struct {
        int serverPort;
};

pthread_t server_threads[NUM_SERVERS];
static int gVOTING = 0;
static int numThreads = 0;

static pthread_mutex_t *lock_cs;
static long *lock_count;

void run_client(MySocket *sock, int serverPort)
{
        HTTPRequest *request = new HTTPRequest(sock, serverPort);
        while(!sock->isClosed() && request->readRequest()) {
                //hx: I need request itself, to get REAL url from https
                //this url passed in, is the url in CONNECT. what we need
                //for voting, is the url in the encrypted request
                //this interface could be changed to passing HTTPRequest* directly
                //TODO: let me deal with this later in voting codes
                cache()->getHTTPResponse(request->getHost(), request->getRequest(),
                                         request->getUrl(), serverPort, sock,
                                         request->isConnect());
                if(request->isConnect()) {
                        break;
                }
                delete request;
                request = new HTTPRequest(sock, serverPort);
        }

        sock->close();
        delete request;
        delete sock;
}

/*
void run_client(MySocket *sock, int serverPort)
{
        HTTPRequest *request = new HTTPRequest(sock, serverPort);
    
        if(!request->readRequest()) {
                cout << "did not read request" << endl;
        } else {    
                bool error = false;
                bool isSSL = false;

                string host = request->getHost();
                string url = request->getUrl();

                MySocket *replySock = NULL;
        
                if(request->isConnect()) {
                        //deal with MITM in later patches
                        assert(false);                        
                        if(!sock->write_bytes(CONNECT_REPLY)) {
                                error = true;
                        } else {
                                delete request;
                                replySock = cache()->getReplySocket(host, true);
                                // need proxy <--> remotesite socket
                                // for information needed to fake a
                                // certificate
                                //
                                // sock->enableSSLServer(replySock);
                                isSSL = true;
                                request = new HTTPRequest(sock, serverPort);
                                if(!request->readRequest()) {
                                        error = true;
                                }
                        }            
                } else {
                        replySock = cache()->getReplySocket(host, false);
                }        

                if(!error) {
                        string req = request->getRequest();
                        if(gVOTING == 0) {
                                cache()->getHTTPResponseNoVote(host, req, url, serverPort,
                                                               sock, isSSL, replySock);
                        } else {
                                //if(isSSL == true)
                                //cache()->getHTTPResponseVote(host, req, url, serverPort,
                                //sock, isSSL, replySock);
                                assert(false);
                                //cache()->getHTTPResponseVote(host, req, url, serverPort,
                                                             sock, isSSL, replySock);
                        }
            
                }
        }    

        sock->close();
        delete request;
}
*/

void *client_thread(void *arg)
{
        struct client_struct *cs = (struct client_struct *) arg;
        MySocket *sock = cs->sock;
        int serverPort = cs->serverPort;

        delete cs;
    
        run_client(sock, serverPort);

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

void *server_thread(void *arg)
{
        struct server_struct *ss = (struct server_struct *)arg;
        int port = ss->serverPort;
        delete ss;
    
        MyServerSocket *server = new MyServerSocket(port);
        assert(server != NULL);
        MySocket *client;
        while(true) {
                try {
                        client = server->accept();
                        start_client(client, port);
                } catch(MySocketException e) {
                        cerr << e.toString() << endl;
                        exit(1);
                }

        }    
        return NULL;
}


pthread_t start_server(int port)
{
        cout << "starting server on port " << port << endl;
        server_struct *ss = new struct server_struct;
        ss->serverPort = port;
        pthread_t tid;
        int ret = pthread_create(&tid, NULL, server_thread, ss);
        assert(ret == 0);
        return tid;
}

static void get_opts(int argc, char *argv[])
{
        int c;
        while((c = getopt(argc, argv, "v")) != EOF) {
                switch(c) {
                case 'v':
                        gVOTING = 1;
                        break;
                default:
                        cerr << "Wrong Argument." << endl;
                        exit(1);
                        break;
                }
        }
}

//from mttest.c in openssl/crypto/threads/mttest.c
static void pthreads_locking_callback(int mode, int type, char *file, int line)
{
#ifdef undef
    fprintf(stderr,"thread=%4d mode=%s lock=%s %s:%d\n",
            CRYPTO_thread_id(),
            (mode&CRYPTO_LOCK)?"l":"u",
            (type&CRYPTO_READ)?"r":"w",file,line);
#endif
    if (mode & CRYPTO_LOCK) {
        pthread_mutex_lock(&(lock_cs[type]));
        lock_count[type]++;
    } else {
        pthread_mutex_unlock(&(lock_cs[type]));
    }
}
//from mttest.c in openssl/crypto/threads/mttest.c
static unsigned long pthreads_thread_id(void)
{
    unsigned long ret;

    ret=(unsigned long)pthread_self();
    return(ret);
}

//from mttest.c in openssl/crypto/threads/mttest.c
static void openssl_thread_setup()
{
    int i;
    
    lock_cs = (pthread_mutex_t *)OPENSSL_malloc(CRYPTO_num_locks() * sizeof(pthread_mutex_t));
    lock_count = (long *)OPENSSL_malloc(CRYPTO_num_locks() * sizeof(long));
    for (i = 0; i < CRYPTO_num_locks(); i++)
    {
        lock_count[i] = 0;
        pthread_mutex_init(&(lock_cs[i]),NULL);
    }
    
    CRYPTO_set_id_callback((unsigned long (*)())pthreads_thread_id);
    CRYPTO_set_locking_callback((void (*)(int, int, const char *, int))pthreads_locking_callback);
}
//from mttest.c in openssl/crypto/threads/mttest.c
static void openssl_thread_cleanup()
{
    int i;

    CRYPTO_set_locking_callback(NULL);
    fprintf(stderr,"cleanup\n");
    for (i = 0; i < CRYPTO_num_locks(); i++)
    {
        pthread_mutex_destroy(&(lock_cs[i]));
        fprintf(stderr,"%8ld:%s\n",lock_count[i], CRYPTO_get_lock_name(i));
    }
    OPENSSL_free(lock_cs);
    OPENSSL_free(lock_count);

    fprintf(stderr,"done cleanup\n");

}


int main(int argc, char *argv[])
{
        // if started with "-v" option, voting will be
        // enabled. Otherwise, just a plain proxy
        get_opts(argc, argv);  
        // get socket write errors from write call
        signal(SIGPIPE, SIG_IGN);

        // initialize ssl library
        SSL_load_error_strings();
        SSL_library_init();

        openssl_thread_setup();
        
        cout << "number of servers: " << NUM_SERVERS << endl;

        // when generating serial number for X509, need random number
        srand(time(NULL));
        //Cache::setNumBrowsers(NUM_SERVERS);
    
        pthread_t tid;
        int ret;
        for(unsigned int idx = 0; idx < NUM_SERVERS; idx++) {
                tid = start_server(serverPorts[idx]);
                server_threads[idx] = tid;
        }

        for(unsigned int idx = 0; idx < NUM_SERVERS; idx++) {
                ret = pthread_join(server_threads[idx], NULL);
                assert(ret == 0);
        }

        openssl_thread_cleanup();
        
        return 0;
}
