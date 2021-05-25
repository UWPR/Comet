/*
   Copyright (C) 2021 David Shteynberg, Institute for Systems Biology

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

#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_
#include <iostream>

#include <mutex>
#include <condition_variable>
#include <deque>
#include <vector>
#include <functional>
#include <chrono>

#include <thread>
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <process.h>
#else
#endif


#ifndef _WIN32
#include <pthread.h>
#endif

#define VERBOSE 0


class  thpldata {
 public:
  thpldata() {};
  void setThreadNum(int i) {
    thread_no = i;
  }

  int thread_no;
  void* tp;
};

#ifdef _WIN32
unsigned int  threadStart(void* ptr); 
#else
void* threadStart(void* ptr); 
#endif  

class ThreadPool
{
    public:
  
  
  ThreadPool () : shutdown_ (false)
    {
      running_count_ = 0;
    }
  
  ThreadPool (int threads) : shutdown_ (false)
    {
      // Create the specified number of threads
      running_count_ = 0;
      threads_.reserve (threads);
      fillPool(threads);
      
    }
  
  void fillPool(int threads) 
  {
    thpldata* data = new thpldata();

#ifdef _WIN32
    lock_ = CreateMutex( 
			      NULL,              // default security attributes
			      FALSE,             // initially not owned
			      NULL);             // unnamed mutex
    
    if (lock_ == NULL) 
      {
	printf("CreateMutex error: %d\n", (int)GetLastError());
	exit(1);
      }
    countlock_ = CreateMutex( 
			      NULL,              // default security attributes
			      FALSE,             // initially not owned
			      NULL);             // unnamed mutex
    
    if (countlock_ == NULL) 
      {
	printf("CreateMutex error: %d\n", (int)GetLastError());
	exit(1);
      }
#else
    pthread_mutex_init(&lock_, NULL);
    pthread_mutex_init(&countlock_, NULL);
#endif
  
#ifdef _WIN32
    HANDLE *pHandle = new HANDLE[threads];
#else
    pthread_t *pHandle = new pthread_t[threads];
#endif
    
    threads_.reserve (threads);
    data_.reserve(threads);
    for (int i = 0; i < threads; ++i) {
      
      data->setThreadNum(i);
      data->tp = this;
      data_.push_back(data);

      threads_[i] = pHandle[i];
      
#ifdef _WIN32
      threads_[i] = (HANDLE)_beginthreadex(0,0,
				   &threadStart,
				   (void*) data_[i],
				   0,NULL);

#else
      pthread_create(&threads_[i],NULL,threadStart, 
		     (void*) data_[i]);
#endif
    }
    
  }

  void wait_on_threads() {
    while(haveJob())  {
      std::this_thread::sleep_for(std::chrono::microseconds(1000));
#ifndef _WIN32
      pthread_yield();
#else
      std::this_thread::yield();
#endif  
    }
  }

  void wait_for_available_thread() {
    this->LOCK(&countlock_);
    while(data_.size() > 0 && running_count_ >= (int)data_.size())  {
      this->UNLOCK(&countlock_);      
#ifndef _WIN32
      pthread_yield();
#else
      std::this_thread::yield();
#endif
      this->LOCK(&countlock_);
    }
    this->UNLOCK(&countlock_);
  }

  void drainPool() 
  {
    shutdown_ = true;    
    for (size_t i =0 ; i < threads_.size(); i++)  {
      delete data_[i];
      void * ignore = 0;
#ifdef _WIN32
     WaitForSingleObject(threads_[i],INFINITE);
#else
     pthread_join(threads_[i],&ignore);
#endif
    }
    data_.clear();

  } 
  
  ~ThreadPool ()
    {
      
	for (size_t i =0 ; i < threads_.size(); i++)  {
	  delete data_[i];
	  void * ignore = 0;
#ifdef _WIN32
	  WaitForSingleObject(threads_[i],INFINITE);
#else
	  pthread_join(threads_[i],&ignore);
#endif
	}
	data_.clear();
	
    }

    void doJob (std::function <void (void)> func)
    {
      
      // Place a job on the queu and unblock a thread

      this->LOCK(&lock_);
      jobs_.emplace_back (std::move (func));
      this->UNLOCK(&lock_);

    }
  
    void incrementRunningCount() 
    {
      this->LOCK(&countlock_);
      running_count_++;
      this->UNLOCK(&countlock_);
    }
  
    void decrementRunningCount() 
    {
      this->LOCK(&countlock_);
      running_count_--;
      this->UNLOCK(&countlock_);
    }


    bool haveJob() {
      bool rtn;
      this->LOCK(&lock_);
      rtn = !jobs_.empty() || running_count_;
      this->UNLOCK(&lock_);
      
      return rtn;
    }

    int getAvailableThreads(int user) {
      //Borrowed from Comet
      int iNumCPUCores;
#ifdef _WIN32
      SYSTEM_INFO sysinfo;
      GetSystemInfo( &sysinfo );
      iNumCPUCores = sysinfo.dwNumberOfProcessors;
#else
      iNumCPUCores= sysconf( _SC_NPROCESSORS_ONLN );

#endif

      iNumCPUCores += user;

      if (iNumCPUCores < 1) iNumCPUCores = 1;
      return iNumCPUCores;

    }



void LOCK(
#ifdef _WIN32
	  HANDLE* mutex
#else
	  pthread_mutex_t* mutex
#endif
	  )  {
  
#ifdef _WIN32
  WaitForSingleObject( *mutex,    // handle to mutex
		       INFINITE); //#include "windows.h"
#else	
  pthread_mutex_lock( &*mutex );  //#include <pthread.h>
#endif 
}

void UNLOCK(
#ifdef _WIN32
	  HANDLE* mutex
#else
	  pthread_mutex_t* mutex
#endif
	  )  {
  
#ifdef _WIN32
  ReleaseMutex( *mutex );          //#include "windows.h"
#else	
  pthread_mutex_unlock( &*mutex ); //#include <pthread.h>
#endif 
}


  int running_count_;
  bool shutdown_;
  std::deque <std::function <void (void)>> jobs_;
  
  vector<thpldata*> data_;

#ifdef _WIN32

  HANDLE lock_;
  HANDLE countlock_;
  
  
  std::vector<HANDLE> threads_;
    
    
#else
  pthread_mutex_t lock_;
  pthread_mutex_t countlock_;
  
  std::vector<pthread_t> threads_;
#endif


};


#ifdef _WIN32
inline unsigned int  threadStart(LPVOID ptr) 
#else
inline void* threadStart(void* ptr) 
#endif  
    {
        std::function <void (void)> job;

	thpldata* data = (thpldata *) ptr; 
	int i = data->thread_no;
        ThreadPool* tp = (ThreadPool*)data->tp;
	while (1)
        {
	      tp->LOCK(&tp->lock_);
	      {

                while (! tp->shutdown_ && tp->jobs_.empty()) {
					tp->UNLOCK(&tp->lock_);
#ifndef _WIN32
					pthread_yield();
#else
               std::this_thread::yield();
#endif 

					if (tp->threads_.capacity() == 0)
#ifdef _WIN32
		    return 1;
#else
		    return NULL;
#endif

					tp->LOCK(&tp->lock_);
		}
		   

                if (tp->jobs_.empty ())
                {
		  // No jobs to do and we are shutting down
		  if (VERBOSE)
                    std::cerr << "Thread " << i << " terminates" << std::endl;
		  tp->UNLOCK(&tp->lock_);
#ifdef _WIN32
		    return 1;
#else
		    return NULL;
#endif
		    break;
		  
		}
		else {
		  //std::cerr << "Thread " << i << " does a job" << std::endl;
		  job = std::move (tp->jobs_.front ());
		  tp->jobs_.pop_front();
		  tp->UNLOCK(&tp->lock_);
		  // Do the job without holding any locks
		  try{
		    job();
		  }catch (std::exception& e){
		    cerr << "WARNING: running job exception ... " << e.what() << " ... exiting ... " <<  endl;
#ifdef _WIN32
		    return 1;
#else
		    return NULL;
#endif 
		    break;
		  }
		}
	      
	      }
	
        }

    }

#endif // _THREAD_POOL_H_
