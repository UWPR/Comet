/*
   Copyright 2013 University of Washington

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

#ifndef _COMETCHECKFORUPDATES_H_
#define _COMETCHECKFORUPDATES_H_

#include "Common.h"
#include "CometDataInternal.h"

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
#else
#include <unistd.h>
#include <err.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif

class CometCheckForUpdates
{
public:
   CometCheckForUpdates();
   ~CometCheckForUpdates();

   static void CheckForUpdates(char *szOut);

private:
   static void SendAnalyticsHit(void);

};

#endif // _COMETCHECKFORUPDATES_H_
