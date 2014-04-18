/*
   Copyright 2014 University of Washington, Institute of Systems Biology

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

#ifndef _PROFILER_H
#define _PROFILER_H

#include <iostream>
#include "Threading.h"

#ifdef _WIN32
  typedef __int64 int64;
  #define getExactTime(a) QueryPerformanceCounter((LARGE_INTEGER*)&a)
  #define getTimerFrequency(a) QueryPerformanceFrequency((LARGE_INTEGER*)&a)
  #define toMicroSec(a) (a)
  #define timeToSec(a,b) (a/b)
#else //These are the Linux functions, I think. I did not test them.
  typedef uint64_t int64;
  #define getExactTime(a) gettimeofday(&a,NULL)
  #define toMicroSec(a) a.tv_sec*1000000+a.tv_usec
  #define getTimerFrequency(a) (a)=1
  #define timeToSec(a,b) (a/1000000)
#endif

struct ProcessProfile{
  char id[128];         //User defined name
  int pid;              //processID assigned by Profiler
  int64 start;          //Start time
  int64 count;          //Sorry, this is a bit of cruft. Might utilize it later.
  ProcessProfile* prev; //For creating a rudimentary linked list
  ProcessProfile* next; //Ditto
  ProcessProfile(){
    id[0]='\0';
    pid=0;
    start=0;
    count=0;
    prev=NULL;
    next=NULL;
  }
  ~ProcessProfile(){    //The rudimentary linked list manages itself through this destructor
    if(prev!=NULL) prev->next=next;
    if(next!=NULL) next->prev=prev;
    prev=NULL;
    next=NULL;
  }
};

class Profiler 
{
public:
  static int StartTimer(char *id);  //Start profiling under user-specified name, return pid
  static void StopTimer(int id);    //Stop profiling the pid.
  static void Init();               //Initializes profiler. Call once at the start of the program
  static void Release();            //Cleans up profiler memory and makes final report. Call at the end of the program.
  static void Report();             //The report of active processes and accumulated processing time.

private:
  static struct ProcessProfile *clocks;       //The list of profiles collected
  static struct ProcessProfile *active;       //The list of active processes
  static struct ProcessProfile *active_last;  //Convenience pointer for fast insertion of new active processes.
  static Mutex  profilerMutex;                //Make it thread safe!
  static int    pid;                          //The profiler has its own process id and profiles itself. Cerebral!

  static void AddTime(char *id, int64 val);   //When a process finishes, add the results to a profile.
  
};



#endif
