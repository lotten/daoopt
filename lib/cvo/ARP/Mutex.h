#ifndef BEEM_MUTEX_H_INCLUDED
#define BEEM_MUTEX_H_INCLUDED

#include <string>

#ifdef WINDOWS

#ifndef WINDOWS_H_INCLUDED
#include <windows.h>
#define WINDOWS_H_INCLUDED
#endif

#elif defined (LINUX)

#include <pthread.h>

#else
#error "Please provide the Mutex implementation for this platform and edit <Mutex.h>"
#endif // WINDOWS

namespace ARE {
namespace utils {

////////////////////////////////////////////////////////////////////
// This class wraps what POSIX calls "recursive mutex" and what
// Win32 calls "critical section".
////////////////////////////////////////////////////////////////////


class RecursiveMutex
{
#ifdef USE_MUTEX_WITH_NAME
protected :
	std::string _Name ;
#endif // USE_MUTEX_WITH_NAME

public:
	RecursiveMutex
		(
#ifdef USE_MUTEX_WITH_NAME
const std::string *pName = NULL
#else
		void
#endif // USE_MUTEX_WITH_NAME
		) throw() ;
	~RecursiveMutex(void) throw() ;

	void setName(const std::string *pName) throw() ;
	void lock(void) throw() ;
	bool trylock(void) throw() ;
	void unlock(void) throw() ;

private :

	RecursiveMutex & operator=(const RecursiveMutex&) ;
	RecursiveMutex(const RecursiveMutex &) ;

#ifdef WIN32
	CRITICAL_SECTION _CS ;
public:
	inline int GetCSRecursionCount(void) const { return _CS.RecursionCount ; }
	inline int GetCSLockCount(void) const { return _CS.LockCount ; }
#elif defined (LINUX)
private :
	pthread_mutex_t _RM ;
#endif
} ;

// The RecursiveMutex wrapper which will lock the mutex when constructed
// and unlock it when destructed
class AutoLock
{
public :
	inline AutoLock(RecursiveMutex & mutex) throw() : _M(mutex) { _M.lock() ; }
	inline ~AutoLock(void) throw() { _M.unlock() ; }

private :
	AutoLock(const AutoLock &) ;
	AutoLock & operator=(const AutoLock &) ;
	RecursiveMutex & _M ;
} ;

}}

#endif // BEEM_MUTEX_H_INCLUDED
