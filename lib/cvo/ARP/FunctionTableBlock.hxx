#ifndef FunctionTableBlock_HXX_INCLUDED
#define FunctionTableBlock_HXX_INCLUDED

#include <stdlib.h>
#include <string>

namespace BucketElimination { class Bucket ; }
namespace ARE { class Function ; }

namespace ARE
{

class Function ;

#define ARE_Function_TableType double
#define ARE_Function_TableTypeString "double"

class FunctionTableBlock
{
	// *********************************************************************************************
	// FunctionTableBlock is defined by ReferenceFunction and IDX.
	// *********************************************************************************************

protected :
	// function this table block refers to; if IDX=0, then this table block belongs to the function.
	// if IDX=[1,F->_nTableBlocks], then table block does not belong to the function; most likely it is computed by BEEM workspace and store on the disk.
	Function *_ReferenceFunction ;
	// index of the block; values run [0,F->_nTableBlocks].
	__int64 _IDX ;
public :
	inline __int64 IDX(void) { return _IDX ; }
	inline Function *ReferenceFunction(void) const { return _ReferenceFunction ; }
	inline void AttachReferenceFunction(Function *RF) { _ReferenceFunction = RF ; }

protected :
	std::string _Filename ;
public :
	inline const std::string & Filename(void) const { return _Filename ; }
	inline void EraseFilename(void) { _Filename.erase() ; }
	int ComputeFilename(void) ;

protected :
	// Start/End Adr can be -1 even if size is >0, since size if the memory block (_Data).
	__int64 _StartAdr ;
	__int64 _EndAdr ; // _EndAdr = _StartAdr + _Size; this is really the index of the next after the last element.
public :
	inline __int64 StartAdr(void) { return _StartAdr ; }
	inline __int64 EndAdr(void) { return _EndAdr ; }
	inline __int64 Size(void) { return _Size ; }

protected :
	__int64 _Size ; // this is the size of the memory block allocated (as number of elements); should be set iff _Data is allocated.
	ARE_Function_TableType *_Data ;
public :
	inline ARE_Function_TableType Entry(__int64 IDX) const { return _Data[IDX] ; }
	inline ARE_Function_TableType *Data(void) { return _Data ; }
	int AllocateData(void) ;
	inline int SetData(__int64 Size, ARE_Function_TableType *Data)
	{
		DestroyData() ;
		_Size = Size ;
		_Data = Data ;
		return 0 ;
	}
	int SaveData(void) ;
	int LoadData(void) ;
	int CheckData(void) ;

	// sum up entire FTB
	double SumEntireData(void) ;

	// this function will check if all tables needed for the computation of this table block exist (in-memory or on-disk).
	// return values : 
	//  0 : yes
	// +1 : some table block does not exist; in this case Bucket/BlockIDX specify which block does not exist
	// -1 : error
	int CheckBEEMInputTablesExist(Function * & MissingFunction, __int64 & MissingBlockIDX) ;

	// compute this table block. this function does not add the table block to its reference function block list.
	// return values :
	//  0 : ok
	// +1 : generic error
	// +2 : failed because a input function table block has not yet been computed; 
	//      in this case, the output argument contain fn/blockIDX of what is missing.
	// +3 : failed because global num restriction was violated (e.g. too many variables/functions in the bucket)
	int ComputeDataBEEM_ProdSum(Function * & MissingFunction, __int64 & MissingBlockIDX) ;
	int ComputeDataBEEM_ProdMax(Function * & MissingFunction, __int64 & MissingBlockIDX) ;
	int ComputeDataBEEM_SumMax(Function * & MissingFunction, __int64 & MissingBlockIDX) ;
	int ComputeDataBEEM_ProdSum_singleV(Function * & MissingFunction, __int64 & MissingBlockIDX) ;
	int ComputeDataBEEM_ProdMax_singleV(Function * & MissingFunction, __int64 & MissingBlockIDX) ;
	int ComputeDataBEEM_SumMax_singleV(Function * & MissingFunction, __int64 & MissingBlockIDX) ;

	// *******************************************************************************************************
	// other function blocks that use this function block; 
	// this reference counting is used to determine when to delete the block.
	// these values here are used only by this-Function::GetFTB()/this-Function::ReleaseFTB().
	// the function will use a mutex to make this access thread-safe.
	// e.g. it will use the _FTBMutex of the workspace.
	// *******************************************************************************************************

protected :
	int _nUsers ;
	int _UsersAllocatedSize ;
	FunctionTableBlock **_Users ;
public :
	inline int nUsers(void) const { return _nUsers ; }
	inline bool IsUser(FunctionTableBlock & FTB) 
	{
		for (int i = _nUsers-1 ; i >= 0 ; i--) {
			if (&FTB == _Users[i]) 
				return true ;
			}
		return false ;
	}
	inline int AddUser(FunctionTableBlock & FTB)
	{
		if (IsUser(FTB)) 
			return 0 ;
		if (_UsersAllocatedSize <= _nUsers) {
			int newsize = _UsersAllocatedSize + 4 ;
			FunctionTableBlock **newarray = new FunctionTableBlock*[newsize] ;
			if (NULL == newarray) 
				return 1 ;
			memcpy(newarray, _Users, _nUsers*sizeof(FunctionTableBlock *)) ;
			delete [] _Users ;
			_Users = newarray ;
			_UsersAllocatedSize = newsize ;
			}
		_Users[_nUsers++] = &FTB ;
		return 0 ;
	}
	inline void RemoveUser(FunctionTableBlock & FTB)
	{
		for (int i = _nUsers-1 ; i >= 0 ; i--) {
			if (&FTB == _Users[i]) 
				_Users[i] = _Users[--_nUsers] ;
			}
		// caller should check if there are any users left and delete the block in no users left.
	}

	// *******************************************************************************************************
	// other function blocks that this function block is using; this also means that the other function blocks have this block as a user.
	// when this function block is unloaded, it will tell input blocks that this block is no longer a user.
	// if nobody is using the input block, it also can be unloaded.
	// *******************************************************************************************************

/*
protected :
	int _nInputs ;
	int _InputsAllocatedSize ;
	FunctionTableBlock **_Inputs ;
public :
	inline int nInputs(void) const { return _nInputs ; }
	inline int AddInput(FunctionTableBlock & FTB)
	{
		if (_InputsAllocatedSize <= _nInputs) {
			int newsize = _InputsAllocatedSize + 4 ;
			FunctionTableBlock **newarray = new FunctionTableBlock*[newsize] ;
			if (NULL == newarray) 
				return 1 ;
			memcpy(newarray, _Inputs, _nInputs*sizeof(FunctionTableBlock *)) ;
			delete [] _Inputs ;
			_Inputs = newarray ;
			_InputsAllocatedSize = newsize ;
			}
		_Inputs[_nInputs++] = &FTB ;
		return 0 ;
	}
	inline void RemoveInput(FunctionTableBlock & FTB)
	{
		for (int i = _nInputs-1 ; i >= 0 ; i--) {
			if (&FTB == _Inputs[i]) 
				_Inputs[i] = _Inputs[--_nInputs] ;
			}
	}
*/

	// *******************************************************************************************************
	// A function keeps track of its FTBs that are in memory.
	// When a parent function needs a FTB from an input function, it asks it to load it.
	// The input function keeps track of FTBs that are loaded; when no longer needed, it can delete those FTBs that are not used.
	// This ptr is used to create a linked list of function table blocks, associated with the function, that are in memory.
	// Its values is set/reset by Function::AddFTB()/Function::RemoveFTB().
	// The function will take care of the mutex to make it thread-safe.
	// *******************************************************************************************************

protected :
	FunctionTableBlock *_NextFTBInFunction ;
public :
	inline FunctionTableBlock *NextFTBInFunction(void) const { return _NextFTBInFunction ; }
	inline void AttachNextFTBInFunction(FunctionTableBlock *FTB) { _NextFTBInFunction = FTB ; }

/*
	// *******************************************************************************************************
	// Function blocks may be organized as a set of tasks, each being "compute (fill in) a table block".
	// BEEM workspace maintains a list of FTBs to be computed, for worker threads to handle.
	// This ptr is used to create a list to tasks, e.g. when running BEEM algorithm, to create a list of table blocks to be computed.
	// *******************************************************************************************************

protected :
	FunctionTableBlock *_NextInTaskQueue ;
public :
	inline FunctionTableBlock *NextInTaskQueue(void) const { return _NextInTaskQueue ; }
	inline void AttachNextInTaskQueue(FunctionTableBlock *FTB) { _NextInTaskQueue = FTB ; }
*/

public :
	int Initialize(Function *ReferenceFunction, __int64 IDX) ;
	void DestroyData(void)
	{
		if (NULL != _Data) {
			delete [] _Data ;
			_Data = NULL ;
			}
		_Size = 0 ;
	}
	inline FunctionTableBlock(Function *ReferenceFunction = NULL)
		:
		_ReferenceFunction(ReferenceFunction), 
		_IDX(-1), 
		_StartAdr(-1), 
		_EndAdr(-1), 
		_Size(0), 
		_Data(NULL), 
		_nUsers(0), 
		_UsersAllocatedSize(0), 
		_Users(NULL), 
//		_nInputs(0), 
//		_InputsAllocatedSize(0), 
//		_Inputs(NULL), 
		_NextFTBInFunction(NULL) 
//		_NextInTaskQueue(NULL)
	{
	}
	virtual ~FunctionTableBlock(void) ;
} ;

} // namespace ARE

#endif // FunctionTableBlock_HXX_INCLUDED
