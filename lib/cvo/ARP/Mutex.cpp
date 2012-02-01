#if false
#include <stdlib.h>

#ifdef WINDOWS
#include <windows.h>
#else 
#include <pthread.h>
#include <sched.h>
#endif // WINDOWS

#include <iostream>

#include "Mutex.h"

namespace ARE {
namespace utils {

RecursiveMutex::RecursiveMutex(
#ifdef USE_MUTEX_WITH_NAME
	const std::string *pName
#else
	void
#endif // USE_MUTEX_WITH_NAME
	) throw()
{
#ifdef USE_MUTEX_WITH_NAME
	setName(pName) ;
#endif // USE_MUTEX_WITH_NAME

#ifdef WINDOWS
	InitializeCriticalSection(&_CS) ;
#elif defined LINUX
	pthread_mutexattr_t mutexAttributes ;
	pthread_mutexattr_init(&mutexAttributes) ;
	pthread_mutexattr_settype(&mutexAttributes, PTHREAD_MUTEX_RECURSIVE_NP) ;
	pthread_mutex_init(&_RM, &mutexAttributes) ;
	pthread_mutexattr_destroy(&mutexAttributes) ;
#endif
}


RecursiveMutex::~RecursiveMutex(void) throw()
{
#ifdef WINDOWS
	DeleteCriticalSection(&_CS) ;
#elif defined LINUX
	pthread_mutex_destroy(&_RM) ;
#endif
}


void RecursiveMutex::setName(const std::string *pName) throw()
{
#ifdef USE_MUTEX_WITH_NAME
	if (pName)
		_Name = *pName ;
	else
		_Name = "" ;
#endif // USE_MUTEX_WITH_NAME
}


void RecursiveMutex::lock(void) throw()
{
#ifdef WINDOWS
//#ifdef USE_MUTEX_WITH_NAME
//	if (_Name.length() > 0)
		//g_log << maui::util::logMessage(10, "--------MUTEX: [" + _Name + "] :LOCK BEGIN\n", true);
//#endif // USE_MUTEX_WITH_NAME
//	else
//		g_log << maui::util::logMessage(10, "--------MUTEX: [] :LOCK BEGIN", true);
//std::cout << "--------MUTEX: [" << _Name << "] :LOCK BEGIN" << std::endl;
	EnterCriticalSection(&_CS) ;
//std::cout << "--------MUTEX: [" << _Name << "] :LOCK GOT" << std::endl;
//#ifdef USE_MUTEX_WITH_NAME
//	if (_Name.length() > 0)
		//g_log << maui::util::logMessage(10, "--------MUTEX: [" + _Name + "] :LOCK GOT\n", true);
//#endif // USE_MUTEX_WITH_NAME
//	else
//		g_log << maui::util::logMessage(10, "--------MUTEX: [] :LOCK GOT", true);
#elif defined LINUX
	pthread_mutex_lock(&_RM) ;
//std::cout << "--------MUTEX: " << this << ": LOCKED" << std::endl;
#endif
}


bool RecursiveMutex::trylock(void) throw()
{
#ifdef WINDOWS
	// does not compile in VC++6.0
	// return TryEnterCriticalSection(&_CS);
#elif defined LINUX
	// TODO : implement for linux
#endif
	return false ;
}


void RecursiveMutex::unlock(void) throw()
{
#ifdef WINDOWS
//#ifdef USE_MUTEX_WITH_NAME
//	if (_Name.length() > 0)
		//g_log << maui::util::logMessage(10, "--------MUTEX: [" + _Name + "] :unLOCK BEGIN\n", true);
//#endif // USE_MUTEX_WITH_NAME
//	else
//		g_log << maui::util::logMessage(10, "--------MUTEX: [] :unLOCK BEGIN", true);
//std::cout << "--------MUTEX: [" << _Name << "] :unLOCK BEGIN" << std::endl;
	LeaveCriticalSection(&_CS) ;
//#ifdef USE_MUTEX_WITH_NAME
//	if (_Name.length() > 0)
		//g_log << maui::util::logMessage(10, "--------MUTEX: [" + _Name + "] :unLOCK GOT\n", true);
//#endif // USE_MUTEX_WITH_NAME
//	else
//		g_log << maui::util::logMessage(10, "--------MUTEX: [] :unLOCK GOT", true);
//std::cout << "--------MUTEX: [" << _Name << "] :unLOCK GOT" << std::endl;
#elif defined LINUX
	pthread_mutex_unlock(&_RM) ;
//std::cout << "--------MUTEX: " << this << ": UNLOCKED" << std::endl;
#endif
}

	}
}
#endif  // false
