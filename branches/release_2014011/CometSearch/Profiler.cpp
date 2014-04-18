#include "Profiler.h"

struct ProcessProfile *Profiler::clocks;
struct ProcessProfile *Profiler::active;
struct ProcessProfile *Profiler::active_last;
Mutex  Profiler::profilerMutex;
int    Profiler::pid;

//Initializes profiler. Creates a mutex for thread safe behavior. Also starts profiling itself as "root".
void Profiler::Init(){

  clocks = new struct ProcessProfile();
  active = new struct ProcessProfile();
  active_last=active;

  Threading::CreateMutex(&profilerMutex);

  pid=1;
  strcpy(clocks->id,"root");
  strcpy(active->id,"root");
  active->count++;
  getExactTime(active->start);
  
}

//Stops self-profiling and reports summary to user. Then cleans up any stray processes and releases memory.
void Profiler::Release() {

  int64 tmp;
  struct ProcessProfile *p;

  StopTimer(0);
  Report();

  //MH: LIFO (a tad inefficient)
  p=active;
  if(p!=NULL){
    while(p->next!=NULL) p=active->next;
    while(p->prev!=NULL){
      getExactTime(tmp);
      AddTime(p->id,tmp-p->start);
      p=p->prev;
      delete p->next;
      p->next=NULL;
    }
    getExactTime(tmp);
    AddTime(p->id,tmp-p->start);
    delete p;
    p=NULL;
  }

  p=clocks;
  while(p->next!=NULL){
    p=p->next;
    delete p->prev;
  }
  delete p;
  p=NULL;

  Threading::DestroyMutex(profilerMutex);
  
}

//Reports time spent in each process, and the time currently residing in active processes.
//This function can be called at any time for interim results, but is also called by the
//destructor when the Profiler is deallocated.
void Profiler::Report(){

  Threading::LockMutex(profilerMutex);

  struct ProcessProfile *p;
  int64 tmp;

  p=clocks;
  printf("Completed Processes:\nID\tTime\n");
  printf("%s\t%lld\n",p->id,p->count);
  while(p->next!=NULL) {
    p=p->next;
    printf("%s\t%lld\n",p->id,p->count);  
  }

  p=active;
  getExactTime(tmp);
  printf("Active Processes:\nID\tTime\tCount\n");
  if(p!=NULL){
    printf("%s\t%lld\t%lld\n",p->id,tmp-p->start,p->count);
    while(p->next!=NULL){
      p=p->next;
      printf("%s\t%lld\t%lld\n",p->id,tmp-p->start,p->count);
    }
  }
  p=NULL;

  Threading::UnlockMutex(profilerMutex);

}

//Adds time to an existing profile. If the profile doesn't exist, creates one on the fly.
void Profiler::AddTime(char *id, int64 val){

  Threading::LockMutex(profilerMutex);

  struct ProcessProfile *p;
  p=clocks;
  while(strcmp(p->id,id)!=0){
    if(p->next==NULL){
      struct ProcessProfile *nc = new struct ProcessProfile();
      nc->count=val;
      strcpy(nc->id,id);
      p->next=nc;
      nc->prev=p;
      p=NULL;
      Threading::UnlockMutex(profilerMutex);
      return;
    }
    p=p->next;
  }
  p->count+=val;
  p=NULL;

  Threading::UnlockMutex(profilerMutex);

}

//Start new timer for a user-defined profile name. Returns the process ID to the user so that the timer
//can be stopped at the user's convenience.
int Profiler::StartTimer(char *id){

  Threading::LockMutex(profilerMutex);

  struct ProcessProfile *p;
  p=active_last;
  struct ProcessProfile *nc = new struct ProcessProfile();
  strcpy(nc->id,id);
  nc->pid=pid++;
  nc->count=1;
  p->next=nc;
  nc->prev=p;
  active_last=nc;
  getExactTime(nc->start);
  p=NULL;
  Threading::UnlockMutex(profilerMutex);
  return nc->pid;
}

//Stops a timer, then calls AddTimer to add the resulting value to the profile.
void Profiler::StopTimer(int id){

  Threading::LockMutex(profilerMutex);

  struct ProcessProfile *p;
  int64 tmp;
  p=active;
  while(p->pid!=id){
    if(p->next==NULL){
      printf("Timer is asynchronous.\n");
      Threading::UnlockMutex(profilerMutex);
      return;
    }
    p=p->next;
  }
  p->count--;
  if(p->count==0){
    getExactTime(tmp);
    AddTime(p->id,tmp-p->start);
    if(p->prev==NULL && p->next==NULL) {
      active=NULL;
      active_last=NULL;
    } else if(p->next==NULL) {
      active_last=p->prev;
    }
    delete p;
  }
  p=NULL;

  Threading::UnlockMutex(profilerMutex);

}
