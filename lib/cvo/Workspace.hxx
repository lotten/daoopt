#ifndef ARE_workspace_HXX_INCLUDED
#define ARE_workspace_HXX_INCLUDED

#include <stdlib.h>

#ifdef WINDOWS
#include <windows.h>
#endif // WINDOWS

#include "Explanation.hxx"
#include "Function.hxx"

namespace ARE { class ARP ; }
namespace ARE { class Function ; }
namespace ARE { class FunctionTableBlock ; }

namespace ARE
{

class Workspace
{
protected :
	bool _IsValid ; // indicates whether workspace was constructed correctly
public :
	inline bool IsValid(void) const { return _IsValid ; }

protected :
	ARE::ARP *_Problem ;
public :
	inline ARE::ARP *Problem(void) const { return _Problem ; }

	// **************************************************************************************************
	// error codes occuring during computation
	// **************************************************************************************************

protected :
	bool _HasFatalError ;
	ARE::Explanation *_ExplanationList ;
public :
	inline bool HasFatalError(void) const { return _HasFatalError ; }
	inline void SetFatalError(void) { _HasFatalError = true ; }
	void AddErrorExplanation(ARE::Function *f, ARE::FunctionTableBlock *ftb) ;
	void AddExplanation(ARE::Explanation & E) ;
	bool HasErrorExplanation(void) ;

	// **************************************************************************************************
	// External Memory BE
	// **************************************************************************************************

protected :
	// This mutex is used to synchronize access to Function-Table-Block related data within this workspace.
	// In particular, it is used :
	// 1) protect access to _CurrentDiskMemorySpaceCached/_MaximumDiskMemorySpaceCached variables of this workspace.
	// 2) _FTBsInMemory ptr (list) of functions in this workspace.
	// 3) indirectly, the users list of each FTB.
	// 4) maintain/update the _BucketFunctionBlockComputationResult arrays in each bucket
public :

protected :
	std::string _DiskSpaceDirectory ;
public :
	inline const std::string & DiskSpaceDirectory(void) const { return _DiskSpaceDirectory ; }

protected :
	__int64 _nInputTableBlocksWaited ;
	__int64 _InputTableBlocksWaitPeriodTotal ;
	__int64 _InputTableGetTimeTotal ; // time of Functio::GetFTB(). in milliseconds.
	__int64 _FileLoadTimeTotal ; // in milliseconds.
	__int64 _FileSaveTimeTotal ; // in milliseconds.
	__int64 _FTBComputationTimeTotal ; // in milliseconds.
	__int64 _nTableBlocksLoaded ;
	__int64 _nTableBlocksSaved ;
	__int64 _CurrentDiskMemorySpaceCached ;
	__int64 _MaximumDiskMemorySpaceCached ;
	int _nDiskTableBlocksInMemory ;
	int _MaximumNumConcurrentDiskTableBlocksInMemory ;
	int _nFTBsLoadedPerBucket[MAX_NUM_BUCKETS] ;
	//
	int _InputFTBWait_BucketIDX[128] ;
	int _InputFTBWait_BlockIDX[128] ;
	//
public :
	void ResetStatistics(void)
	{ 
		_nInputTableBlocksWaited = _InputTableBlocksWaitPeriodTotal = _nTableBlocksLoaded = _nTableBlocksSaved = 0 ; 
		_InputTableGetTimeTotal = _FileLoadTimeTotal = _FileSaveTimeTotal = _FTBComputationTimeTotal = 0 ;
		_CurrentDiskMemorySpaceCached = _MaximumDiskMemorySpaceCached = 0 ;
		_nDiskTableBlocksInMemory = _MaximumNumConcurrentDiskTableBlocksInMemory = 0 ;
		for (int i = 0 ; i < MAX_NUM_BUCKETS ; i++) 
			_nFTBsLoadedPerBucket[i] = 0 ;
	}
	inline __int64 nInputTableBlocksWaited(void) const { return _nInputTableBlocksWaited ; }
	inline void InputTableBlockWaitDetails(int i, int & BucketIDX, __int64 & BlockIDX) const { BucketIDX = _InputFTBWait_BucketIDX[i] ; BlockIDX = _InputFTBWait_BlockIDX[i] ; }
	inline __int64 InputTableBlocksWaitPeriodTotal(void) const { return _InputTableBlocksWaitPeriodTotal ; }
	inline __int64 InputTableGetTimeTotal(void) const { return _InputTableGetTimeTotal ; }
	inline __int64 FileLoadTimeTotal(void) const { return _FileLoadTimeTotal ; }
	inline __int64 FileSaveTimeTotal(void) const { return _FileSaveTimeTotal ; }
	inline __int64 FTBComputationTimeTotal(void) const { return _FTBComputationTimeTotal ; }
	inline __int64 nTableBlocksLoaded(void) const { return _nTableBlocksLoaded ; }
	inline __int64 nTableBlocksSaved(void) const { return _nTableBlocksSaved; }
	inline int nFTBsLoadedPerBucket(int IDX) const { return _nFTBsLoadedPerBucket[IDX] ; }
	inline void NoteInputTableGetTime(DWORD t)
	{
		_InputTableGetTimeTotal += t ;
	}
	inline void NoteFileLoadTime(DWORD t)
	{
		_FileLoadTimeTotal += t ;
	}
	inline void NoteFileSaveTime(DWORD t)
	{
		_FileSaveTimeTotal += t ;
	}
	inline void NoteFTBComputationTime(DWORD t)
	{
		_FTBComputationTimeTotal += t ;
	}
	inline void NoteInputTableBlocksWait(int BucketIDX, __int64 BlockIDX, bool Increment, long WaitInMilliseconds)
	{
		_InputTableBlocksWaitPeriodTotal += WaitInMilliseconds ;
		if (Increment) {
			++_nInputTableBlocksWaited ;
			}
	}
	inline void IncrementnTableBlocksLoaded(int IDX)
	{
		++_nTableBlocksLoaded ;
		if (IDX >= 0) 
			_nFTBsLoadedPerBucket[IDX]++ ;
	}
	inline void IncrementnTableBlocksSaved(void)
	{
		++_nTableBlocksSaved ;
	}
	inline __int64 CurrentDiskMemorySpaceCached(void) const { return _CurrentDiskMemorySpaceCached ; }
	inline __int64 MaximumDiskMemorySpaceCached(void) const { return _MaximumDiskMemorySpaceCached ; }
	inline int nDiskTableBlocksInMemory(void) const { return _nDiskTableBlocksInMemory ; }
	inline int MaximumNumConcurrentDiskTableBlocksInMemory(void) const { return _MaximumNumConcurrentDiskTableBlocksInMemory ; }
	void NoteDiskMemoryBlockLoaded(__int64 Space) 
	{ 
		++_nDiskTableBlocksInMemory ;
		if (_nDiskTableBlocksInMemory > _MaximumNumConcurrentDiskTableBlocksInMemory) 
			_MaximumNumConcurrentDiskTableBlocksInMemory = _nDiskTableBlocksInMemory ;
		_CurrentDiskMemorySpaceCached += Space ;
		if (_CurrentDiskMemorySpaceCached > _MaximumDiskMemorySpaceCached)
			_MaximumDiskMemorySpaceCached = _CurrentDiskMemorySpaceCached ;
	}
	void NoteDiskMemoryBlockUnLoaded(__int64 Space) 
	{
		--_nDiskTableBlocksInMemory ;
		_CurrentDiskMemorySpaceCached -= Space ;
	}
	void LogStatistics(time_t ttStart, time_t ttFinish) ;

public :

	virtual int Initialize(ARE::ARP & Problem)
		{ if (NULL != _Problem) return 1 ; _Problem = &Problem ; return 0 ; }
	Workspace(const char *BEEMDiskSpaceDirectory)
		:
		_IsValid(true), 
		_Problem(NULL), 
		_HasFatalError(false), 
		_ExplanationList(NULL), 
		_nInputTableBlocksWaited(0), 
		_InputTableBlocksWaitPeriodTotal(0), 
		_InputTableGetTimeTotal(0), 
		_FileLoadTimeTotal(0), 
		_FileSaveTimeTotal(0), 
		_FTBComputationTimeTotal(0), 
		_nTableBlocksLoaded(0), 
		_nTableBlocksSaved(0), 
		_CurrentDiskMemorySpaceCached(0), 
		_MaximumDiskMemorySpaceCached(0), 
		_nDiskTableBlocksInMemory(0), 
		_MaximumNumConcurrentDiskTableBlocksInMemory(0)
	{
		if (NULL != BEEMDiskSpaceDirectory) 
			_DiskSpaceDirectory = BEEMDiskSpaceDirectory ;
	}
	virtual ~Workspace(void)
	{
		while (NULL != _ExplanationList) {
			ARE::Explanation *e = _ExplanationList ;
			_ExplanationList = _ExplanationList->_Next ;
			delete e ;
			}
	}
} ;

} // namespace ARE

#endif // ARE_workspace_HXX_INCLUDED
