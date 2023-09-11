/*
MIT License

Copyright (c) 2023 University of Washington's Proteomics Resource

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "CometCheckForUpdates.h"


CometCheckForUpdates::CometCheckForUpdates()
{
}


CometCheckForUpdates::~CometCheckForUpdates()
{
}


static void fail (int test, const char * format, ...)
{
   if (test)
   {
      va_list args;
      va_start (args, format);
      vfprintf (stderr, format, args);
      va_end (args);
      exit (EXIT_FAILURE);
   }
}


void CometCheckForUpdates::CheckForUpdates(char *szOut)
{
   
#ifndef NOUPDATECHECK
#define BSIZE 4096     // hold return buffer from webpage

   struct addrinfo hints, *res, *res0;
   int error;
   int s;                                    // Get one of the web pages here
   const char *host = "comet-ms.sourceforge.net";
   const char *page = "current_version.htm";

#ifdef _WIN32
#define snprintf _snprintf
#define close _close
   WSADATA wsaData;
   if (WSAStartup(0x202, &wsaData) != 0)
   {
      printf(" Error - WSAStartup initialization failure.\n");
      exit(1);
   }
#endif
   
   memset (&hints, 0, sizeof (hints));

   hints.ai_family = PF_UNSPEC;              // Don't specify what type of internet connection.
   hints.ai_socktype = SOCK_STREAM;
   error = getaddrinfo (host, "http", & hints, & res0);
   fail(error, gai_strerror (error));
   s = -1;

   for (res = res0; res; res = res->ai_next)
   {
      s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
      fail(s < 0, "socket: %s\n", strerror (errno));
      if (connect(s, res->ai_addr, res->ai_addrlen) != 0)
      {
         fprintf(stderr, "connect: %s\n", strerror (errno));
         close(s);
         exit(EXIT_FAILURE);
      }
      break;
   }

   if (s != -1)
   {
      int status;           // This holds return values from functions.
      char szMsg[BSIZE];     // szMsg is the request message that we will send to the server.
      char *pStr;

      status = snprintf(szMsg, BSIZE, "GET /%s HTTP/1.0\r\nHost: %s\r\nUser-Agent: Comet\r\n\r\n", page, host);
      fail(status == -1 || status>BSIZE, "snprintf failed.\n");

      status = send(s, szMsg, strlen (szMsg), 0);   // Send the request
      fail(status == -1, "send failed: %s\n", strerror (errno));

      while (1)
      {
         int bytes;                                  // The number of bytes received.
         char buf[BSIZE+10];                         // Our receiving buffer.
         bytes = recvfrom(s, buf, BSIZE, 0, 0, 0);  // Get "BSIZE" bytes from "s".

         if (bytes == 0)                             // Stop once there is nothing left to print.
            break;

         fail(bytes == -1, "%s\n", strerror (errno));
         buf[bytes] = '\0';                          // Nul-terminate the string before printing.

         if ((pStr = strstr(buf, "Comet 20"))!=NULL && strstr(buf, " rev. "))
         {
            char szOnlineVersion[128];
            char szCurrentVersion[128];

            strcpy(szOnlineVersion, pStr+6);
            while ((szOnlineVersion[strlen(szOnlineVersion)-1]) < 32 || (szOnlineVersion[strlen(szOnlineVersion)-1])>126)
               szOnlineVersion[strlen(szOnlineVersion)-1] = '\0';       // strip termination chars

            strcpy(szCurrentVersion, comet_version);

            if (strcmp(szCurrentVersion, szOnlineVersion) < 0 && !g_staticParams.options.bOutputSqtStream)
               sprintf(szOut+strlen(szOut), "  **UPDATE AVAILABLE**");

            break;
         }

         // parse out line containing string "Comet 20" for version e.g. "Comet 2018.01 rev. 5".
      }
   }

   freeaddrinfo(res0);

   SendAnalyticsHit(); // google analytics tracking
#ifdef _WIN32
   WSACleanup();
#endif

#endif
}


void CometCheckForUpdates::SendAnalyticsHit(void)
{
   int portno = 80;
   char *host = "www.google-analytics.com";

   struct hostent *server;
   struct sockaddr_in serv_addr;
   int sockfd, bytes, sent, total;

   char postData[1024];
   char message[1024];
   memset(message, 0, 1024);
   sprintf(postData, "v=1&tid=UA-35341957-1&cid=555&t=event&ec=comet&ea=exe&el=%s", comet_version);
   snprintf(message, 1024,
         "POST /collect HTTP/1.1\r\n"
         "Host: %s\r\n"
         "Content-type: application/x-www-form-urlencoded\r\n"
         "Content-length: %d\r\n\r\n"
         "%s\r\n", host, (int)strlen(postData), postData);

   // create the socket
   sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#ifdef _WIN32
   if (sockfd == INVALID_SOCKET)
   {
      WSACleanup();
      return;
   }
#else
   if (sockfd == -1)
      return;
#endif

   // lookup the ip address
   server = gethostbyname(host);
   if (server == NULL)
      return;

   // fill in the structure
   memset(&serv_addr,0,sizeof(serv_addr));
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_port = htons(portno);
   memcpy(&serv_addr.sin_addr.s_addr,server->h_addr,server->h_length);

   // connect the socket
   if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) != 0)
      return;

   // send the request
   total = strlen(message);
#ifdef _WIN32
   bytes = send(sockfd, message, total, 0);
   // no point in checking return value here as the
   // analytics message was sent or it wasn't
   closesocket(sockfd);
   WSACleanup();
#else
   sent = 0;
   do
   {
      bytes = write(sockfd,message+sent,total-sent);
      if (bytes < 0)
      {
         printf("ERROR writing message to socket");
         return;
      }
      if (bytes == 0)
         break;
      sent+=bytes;
   } while (sent < total);

   // close the socket
   close(sockfd);
#endif
}
