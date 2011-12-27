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

#include "Cache.h"

#include <iomanip>
#include <iostream>

#include <assert.h>
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>

#include "CacheEntry.h"
#include "dbg.h"

using namespace std;

static Cache globalCache;
int Cache::num_browsers = 0;

static string reply404 = "HTTP/1.1 404 Not Found\r\nServer: twproxy\r\nConnection: close\r\nContent-Length: 0\r\n\r\n";


extern int serverPorts[];

Cache *cache()
{
        return &globalCache;    
}


MySocket *Cache::getReplySocket(string host, bool isSSL)
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
                if(isSSL) {
                        replySock->enableSSLClient();
                }
        } catch(char *e) {
                cout << e << endl;
        } catch(...) {
                cout << "could not connect to " << hostStr << ":" << port << endl;
        }
        return replySock;
}

//XXX: should check url, method, cookie, possible even port
CacheEntry *Cache::find(string url, string /*request*/)
{
        assert(false);
        return NULL;
}

void Cache::addToStore(string url, CacheEntry *ent)
{
        assert(false);
}

int Cache::votingFetchInsertWriteback(string url, string request, int browserId,
                                      MySocket *browserSock, string host, bool isSSL,
                                      MySocket *replySock)
{
        assert(false);
        return 0;
}

int Cache::sendBrowser(MySocket *browserSock, CacheEntry *ent, int browserId)
{
        assert(false);
        return 0;
}

void Cache::getHTTPResponseVote(string host, string request, string url, int serverPort, 
                                MySocket *browserSock, bool isSSL, MySocket *replySock)
{
        assert(false);
        int browserId = -1;
        pthread_mutex_lock(&cache_mutex);
        browserId = serverPort - serverPorts[0];
        cache_dbg("cache lock browser %d  %s\n", browserId, url.c_str());
        votingFetchInsertWriteback(url, request, browserId, browserSock, host, isSSL,
                                   replySock);
        cache_dbg("cache UNlock browser %d  %s\n", browserId, url.c_str());
        pthread_mutex_unlock(&cache_mutex);
        cache_dbg("cache BROADCAST browser %d  %s\n", browserId, url.c_str());
        pthread_cond_broadcast(&cache_cond);
}

static void dbg_fetch(int ret) {
        assert(false);
}


int Cache::fetch(CacheEntry *ent, string host, bool isSSL, int browserId, MySocket *replySock)
{
        assert(false);
        return 0;
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

void Cache::getHTTPResponseNoVote(string host, string request, string url, int serverPort, 
                                  MySocket *browserSock, bool isSSL, MySocket *replySock)
{
        if(replySock == NULL) {
                cout << "returning 404" << endl;
                browserSock->write_bytes(reply404);
                return;
        }
        handleResponse(browserSock, replySock, request);

        delete replySock;
}

void Cache::setNumBrowsers(const int num) 
{
        num_browsers = (int)num;
}

Cache::Cache()
{
        pthread_mutex_init(&cache_mutex, NULL);
        pthread_cond_init(&cache_cond, NULL);
}
Cache::~Cache()
{
        pthread_cond_destroy(&cache_cond);
        pthread_mutex_destroy(&cache_mutex);
}


