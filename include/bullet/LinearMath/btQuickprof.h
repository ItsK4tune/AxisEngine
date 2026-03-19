





#ifndef BT_QUICK_PROF_H
#define BT_QUICK_PROF_H

#include "btScalar.h"
#define USE_BT_CLOCK 1

#ifdef USE_BT_CLOCK


class btClock
{
public:
	btClock();

	btClock(const btClock& other);
	btClock& operator=(const btClock& other);

	~btClock();

	
	void reset();

	
	
	unsigned long long int getTimeMilliseconds();

	
	
	unsigned long long int getTimeMicroseconds();

	unsigned long long int getTimeNanoseconds();

	
	
	btScalar getTimeSeconds();

private:
	struct btClockData* m_data;
};

#endif  

typedef void(btEnterProfileZoneFunc)(const char* msg);
typedef void(btLeaveProfileZoneFunc)();

btEnterProfileZoneFunc* btGetCurrentEnterProfileZoneFunc();
btLeaveProfileZoneFunc* btGetCurrentLeaveProfileZoneFunc();

void btSetCustomEnterProfileZoneFunc(btEnterProfileZoneFunc* enterFunc);
void btSetCustomLeaveProfileZoneFunc(btLeaveProfileZoneFunc* leaveFunc);

#ifndef BT_ENABLE_PROFILE
#define BT_NO_PROFILE 1
#endif  

const unsigned int BT_QUICKPROF_MAX_THREAD_COUNT = 64;



unsigned int btQuickprofGetCurrentThreadIndex2();

#ifndef BT_NO_PROFILE


#include <stdio.h>  

#include "btAlignedAllocator.h"
#include <new>


class CProfileNode
{
public:
	CProfileNode(const char* name, CProfileNode* parent);
	~CProfileNode(void);

	CProfileNode* Get_Sub_Node(const char* name);

	CProfileNode* Get_Parent(void) { return Parent; }
	CProfileNode* Get_Sibling(void) { return Sibling; }
	CProfileNode* Get_Child(void) { return Child; }

	void CleanupMemory();
	void Reset(void);
	void Call(void);
	bool Return(void);

	const char* Get_Name(void) { return Name; }
	int Get_Total_Calls(void) { return TotalCalls; }
	float Get_Total_Time(void) { return TotalTime; }
	void* GetUserPointer() const { return m_userPtr; }
	void SetUserPointer(void* ptr) { m_userPtr = ptr; }

protected:
	const char* Name;
	int TotalCalls;
	float TotalTime;
	unsigned long int StartTime;
	int RecursionCounter;

	CProfileNode* Parent;
	CProfileNode* Child;
	CProfileNode* Sibling;
	void* m_userPtr;
};


class CProfileIterator
{
public:
	
	void First(void);
	void Next(void);
	bool Is_Done(void);
	bool Is_Root(void) { return (CurrentParent->Get_Parent() == 0); }

	void Enter_Child(int index);     
	void Enter_Largest_Child(void);  
	void Enter_Parent(void);         

	
	const char* Get_Current_Name(void) { return CurrentChild->Get_Name(); }
	int Get_Current_Total_Calls(void) { return CurrentChild->Get_Total_Calls(); }
	float Get_Current_Total_Time(void) { return CurrentChild->Get_Total_Time(); }

	void* Get_Current_UserPointer(void) { return CurrentChild->GetUserPointer(); }
	void Set_Current_UserPointer(void* ptr) { CurrentChild->SetUserPointer(ptr); }
	
	const char* Get_Current_Parent_Name(void) { return CurrentParent->Get_Name(); }
	int Get_Current_Parent_Total_Calls(void) { return CurrentParent->Get_Total_Calls(); }
	float Get_Current_Parent_Total_Time(void) { return CurrentParent->Get_Total_Time(); }

protected:
	CProfileNode* CurrentParent;
	CProfileNode* CurrentChild;

	CProfileIterator(CProfileNode* start);
	friend class CProfileManager;
};


class CProfileManager
{
public:
	static void Start_Profile(const char* name);
	static void Stop_Profile(void);

	static void CleanupMemory(void);
	
	
	

	static void Reset(void);
	static void Increment_Frame_Counter(void);
	static int Get_Frame_Count_Since_Reset(void) { return FrameCounter; }
	static float Get_Time_Since_Reset(void);

	static CProfileIterator* Get_Iterator(void);
	
	
	
	
	static void Release_Iterator(CProfileIterator* iterator) { delete (iterator); }

	static void dumpRecursive(CProfileIterator* profileIterator, int spacing);

	static void dumpAll();

private:
	static int FrameCounter;
	static unsigned long int ResetTime;
};

#endif  



class CProfileSample
{
public:
	CProfileSample(const char* name);

	~CProfileSample(void);
};

#define BT_PROFILE(name) CProfileSample __profile(name)

#endif  
