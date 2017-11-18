#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

#include <vector>
#include <map>
#include <pthread.h>

using namespace std;


typedef struct _thread_task
{
	pthread_mutex_t mtx;
	pthread_cond_t cond;
	void (*func)(void *);
	void *args;
	bool bUsed;
}stThreadTask;

typedef enum _POOL_STATE
{
	ENUM_THREAD_RUNNING = 1,
	ENUM_THREAD_FINISH ,
	ENUM_THREAD_DEL,
	ENUM_THREAD_FAIL,
}E_THREAD_STATE;

typedef map<pthread_t, stThreadTask> mapThreadTask;

class threadPool
{
public:
	threadPool();
	threadPool(int n);
	virtual ~threadPool();

	int thread_add(void (*function)(void *), void *args);
	int thread_del(pthread_t pid);
	void show();

public:
	#define DEFAULT_THREAD_NUM  1
	#define MAX_THREAD_NUM  8192
	int THREAD_TOTAL;
	pthread_mutex_t _mtx;
	pthread_cond_t _cond;
	int _runningThreadNum;
	int _availableThreadNum;
	mapThreadTask _mThread;

private:
	void init(int n);
	static void* threadpool_handle(void *args);
	int update_pool_state(E_THREAD_STATE e);
	inline mapThreadTask::iterator find_mThread(pthread_t pid);
	inline int delete_mThread(pthread_t pid);
	inline mapThreadTask::iterator end_mThread() 
	{
		return _mThread.end();	
	}

};

#endif

