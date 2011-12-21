/*
	This file contains HEAP definitions.

	Kalev Kask, May 2002.
*/

#ifndef HEAP_HXX_INCLUDED
#define HEAP_HXX_INCLUDED

#define HEAP_DEFAULT_SIZE	128
#define HEAP_SIZE_LIMIT		1048576

class CMauiHeap
{

protected :

	// number of elements in the heap; actual key/data array sizes are m_size+1 since user keys/data start from [1]
	unsigned long m_size ;

	// allocated space; as the max number of elements that can be accomodated.
	unsigned long m_allocated_space ;

	// When reallocating the heap, DBM will reserve that much more memory.
	// If this variable is 0, DBM will never reallocate.
	// That means that when the heap is full and the user calls Insert() on the heap, it will fail.
	// Initially equal to DBM_HEAP_DEFAULT_SIZE.
	unsigned long m_re_allocation_size ;

	// keys by which the heap is sorted; keys start from [1]
	long *m_key ;

	// information associated with every key
	long *m_data ;

	// if TRUE, largest comes first; otherwise smallest comes first.
	// default is TRUE.
	bool m_bSortOrderIsDecreasing ;

private :

	// when the heap is full, then DBM tries to reallocate the entire heap
	int ReAllocateMemory(void) ;

	// assume a new key was placed at index 'Idx'; assume the size of the heap is correct and anything 
	// above m_data[Idx] is correct. Fix the data below m_data[Idx].
	int UpdateHeap_AfterRemove(unsigned long Idx) ;

public :

	// Default constructor creates an empty heap of size DBM_HEAP_DEFAULT_SIZE.
	CMauiHeap(void) ;

	// This constructor creates a heap from a set of keys.
	CMauiHeap(unsigned long size, /* number of keys, has to be >= 0 */ 
		long *key, /* an array of keys */ 
		long *data, /* data associated with each key */ 
		unsigned long space, /* how much space to allocate for the heap */
		unsigned long re_allocation_size = HEAP_DEFAULT_SIZE) ;

	int Import(unsigned long size, /* number of keys, has to be >= 0 */ 
		long *key, /* an array of keys */ 
		long *data /* data associated with each key */) ;

	~CMauiHeap(void) ;

	int AllocateMemory(long size) ;

	void SetReAllocationSize(long reallocation_size) ; // has to be non-negative. default DBM_HEAP_DEFAULT_SIZE.

	long CheckHeap(void) ; // returns TRUE iff the heap is OK.

	inline void SetSortOrder(bool NewOrder) { m_bSortOrderIsDecreasing = NewOrder ; }

	inline void Empty(void) { m_size = 0 ; } // this will empty the heap.

	inline long GetSize(void) const { return m_size ; }

	inline long GetSpace(void) const { return m_allocated_space ; }

	long Insert(long key, long data) ; // returns FALSE iff it fails (no memory)

	// remove the largest key/data; returns FALSE if the heap is empty
	long Remove(long *key /* return key */, long *data /* return data, can be NULL */) ;
	// remove a specific key/data; returns FALSE if the key is not in the heap
	long Remove_key(long key /* key to remove */, long *data /* return data, can be NULL */) ;
	long Remove_data(long *key /* return key */, long data /* data to remove */) ;
	// remove element at specific idx
	long Remove_idx(long idx, long *key, long *data) ;

	int ContainsKey(long key) ;
	int ContainsData(long data) ;

	// This function returns the first (topmost) key and the data associated with it,
	// but does not remove it from the heap.
	long GetFirst(long *key /* return key */, long *data /* return data, can be NULL */) ; // returns FALSE if the heap is empty
	inline void GetIdx(int idx, long & key, long & data) { key = m_key[idx] ; data = m_data[idx] ; }
} ;

class CMauiHeap_Basic
{

protected :

	// number of elements in the heap; actual key/data array sizes are m_size+1 since user keys/data start from [1]
	unsigned long m_size ;

	// allocated space; as the max number of elements that can be accomodated.
	unsigned long m_allocated_space ;

	// When reallocating the heap, DBM will reserve that much more memory.
	// If this variable is 0, DBM will never reallocate.
	// That means that when the heap is full and the user calls Insert() on the heap, it will fail.
	// Initially equal to DBM_HEAP_DEFAULT_SIZE.
	unsigned long m_re_allocation_size ;

	// keys by which the heap is sorted; keys start from [1]
	long *m_key ;

	// if TRUE, largest comes first; otherwise smallest comes first.
	// default is TRUE.
	bool m_bSortOrderIsDecreasing ;

private :

	// when the heap is full, then DBM tries to reallocate the entire heap
	int ReAllocateMemory(void) ;

	// assume a new key was placed at index 'Idx'; assume the size of the heap is correct and anything 
	// above m_data[Idx] is correct. Fix the data below m_data[Idx].
	int UpdateHeap_AfterRemove(unsigned long Idx) ;

public :

	// Default constructor creates an empty heap of size DBM_HEAP_DEFAULT_SIZE.
	CMauiHeap_Basic(void) ;

	// This constructor creates a heap from a set of keys.
	CMauiHeap_Basic(unsigned long size, /* number of keys, has to be >= 0 */ 
		long *key, /* an array of keys */ 
		unsigned long space, /* how much space to allocate for the heap */
		unsigned long re_allocation_size = HEAP_DEFAULT_SIZE) ;

	int Import(unsigned long size, /* number of keys, has to be >= 0 */ 
		long *key/* an array of keys */) ;

	~CMauiHeap_Basic(void) ;

	int operator=(const CMauiHeap_Basic & Heap) ;

	int AllocateMemory(long size) ;

	void SetReAllocationSize(long reallocation_size) ; // has to be non-negative. default DBM_HEAP_DEFAULT_SIZE.

	long CheckHeap(void) ; // returns TRUE iff the heap is OK.

	inline void SetSortOrder(bool NewOrder) { m_bSortOrderIsDecreasing = NewOrder ; }

	inline void Empty(void) { m_size = 0 ; } // this will empty the heap.

	inline long GetSize(void) const { return m_size ; }

	inline long GetSpace(void) const { return m_allocated_space ; }

	long Insert(long key) ; // returns FALSE iff it fails (no memory)

	// remove the largest key/data; returns FALSE if the heap is empty
	long Remove(long *key /* return key */) ;
	// remove a specific key/data; returns FALSE if the key is not in the heap
	long Remove_key(long key /* key to remove */) ;
	// remove element at specific idx
	long Remove_idx(long idx, long *key) ;

	int ContainsKey(long key) ;

	// This function returns the first (topmost) key and the data associated with it,
	// but does not remove it from the heap.
	long GetFirst(long *key /* return key */) ; // returns FALSE if the heap is empty
	inline void GetIdx(int idx, long & key) { key = m_key[idx] ; }
} ;

/*
	Reduction -> after heap is constructed, its size will be reduced, one at a time until 0.
	PermutationMapping -> we keep track of how the original order of items changes over time.
*/

class CMauiHeap_Basic_ReductionWithPermutationMapping
{

protected :

	// number of elements in the heap; actual key/data array sizes are m_size+1 since user keys/data start from [1]
	unsigned long m_size ;

	// allocated space; as the max number of elements that can be accomodated.
	unsigned long m_allocated_space ;

	// When reallocating the heap, DBM will reserve that much more memory.
	// If this variable is 0, DBM will never reallocate.
	// That means that when the heap is full and the user calls Insert() on the heap, it will fail.
	// Initially equal to DBM_HEAP_DEFAULT_SIZE.
	unsigned long m_re_allocation_size ;

	// keys by which the heap is sorted; keys start from [1]
	long *m_key ;

	// mapping data :
	//		Original index of the corresponding element; e.g. m_CurrentIdx2OriginalIdxMap[5] = 3 means that element 5 was originally 3rd in the input list
	long *m_CurrentIdx2OriginalIdxMap ;
	//		Position of the corresponding element with the given original index; e.g. m_OriginalIdx2CurrentIdxMap[3] = 5 means that 3rd element in the original input list is at position 5
	long *m_OriginalIdx2CurrentIdxMap ;
	//		we have the property that m_OriginalIdx2CurrentIdxMap[m_CurrentIdx2OriginalIdxMap[i]] = i, where i is an index into m_key[]

	// if TRUE, largest comes first; otherwise smallest comes first.
	// default is TRUE.
	bool m_bSortOrderIsDecreasing ;

private :

	// when the heap is full, then DBM tries to reallocate the entire heap
	int ReAllocateMemory(void) ;

	// assume a new key was placed at index 'Idx'; assume the size of the heap is correct and anything 
	// above m_data[Idx] is correct. Fix the data below m_data[Idx].
	int UpdateHeap_AfterRemove(unsigned long Idx) ;

public :

	// Default constructor creates an empty heap of size DBM_HEAP_DEFAULT_SIZE.
	CMauiHeap_Basic_ReductionWithPermutationMapping(void) ;

	int Import(unsigned long size, /* number of keys, has to be >= 0 */ 
		long *key/* an array of keys */) ;

	~CMauiHeap_Basic_ReductionWithPermutationMapping(void) ;

	inline long OriginalIdx2CurrentIdxMap(int idx) const { return m_OriginalIdx2CurrentIdxMap[idx] ; }
	inline long CurrentIdx2OriginalIdxMap(int idx) const { return m_CurrentIdx2OriginalIdxMap[idx] ; }

	int operator=(const CMauiHeap_Basic_ReductionWithPermutationMapping & Heap) ;

	int AllocateMemory(long size) ;

	void SetReAllocationSize(long reallocation_size) ; // has to be non-negative. default DBM_HEAP_DEFAULT_SIZE.

	// update key at the given position
	int UpdateKey(unsigned long Idx, long new_key) ;

	long CheckHeap(void) ; // returns TRUE iff the heap is OK.

	inline void SetSortOrder(bool NewOrder) { m_bSortOrderIsDecreasing = NewOrder ; }

	inline void Empty(void) { m_size = 0 ; } // this will empty the heap.

	inline long GetSize(void) const { return m_size ; }

	inline long GetSpace(void) const { return m_allocated_space ; }

	long Insert(long key) ; // returns FALSE iff it fails (no memory)

	// remove the largest key/data; returns FALSE if the heap is empty
	long Remove(long *key /* return key */) ;
	// remove a specific key/data; returns FALSE if the key is not in the heap
	long Remove_key(long key /* key to remove */) ;
	// remove element at specific idx
	long Remove_idx(long idx, long *key) ;

	int ContainsKey(long key) ;

	// This function returns the first (topmost) key and the data associated with it,
	// but does not remove it from the heap.
	long GetFirst(long *key /* return key */) ; // returns FALSE if the heap is empty
	inline void GetIdx(int idx, long & key) const { key = m_key[idx] ; }
} ;

#endif // HEAP_HXX_INCLUDED
