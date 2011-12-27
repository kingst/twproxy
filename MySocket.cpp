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

#include "MySocket.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <assert.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

using namespace std;

#define CHK_SSL(err) if ((err)==-1) { ERR_print_errors_fp(stderr); exit(2); }
#define CHK_NULL(x) if ((x)==NULL) exit (1)

#define HOME "./"
#define CERTF  HOME "cacert.pem"
#define KEYF  HOME  "privkey.pem"


MySocket::MySocket(const char *inetAddr, int port)
{
        struct sockaddr_in server;
        struct addrinfo hints;
        struct addrinfo *res;

        isSSL = false;
        ctx = NULL;
        ssl = NULL;

        // set up the new socket (TCP/IP)
        sockFd = socket(AF_INET,SOCK_STREAM,0);
    
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        int ret = getaddrinfo(inetAddr, NULL, &hints, &res);
        if(ret != 0) {
                string str;
                str = string("Could not get host ") + string(inetAddr);
                throw MySocketException(str.c_str());
        }
    
        server.sin_addr = ((struct sockaddr_in *) (res->ai_addr))->sin_addr;
        server.sin_port = htons((short) port);
        server.sin_family = AF_INET;
        freeaddrinfo(res);
    
        // conenct to the server
        if( connect(sockFd, (struct sockaddr *) &server,
                    sizeof(server)) == -1 ) {
                throw MySocketException("Did not connect to the server");
        }
    
}
MySocket::MySocket(void)
{
        sockFd = -1;
        isSSL = false;
        ctx = NULL;
        ssl = NULL;
}

MySocket::MySocket(int socketFileDesc)
{
        sockFd = socketFileDesc;
        isSSL = false;
        ctx = NULL;
        ssl = NULL;
}

MySocket::~MySocket(void)
{
        close();
}

void MySocket::enableSSLServer(void)
{
        if(sockFd < 0) return;

        ctx = SSL_CTX_new (SSLv23_server_method());
        if (!ctx) {
                ERR_print_errors_fp(stderr);
                exit(2);
        }
    
        if (SSL_CTX_use_certificate_chain_file(ctx, CERTF) <= 0) {
                ERR_print_errors_fp(stderr);
                exit(3);
        }
        if (SSL_CTX_use_PrivateKey_file(ctx, KEYF, SSL_FILETYPE_PEM) <= 0) {
                ERR_print_errors_fp(stderr);
                exit(4);
        }
    
        if (!SSL_CTX_check_private_key(ctx)) {
                fprintf(stderr,"Private key does not match the certificate public key\n");
                exit(5);
        }   

        ssl = SSL_new (ctx);                           CHK_NULL(ssl);
        SSL_set_fd (ssl, sockFd);
        int err = SSL_accept (ssl);                        CHK_SSL(err);
  
        printf ("SSL connection using %s\n", SSL_get_cipher (ssl));
        isSSL = true;
}

void MySocket::enableSSLClient(void)
{
        if(sockFd < 0) return;

        ctx = SSL_CTX_new (SSLv23_client_method());
        if (!ctx) {
                ERR_print_errors_fp(stderr);
                exit(2);
        }

        ssl = SSL_new (ctx);                         CHK_NULL(ssl);
        SSL_set_fd (ssl, sockFd);
        int err = SSL_connect (ssl);                     CHK_SSL(err);
    
        printf ("SSL connection using %s\n", SSL_get_cipher (ssl));

        X509 *server_cert = SSL_get_peer_certificate (ssl);       CHK_NULL(server_cert);
        printf ("Server certificate:\n");

        char *str = X509_NAME_oneline (X509_get_subject_name (server_cert),0,0);
        CHK_NULL(str);
        printf ("\t subject: %s\n", str);
        OPENSSL_free (str);

        str = X509_NAME_oneline (X509_get_issuer_name  (server_cert),0,0);
        CHK_NULL(str);
        printf ("\t issuer: %s\n", str);
        OPENSSL_free (str);

        X509_free (server_cert);
        isSSL = true;
}

int MySocket::write(const void *buffer, int len)
{
        if(sockFd<0) return ENOT_CONNECTED;
    
        int ret;
    
        if(isSSL) {
                ret = SSL_write(ssl, buffer, len);
        } else {
                ret = ::write(sockFd, buffer, len);
        }    

        if(ret != len) return ESOCKET_ERROR;
    
        return ret;
}

bool MySocket::write_bytes(string buffer)
{
        return write_bytes(buffer.c_str(), buffer.size());
}
bool MySocket::write_bytes(const void *buffer, int len)
{
        const unsigned char *buf = (const unsigned char *) buffer;
        int bytesWritten = 0;

        while(len > 0) {
                bytesWritten = this->write(buf, len);
                if(bytesWritten <= 0) {
                        return false;
                }
                buf += bytesWritten;
                len -= bytesWritten;
        }

        return true;

}

int MySocket::read(void *buffer, int len)
{
        if(sockFd<0) return ENOT_CONNECTED;
    
        int ret;
    
        if(isSSL) {
                ret = SSL_read(ssl, buffer, len);
        } else {
                ret = ::read(sockFd, buffer, len);
        }
    
        if(ret == 0) return ECONN_CLOSED;
        if(ret < 0) return ESOCKET_ERROR;
  
        return ret;
}

void MySocket::close(void)
{
        if(sockFd<0) return;
    
        ::close(sockFd);

        sockFd = -1;

        isSSL = false;

        if(ssl != NULL)
                SSL_free(ssl);

        if(ctx != NULL)
                SSL_CTX_free(ctx);

        ssl = NULL;
        ctx = NULL;
}
