

#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <stdlib.h>
#include "thread_pool.h"

#define DEBUG  0

void log(char *arg)
{
	if(DEBUG)
		cout<<arg<<endl;
}

volatile int gThreadTol = 0;

threadPool::threadPool()
{
	_runningThreadNum = 0;
	_availableThreadNum = 0;
	THREAD_TOTAL = 0;
	init(DEFAULT_THREAD_NUM);
}
threadPool::threadPool(int n)
{
	n = n>MAX_THREAD_NUM?MAX_THREAD_NUM:n;
	_runningThreadNum = 0;
	_availableThreadNum = 0;
	THREAD_TOTAL = 0;
	init(n);
}
threadPool::~threadPool()
{
	pthread_mutex_destroy(&_mtx);	
	pthread_cond_destroy(&_cond);	
	_mThread.clear();
}

void threadPool::init(int n)
{
	if(pthread_mutex_init(&_mtx, NULL) != 0)	
		cout<<"################ pthread_mutex_init err\n";
	if(pthread_cond_init(&_cond, NULL) != 0)	
		cout<<"################ pthread_conn_init err\n";
	
	int i;
	pthread_t pid;
	pthread_mutex_t mtx;
	pthread_cond_t cond;
	stThreadTask task;
	//	create threads
	for(i=0;i<n;i++)
	{

		if(pthread_mutex_init(&mtx, NULL) != 0)	
		{
			cout<<"################ pthread_mutex_init err\n";
			break;
		}
		if(pthread_cond_init(&cond, NULL) != 0)	
		{
			cout<<"################ pthread_mutex_init err\n";
			break;
		}
		task.func = NULL;
		task.args = NULL;
		task.mtx  = mtx;
		task.cond = cond;
		task.bUsed = false;

		pthread_mutex_lock(&_mtx);
		if(pthread_create(&pid, NULL, threadpool_handle, this) != 0)
		{
			cout<<"i:"<<i<<"################  pthread_create err\n";
			break;
		}
		_mThread.insert(pair<pthread_t,stThreadTask>(pid, task));
		pthread_mutex_unlock(&_mtx);
		THREAD_TOTAL++;
	}
	_availableThreadNum  = THREAD_TOTAL;
	cout<<"-------------threads pool have ["<<_availableThreadNum<<"] threads is available to be used\n";
}

/*
 * add function to thread pool
 * return pthreadid if success, otherwise -1
 */
int threadPool::thread_add(void (*function)(void*), void *args)
{
	int ret = -1;
	map<pthread_t, stThreadTask>::iterator itm;
	if(function == NULL)
		return -1;


	if(pthread_mutex_lock(&_mtx) != 0)
		return -1;
	do
	{
		for(itm=_mThread.begin();itm!=_mThread.end();itm++)
			if(itm->second.bUsed == false)
				break;
		if(itm == _mThread.end())
		{
			cout<<"thread pool have full. max size is:"<<THREAD_TOTAL<<endl;
			break;
		}

		pthread_mutex_lock(&(itm->second.mtx));
		itm->second.func = function;
		itm->second.args = args;
		itm->second.bUsed= true;
		if(pthread_cond_signal(&(itm->second.cond)) != 0)
		{
			pthread_mutex_unlock(&(itm->second.mtx));
			cout<<"################ pthread_cond_signal err\n";
			break;
		}
		pthread_mutex_unlock(&(itm->second.mtx));

		ret = itm->first;

	}while(0);
	if(pthread_mutex_unlock(&_mtx) != 0)
		return -1;

	return ret;
}

/*
 * delete the pthread from threads pool
 */
int threadPool::thread_del(pthread_t pid)
{
	map<pthread_t, stThreadTask>::iterator itm;
	itm = find_mThread(pid);
	if(itm == end_mThread())
		return -1;
	if(pthread_cancel(pid) != 0)
	{
		cout<<"pthread_cancel ["<<pid<<"]  failed!!!";
		return -1;
	}
	delete_mThread(pid);
	return pid;
}

void* threadPool::threadpool_handle(void *args)
{
	pthread_t pid;
	threadPool *th;
	map<pthread_t, stThreadTask>::iterator itm;
	if(args == NULL)
		return NULL;

	th = static_cast<threadPool*>(args);
	pid = pthread_self();

	usleep(1);
	// wait _mThread
	itm = th->find_mThread(pid);
	if(itm == th->end_mThread())
	{
		cout<<"\t######## no finded.........."<<pid<<endl;
		return NULL;
	}

	cout<<++gThreadTol<<"......[pthread:"<<pid<<"] running......\n";
	for(;;)
	{

		do
		{
			// wait cond signal
			pthread_mutex_lock(&(itm->second.mtx));
			pthread_cond_wait(&(itm->second.cond), &(itm->second.mtx));
			//cout<<pid<<" handle waited signal.....\n";

			th->update_pool_state(ENUM_THREAD_RUNNING);

			/* start work */
			itm->second.func(itm->second.args);

			pthread_mutex_unlock(&(itm->second.mtx));
		}while(0);

		/* update state after over .func()*/
		th->update_pool_state(ENUM_THREAD_FINISH);
		itm->second.bUsed = false;
	}

	return NULL;
}


/*
 * update the thread states of running,available,...etc
 */
int threadPool::update_pool_state(E_THREAD_STATE e)
{
	pthread_mutex_lock(&_mtx);
	switch(e)
	{
		case ENUM_THREAD_RUNNING:
			_runningThreadNum++;	
			if(_availableThreadNum > 0)
				_availableThreadNum--;	
			break;
		case ENUM_THREAD_FINISH:
			if(_runningThreadNum > 0)
				_runningThreadNum--;	
			_availableThreadNum++;	
			break;
		case ENUM_THREAD_FAIL:
			break;
		case ENUM_THREAD_DEL:
			//have bug,
			if(_runningThreadNum > 0)
				_runningThreadNum--;	
			if(_availableThreadNum > 0)
				_availableThreadNum--;	
			break;
		default:
			break;
	}
	pthread_mutex_unlock(&_mtx);
}


mapThreadTask::iterator threadPool::find_mThread(pthread_t pid)
{
	mapThreadTask::iterator itm;
	pthread_mutex_lock(&_mtx);
	itm = _mThread.find(pid);
	pthread_mutex_unlock(&_mtx);
	return itm;
}

int threadPool::delete_mThread(pthread_t pid)
{
	pthread_mutex_lock(&_mtx);
	_mThread.erase(pid);
	pthread_mutex_unlock(&_mtx);
	return _mThread.size();
}

void threadPool::show()
{
	cout<<"--------------- available:"<<_availableThreadNum
						<<" running:"<<_runningThreadNum<<endl;
	return;
	map<pthread_t, stThreadTask>::iterator itm;
	for(itm=_mThread.begin();itm!=_mThread.end();itm++)
	{
		cout<<itm->first<<endl;
	}
}

#if 1

volatile int gCB;
void func_cb(void *args)
{
	int i;
	for(i=0;i<1;i++)
	{
		cout<<pthread_self()<<" "<<(char*)args<<" func_cb running....."<<++gCB<<endl;
		usleep(1000);
	}

}

void top_state(void *args)
{
	threadPool *th;
	th = static_cast<threadPool*>(args);
	while(1)
	{
		usleep(512);
		if(gCB >= 1)
		th->show();
	}
}
int main()
{
	threadPool *tp = new threadPool(6);
	char str1[] = "hello111";
	char str2[] = "hello222";
	char str3[] = "hello333";
	tp->thread_add(top_state, tp);
	sleep(10);
	for(int i=0;i<5;i++)
	{
		switch(i%3)
		{
			case 0:	
			tp->thread_add(func_cb, (void*)str1);
			break;
			case 1:	
			tp->thread_add(func_cb, (void*)str2);
			break;
			case 2:	
			tp->thread_add(func_cb, (void*)str2);
			break;
		}
	}
	sleep(5);
	cout<<"main() finish.  gCB:"<<gCB<<endl;
	//tp->show();
}
#endif


