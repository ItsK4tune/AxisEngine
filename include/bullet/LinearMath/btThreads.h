

#ifndef BT_THREADS_H
#define BT_THREADS_H

#include "btScalar.h"  

#if defined(_MSC_VER) && _MSC_VER >= 1600

#define BT_OVERRIDE override
#endif

#ifndef BT_OVERRIDE
#define BT_OVERRIDE
#endif



const unsigned int BT_MAX_THREAD_COUNT = 64;  


bool btIsMainThread();
bool btThreadsAreRunning();
unsigned int btGetCurrentThreadIndex();
void btResetThreadIndexCounter();  







class btSpinMutex
{
	int mLock;

public:
	btSpinMutex()
	{
		mLock = 0;
	}
	void lock();
	void unlock();
	bool tryLock();
};












SIMD_FORCE_INLINE void btMutexLock(btSpinMutex* mutex)
{
#if BT_THREADSAFE
	mutex->lock();
#else
	(void)mutex;
#endif  
}

SIMD_FORCE_INLINE void btMutexUnlock(btSpinMutex* mutex)
{
#if BT_THREADSAFE
	mutex->unlock();
#else
	(void)mutex;
#endif  
}

SIMD_FORCE_INLINE bool btMutexTryLock(btSpinMutex* mutex)
{
#if BT_THREADSAFE
	return mutex->tryLock();
#else
	(void)mutex;
	return true;
#endif  
}




class btIParallelForBody
{
public:
	virtual ~btIParallelForBody() {}
	virtual void forLoop(int iBegin, int iEnd) const = 0;
};





class btIParallelSumBody
{
public:
	virtual ~btIParallelSumBody() {}
	virtual btScalar sumLoop(int iBegin, int iEnd) const = 0;
};





class btITaskScheduler
{
public:
	btITaskScheduler(const char* name);
	virtual ~btITaskScheduler() {}
	const char* getName() const { return m_name; }

	virtual int getMaxNumThreads() const = 0;
	virtual int getNumThreads() const = 0;
	virtual void setNumThreads(int numThreads) = 0;
	virtual void parallelFor(int iBegin, int iEnd, int grainSize, const btIParallelForBody& body) = 0;
	virtual btScalar parallelSum(int iBegin, int iEnd, int grainSize, const btIParallelSumBody& body) = 0;
	virtual void sleepWorkerThreadsHint() {}  

	
	virtual void activate();
	virtual void deactivate();

protected:
	const char* m_name;
	unsigned int m_savedThreadCounter;
	bool m_isActive;
};



void btSetTaskScheduler(btITaskScheduler* ts);


btITaskScheduler* btGetTaskScheduler();


btITaskScheduler* btGetSequentialTaskScheduler();


btITaskScheduler* btCreateDefaultTaskScheduler();


btITaskScheduler* btGetOpenMPTaskScheduler();


btITaskScheduler* btGetTBBTaskScheduler();


btITaskScheduler* btGetPPLTaskScheduler();



void btParallelFor(int iBegin, int iEnd, int grainSize, const btIParallelForBody& body);



btScalar btParallelSum(int iBegin, int iEnd, int grainSize, const btIParallelSumBody& body);

#endif
