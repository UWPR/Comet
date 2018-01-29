/*
   Copyright 2012 University of Washington

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include "Common.h"
#include "CometDataInternal.h"
#include "CometCheckForUpdates.h"

#ifndef NOUPDATECHECK
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdarg.h>
#ifdef WIN32
#include <io.h>
#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#include <err.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif
#endif

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
   const char * host = "proteomicsresource.washington.edu";
   const char * page = "dist/version.html";

#ifdef WIN32
#define snprintf _snprintf
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
      if (connect(s, res->ai_addr, res->ai_addrlen) < 0)
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
#ifdef WIN32
   WSACleanup();
#endif

   return 0;
#endif
}
