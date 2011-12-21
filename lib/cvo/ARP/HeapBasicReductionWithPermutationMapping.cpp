/*
	Module HEAP.CPP

	This module contains the implementation of a HEAP.

	Kalev Kask, May 2002.
*/

/*
	Regular MS C++ include files.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>

#include "Heap.hxx"

/*
	Local definitions.
*/

/*
	Local variables.
*/


CMauiHeap_Basic_ReductionWithPermutationMapping::CMauiHeap_Basic_ReductionWithPermutationMapping(void)
	:
	m_size(0), 
	m_allocated_space(0), 
	m_re_allocation_size(0), 
	m_key(NULL), 
	m_CurrentIdx2OriginalIdxMap(NULL), 
	m_OriginalIdx2CurrentIdxMap(NULL), 
	m_bSortOrderIsDecreasing(true) 
{
}


CMauiHeap_Basic_ReductionWithPermutationMapping::~CMauiHeap_Basic_ReductionWithPermutationMapping(void)
{
	if (m_key) delete[] m_key ;
	if (m_CurrentIdx2OriginalIdxMap) delete[] m_CurrentIdx2OriginalIdxMap ;
	if (m_OriginalIdx2CurrentIdxMap) delete[] m_OriginalIdx2CurrentIdxMap ;
}


/*
	This is an internal function to reserve space for the heap.
*/

int CMauiHeap_Basic_ReductionWithPermutationMapping::AllocateMemory(long new_size)
{
/*
	check if there is a heap already. if yes, destroy it.
*/
	if (m_key) { delete[] m_key ; m_key = NULL ; }
	if (m_CurrentIdx2OriginalIdxMap) { delete[] m_CurrentIdx2OriginalIdxMap ; m_CurrentIdx2OriginalIdxMap = NULL ; }
	if (m_OriginalIdx2CurrentIdxMap) { delete[] m_OriginalIdx2CurrentIdxMap ; m_OriginalIdx2CurrentIdxMap = NULL ; }
	m_size = 0 ;
	m_allocated_space = -1 ;

/*
	Try to allocate memory for the new heap.
*/
	if (new_size > 0) {
		m_key = new long[new_size] ;
		m_CurrentIdx2OriginalIdxMap = new long[new_size] ;
		m_OriginalIdx2CurrentIdxMap = new long[new_size] ;
		if (NULL == m_key || NULL == m_CurrentIdx2OriginalIdxMap || NULL == m_OriginalIdx2CurrentIdxMap) {
			if (m_key) { delete[] m_key ; m_key = NULL ; }
			if (m_CurrentIdx2OriginalIdxMap) { delete[] m_CurrentIdx2OriginalIdxMap ; m_CurrentIdx2OriginalIdxMap = NULL ; }
			if (m_OriginalIdx2CurrentIdxMap) { delete[] m_OriginalIdx2CurrentIdxMap ; m_OriginalIdx2CurrentIdxMap = NULL ; }
			return 0 ;
			}
		m_allocated_space = new_size - 1 ;
		}

	return 1 ;
}


/*
	When the user tries to insert an element to the heap and the heap is already full,
	DBM tries to allocate more space for the heap. By default DBM allocates space
	for another CMauiHeap_DEFAULT_SIZE elements.

	This function allows the user to set the value of how much space DBM allocates when the heap is full.
*/

void CMauiHeap_Basic_ReductionWithPermutationMapping::SetReAllocationSize(long reallocation_size)
{
	if (reallocation_size >= 0 && reallocation_size <= HEAP_SIZE_LIMIT) 
		m_re_allocation_size = reallocation_size ;
}


/*
	This function will check if the heap satisfies the heap property.
	Returns TRUE iff yes, otherwise FALSE.
*/

long CMauiHeap_Basic_ReductionWithPermutationMapping::CheckHeap(void)
{
	unsigned long i, j ;

	for (i = 1 ; i <= m_size ; i++) {
		j = i << 1 ;
		if (j > m_size) return 1 ;
		if (m_key[j] > m_key[i]) {
			return 0 ;
			}
		++j ;
		if (j > m_size) continue ;
		if (m_key[j] > m_key[i]) {
			return 0 ;
			}
		}

	return 1 ;
}


/*
	When the heap is full and we Insert(), DBM tries to reallocate the heap.
*/

int CMauiHeap_Basic_ReductionWithPermutationMapping::ReAllocateMemory(void)
{
	long *old_key, *old_IdxOriginal, *old_PosOfElement ;
	signed long i, old_size, old_space, new_size ;

	if (m_re_allocation_size < 1) return 0 ;

	new_size = 1 + m_allocated_space + m_re_allocation_size ;

	old_key = m_key ;
	old_IdxOriginal = m_CurrentIdx2OriginalIdxMap ;
	old_PosOfElement = m_OriginalIdx2CurrentIdxMap ;
	old_size = m_size ;
	old_space = m_allocated_space ;
	m_key = NULL ;
	if (! AllocateMemory(new_size)) {
		m_key = old_key ;
		m_CurrentIdx2OriginalIdxMap = old_IdxOriginal ;
		m_OriginalIdx2CurrentIdxMap = old_PosOfElement ;
		m_size = old_size ;
		m_allocated_space = old_space ;
		return 0 ;
		}
	m_size = old_size ;

	m_key[0] = 0 ;
	m_CurrentIdx2OriginalIdxMap[0] = 0 ;
	m_OriginalIdx2CurrentIdxMap[0] = 0 ;
	for (i = 1 ; i <= old_size ; i++) {
		m_key[i] = old_key[i] ;
		m_CurrentIdx2OriginalIdxMap[i] = old_IdxOriginal[i] ;
		m_OriginalIdx2CurrentIdxMap[i] = old_PosOfElement[i] ;
		}	

	if (old_key) delete[] old_key ;

	return 1 ;
}


int CMauiHeap_Basic_ReductionWithPermutationMapping::Import(unsigned long size, /* number of keys, has to be >= 0 */ 
	long *input_key /* an array of keys */)
{
	if (size < 1) { m_size = 0 ; return 0 ; }
	if (size > m_allocated_space) return 1 ;

	unsigned long i, j, left, right ;
	long temp_key ;

	// first, copy stuff over to the heap arrays
	m_key[0] = 0 ;
	m_size = size ;
	for (j = 0, i = 1 ; j < m_size ; i++) {
		m_key[i] = input_key[j] ;
		m_CurrentIdx2OriginalIdxMap[i] = i ;
		m_OriginalIdx2CurrentIdxMap[i] = i ;
		j = i ;
		}

/*
	Sort the heap. Notice the starting point. It is guaranteed that heap elements after the starting
	point don't have to processed because they have no children.
*/
	for (j = (m_size >> 1) ; j > 0 ; j--) {
		i = j ;

heap_creation_propagate_down :
		left = i << 1 ;
		right = left + 1 ;
		if (left > m_size) continue ; // a leaf, no children
		if (m_bSortOrderIsDecreasing) {
			if (right > m_size) { // only left child
				if (m_key[left] > m_key[i]) { // swap
					temp_key = m_key[left] ; m_key[left] = m_key[i] ; m_key[i] = temp_key ;
					temp_key = m_CurrentIdx2OriginalIdxMap[left] ; m_CurrentIdx2OriginalIdxMap[left] = m_CurrentIdx2OriginalIdxMap[i] ; m_CurrentIdx2OriginalIdxMap[i] = temp_key ;
					m_OriginalIdx2CurrentIdxMap[m_CurrentIdx2OriginalIdxMap[left]] = left ;
					m_OriginalIdx2CurrentIdxMap[m_CurrentIdx2OriginalIdxMap[i]] = i ;
					}
				continue ;
				}

			// two children. the best key of three goes to the top.
			if (m_key[left] > m_key[right]) { // left is better than right
				if (m_key[left] > m_key[i]) { // left is also better than top. swap
					temp_key = m_key[left] ; m_key[left] = m_key[i] ; m_key[i] = temp_key ;
					temp_key = m_CurrentIdx2OriginalIdxMap[left] ; m_CurrentIdx2OriginalIdxMap[left] = m_CurrentIdx2OriginalIdxMap[i] ; m_CurrentIdx2OriginalIdxMap[i] = temp_key ;
					m_OriginalIdx2CurrentIdxMap[m_CurrentIdx2OriginalIdxMap[left]] = left ;
					m_OriginalIdx2CurrentIdxMap[m_CurrentIdx2OriginalIdxMap[i]] = i ;
					i = left ; // follow left side
					goto heap_creation_propagate_down ;
					}
				continue ;
				}
			else { // right is better than left
				if (m_key[right] > m_key[i]) { // right is also better than top. swap
					temp_key = m_key[right] ; m_key[right] = m_key[i] ; m_key[i] = temp_key ;
					temp_key = m_CurrentIdx2OriginalIdxMap[right] ; m_CurrentIdx2OriginalIdxMap[right] = m_CurrentIdx2OriginalIdxMap[i] ; m_CurrentIdx2OriginalIdxMap[i] = temp_key ;
					m_OriginalIdx2CurrentIdxMap[m_CurrentIdx2OriginalIdxMap[right]] = right ;
					m_OriginalIdx2CurrentIdxMap[m_CurrentIdx2OriginalIdxMap[i]] = i ;
					i = right ; // follow right side
					goto heap_creation_propagate_down ;
					}
				continue ;
				}
			}
		else {
			if (right > m_size) { // only left child
				if (m_key[left] < m_key[i]) { // swap
					temp_key = m_key[left] ; m_key[left] = m_key[i] ; m_key[i] = temp_key ;
					temp_key = m_CurrentIdx2OriginalIdxMap[left] ; m_CurrentIdx2OriginalIdxMap[left] = m_CurrentIdx2OriginalIdxMap[i] ; m_CurrentIdx2OriginalIdxMap[i] = temp_key ;
					m_OriginalIdx2CurrentIdxMap[m_CurrentIdx2OriginalIdxMap[left]] = left ;
					m_OriginalIdx2CurrentIdxMap[m_CurrentIdx2OriginalIdxMap[i]] = i ;
					}
				continue ;
				}

			// two children. the best key of three goes to the top.
			if (m_key[left] < m_key[right]) { // left is better than right
				if (m_key[left] < m_key[i]) { // left is also better than top. swap
					temp_key = m_key[left] ; m_key[left] = m_key[i] ; m_key[i] = temp_key ;
					temp_key = m_CurrentIdx2OriginalIdxMap[left] ; m_CurrentIdx2OriginalIdxMap[left] = m_CurrentIdx2OriginalIdxMap[i] ; m_CurrentIdx2OriginalIdxMap[i] = temp_key ;
					m_OriginalIdx2CurrentIdxMap[m_CurrentIdx2OriginalIdxMap[left]] = left ;
					m_OriginalIdx2CurrentIdxMap[m_CurrentIdx2OriginalIdxMap[i]] = i ;
					i = left ; // follow left side
					goto heap_creation_propagate_down ;
					}
				continue ;
				}
			else { // right is better than left
				if (m_key[right] < m_key[i]) { // right is also better than top. swap
					temp_key = m_key[right] ; m_key[right] = m_key[i] ; m_key[i] = temp_key ;
					temp_key = m_CurrentIdx2OriginalIdxMap[right] ; m_CurrentIdx2OriginalIdxMap[right] = m_CurrentIdx2OriginalIdxMap[i] ; m_CurrentIdx2OriginalIdxMap[i] = temp_key ;
					m_OriginalIdx2CurrentIdxMap[m_CurrentIdx2OriginalIdxMap[right]] = right ;
					m_OriginalIdx2CurrentIdxMap[m_CurrentIdx2OriginalIdxMap[i]] = i ;
					i = right ; // follow right side
					goto heap_creation_propagate_down ;
					}
				continue ;
				}
			}
		}

	return 0 ;
}


int CMauiHeap_Basic_ReductionWithPermutationMapping::ContainsKey(long given_key)
{
	for (unsigned long i = 1 ; i <= m_size ; i++) {
		if (m_key[i] == given_key) return 1 ;
		}

	return 0 ;
}


/*
	This function adds a key to the heap.
	NB! it does not check if this key is already in the heap.

	Returns FALSE if operation failed.
	NB! even if this operation fails (because new memory for the heap could not be allocated)
	the old heap is still there.
*/

long CMauiHeap_Basic_ReductionWithPermutationMapping::Insert(long new_key /* new key to be added to the heap */)
{
	signed long i, j ;
	long temp_key ;

/*
	Qualification check.
*/
	if (m_size >= m_allocated_space) {
		if (! ReAllocateMemory()) return 0 ;
		}

	i = ++m_size ;
	m_key[i] = new_key ;

	if (m_bSortOrderIsDecreasing) {
		for (j = i >> 1 ; j > 0 ; j = i >> 1) {
			if (m_key[j] < m_key[i]) {
				temp_key = m_key[j] ; m_key[j] = m_key[i] ; m_key[i] = temp_key ;
				}
			else return 1 ;
			i = j ;
			}
		}
	else {
		for (j = i >> 1 ; j > 0 ; j = i >> 1) {
			if (m_key[j] > m_key[i]) {
				temp_key = m_key[j] ; m_key[j] = m_key[i] ; m_key[i] = temp_key ;
				}
			else return 1 ;
			i = j ;
			}
		}

	return 1 ;
}


int CMauiHeap_Basic_ReductionWithPermutationMapping::UpdateKey(unsigned long Idx, long new_key)
{
	if (Idx <= 0 || Idx > m_size) 
		return 0 ;

	unsigned long left, right, parent ;
	long temp_key ;
	unsigned long i = Idx ;

	if (m_bSortOrderIsDecreasing) {
		// TODO
		return 0 ;
		}
	else {
		if (m_key[Idx] < new_key) { // value of the key is being increased; propagate down
			m_key[Idx] = new_key ;
heap_update_propagate_down_DEC :
			left = i << 1 ;
			right = left + 1 ;
			if (left > m_size) return 1 ; // a leaf, no children
			else if (right > m_size) { // only left child
				if (m_key[left] < m_key[i]) { // swap
					temp_key = m_key[left] ; m_key[left] = m_key[i] ; m_key[i] = temp_key ;
					temp_key = m_CurrentIdx2OriginalIdxMap[left] ; m_CurrentIdx2OriginalIdxMap[left] = m_CurrentIdx2OriginalIdxMap[i] ; m_CurrentIdx2OriginalIdxMap[i] = temp_key ;
					m_OriginalIdx2CurrentIdxMap[m_CurrentIdx2OriginalIdxMap[left]] = left ;
					m_OriginalIdx2CurrentIdxMap[m_CurrentIdx2OriginalIdxMap[i]] = i ;
					}
				return 1 ;
				}

			// two children. the best key of three goes to the top.
			if (m_key[left] < m_key[right]) { // left is better than right
				if (m_key[left] < m_key[i]) { // left is also better than top. swap
					temp_key = m_key[left] ; m_key[left] = m_key[i] ; m_key[i] = temp_key ;
					temp_key = m_CurrentIdx2OriginalIdxMap[left] ; m_CurrentIdx2OriginalIdxMap[left] = m_CurrentIdx2OriginalIdxMap[i] ; m_CurrentIdx2OriginalIdxMap[i] = temp_key ;
					m_OriginalIdx2CurrentIdxMap[m_CurrentIdx2OriginalIdxMap[left]] = left ;
					m_OriginalIdx2CurrentIdxMap[m_CurrentIdx2OriginalIdxMap[i]] = i ;
					i = left ; // follow left side
					goto heap_update_propagate_down_DEC ;
					}
				}
			else { // right is better than left
				if (m_key[right] < m_key[i]) { // right is also better than top. swap
					temp_key = m_key[right] ; m_key[right] = m_key[i] ; m_key[i] = temp_key ;
					temp_key = m_CurrentIdx2OriginalIdxMap[right] ; m_CurrentIdx2OriginalIdxMap[right] = m_CurrentIdx2OriginalIdxMap[i] ; m_CurrentIdx2OriginalIdxMap[i] = temp_key ;
					m_OriginalIdx2CurrentIdxMap[m_CurrentIdx2OriginalIdxMap[right]] = right ;
					m_OriginalIdx2CurrentIdxMap[m_CurrentIdx2OriginalIdxMap[i]] = i ;
					i = right ; // follow right side
					goto heap_update_propagate_down_DEC ;
					}
				}
			}
		else if (m_key[Idx] > new_key) { // value of the key is being reduced; propagate up
			m_key[Idx] = new_key ;
heap_update_propagate_up_DEC :
			parent = i >> 1 ;
			if (parent <= 0) return 1 ;
			if (m_key[i] < m_key[parent]) { // swap
				temp_key = m_key[parent] ; m_key[parent] = m_key[i] ; m_key[i] = temp_key ;
				temp_key = m_CurrentIdx2OriginalIdxMap[parent] ; m_CurrentIdx2OriginalIdxMap[parent] = m_CurrentIdx2OriginalIdxMap[i] ; m_CurrentIdx2OriginalIdxMap[i] = temp_key ;
				m_OriginalIdx2CurrentIdxMap[m_CurrentIdx2OriginalIdxMap[parent]] = parent ;
				m_OriginalIdx2CurrentIdxMap[m_CurrentIdx2OriginalIdxMap[i]] = i ;
				goto heap_update_propagate_up_DEC ;
				}
			return 0 ;
			}
		}

	return 0 ;
}


int CMauiHeap_Basic_ReductionWithPermutationMapping::UpdateHeap_AfterRemove(unsigned long Idx)
{
	if (Idx <= 0 || Idx > m_size) 
		return 0 ;

	unsigned long left, right ;
	long temp_key ;
	unsigned long i = Idx ;

	// we assume that before calling this function, m_key[Idx] = m_key[m_size--] was done; i.e. m_key[Idx] is possible larger than its children, 
	// but never smaller, so m_key[Idx] needs to propagated down the heap tree.

	if (m_bSortOrderIsDecreasing) {
heap_deletion_propagate_down_INC :
		left = i << 1 ;
		right = left + 1 ;
		if (left > m_size) return 1 ; // a leaf, no children
		else if (right > m_size) { // only left child
			if (m_key[left] > m_key[i]) { // swap
				temp_key = m_key[left] ; m_key[left] = m_key[i] ; m_key[i] = temp_key ;
				temp_key = m_CurrentIdx2OriginalIdxMap[left] ; m_CurrentIdx2OriginalIdxMap[left] = m_CurrentIdx2OriginalIdxMap[i] ; m_CurrentIdx2OriginalIdxMap[i] = temp_key ;
				m_OriginalIdx2CurrentIdxMap[m_CurrentIdx2OriginalIdxMap[left]] = left ;
				m_OriginalIdx2CurrentIdxMap[m_CurrentIdx2OriginalIdxMap[i]] = i ;
				}
			return 1 ;
			}

		// two children. the best key of three goes to the top.
		if (m_key[left] > m_key[right]) { // left is better than right
			if (m_key[left] > m_key[i]) { // left is also better than top. swap
				temp_key = m_key[left] ; m_key[left] = m_key[i] ; m_key[i] = temp_key ;
				temp_key = m_CurrentIdx2OriginalIdxMap[left] ; m_CurrentIdx2OriginalIdxMap[left] = m_CurrentIdx2OriginalIdxMap[i] ; m_CurrentIdx2OriginalIdxMap[i] = temp_key ;
				m_OriginalIdx2CurrentIdxMap[m_CurrentIdx2OriginalIdxMap[left]] = left ;
				m_OriginalIdx2CurrentIdxMap[m_CurrentIdx2OriginalIdxMap[i]] = i ;
				i = left ; // follow left side
				goto heap_deletion_propagate_down_INC ;
				}
			}
		else { // right is better than left
			if (m_key[right] > m_key[i]) { // right is also better than top. swap
				temp_key = m_key[right] ; m_key[right] = m_key[i] ; m_key[i] = temp_key ;
				temp_key = m_CurrentIdx2OriginalIdxMap[right] ; m_CurrentIdx2OriginalIdxMap[right] = m_CurrentIdx2OriginalIdxMap[i] ; m_CurrentIdx2OriginalIdxMap[i] = temp_key ;
				m_OriginalIdx2CurrentIdxMap[m_CurrentIdx2OriginalIdxMap[right]] = right ;
				m_OriginalIdx2CurrentIdxMap[m_CurrentIdx2OriginalIdxMap[i]] = i ;
				i = right ; // follow right side
				goto heap_deletion_propagate_down_INC ;
				}
			}
		}
	else {
heap_deletion_propagate_down_DEC :
		left = i << 1 ;
		right = left + 1 ;
		if (left > m_size) return 1 ; // a leaf, no children
		else if (right > m_size) { // only left child
			if (m_key[left] < m_key[i]) { // swap
				temp_key = m_key[left] ; m_key[left] = m_key[i] ; m_key[i] = temp_key ;
				temp_key = m_CurrentIdx2OriginalIdxMap[left] ; m_CurrentIdx2OriginalIdxMap[left] = m_CurrentIdx2OriginalIdxMap[i] ; m_CurrentIdx2OriginalIdxMap[i] = temp_key ;
				m_OriginalIdx2CurrentIdxMap[m_CurrentIdx2OriginalIdxMap[left]] = left ;
				m_OriginalIdx2CurrentIdxMap[m_CurrentIdx2OriginalIdxMap[i]] = i ;
				}
			return 1 ;
			}

		// two children. the best key of three goes to the top.
		if (m_key[left] < m_key[right]) { // left is better than right
			if (m_key[left] < m_key[i]) { // left is also better than top. swap
				temp_key = m_key[left] ; m_key[left] = m_key[i] ; m_key[i] = temp_key ;
				temp_key = m_CurrentIdx2OriginalIdxMap[left] ; m_CurrentIdx2OriginalIdxMap[left] = m_CurrentIdx2OriginalIdxMap[i] ; m_CurrentIdx2OriginalIdxMap[i] = temp_key ;
				m_OriginalIdx2CurrentIdxMap[m_CurrentIdx2OriginalIdxMap[left]] = left ;
				m_OriginalIdx2CurrentIdxMap[m_CurrentIdx2OriginalIdxMap[i]] = i ;
				i = left ; // follow left side
				goto heap_deletion_propagate_down_DEC ;
				}
			}
		else { // right is better than left
			if (m_key[right] < m_key[i]) { // right is also better than top. swap
				temp_key = m_key[right] ; m_key[right] = m_key[i] ; m_key[i] = temp_key ;
				temp_key = m_CurrentIdx2OriginalIdxMap[right] ; m_CurrentIdx2OriginalIdxMap[right] = m_CurrentIdx2OriginalIdxMap[i] ; m_CurrentIdx2OriginalIdxMap[i] = temp_key ;
				m_OriginalIdx2CurrentIdxMap[m_CurrentIdx2OriginalIdxMap[right]] = right ;
				m_OriginalIdx2CurrentIdxMap[m_CurrentIdx2OriginalIdxMap[i]] = i ;
				i = right ; // follow right side
				goto heap_deletion_propagate_down_DEC ;
				}
			}
		}

	return 1 ;
}


/*
	This function returns the key with the topmost value.

	Returns FALSE if the heap is empty.
*/

long CMauiHeap_Basic_ReductionWithPermutationMapping::Remove(long *ret_key /* returned key will be written here */)
{
/*
	Qualification check.
*/
	if (m_size < 1) return 0 ;

/*
	Get the first key.
*/
	if (ret_key) *ret_key = m_key[1] ;

/*
	Reorganize the heap.
*/
	m_key[1] = m_key[m_size] ;
	m_OriginalIdx2CurrentIdxMap[m_CurrentIdx2OriginalIdxMap[1]] = 0 ;
	m_CurrentIdx2OriginalIdxMap[1] = m_CurrentIdx2OriginalIdxMap[m_size] ;
	m_OriginalIdx2CurrentIdxMap[m_CurrentIdx2OriginalIdxMap[1]] = 1 ;
	--m_size ;
	UpdateHeap_AfterRemove(1) ;

	return 1 ;
}


long CMauiHeap_Basic_ReductionWithPermutationMapping::Remove_key(long key /* key to remove */)
{
	for (unsigned long i = 1 ; i <= m_size ; i++) {
		if (m_key[i] == key) {
			// update the heap
			m_key[i] = m_key[m_size] ;
			m_OriginalIdx2CurrentIdxMap[m_CurrentIdx2OriginalIdxMap[i]] = 0 ;
			m_CurrentIdx2OriginalIdxMap[i] = m_CurrentIdx2OriginalIdxMap[m_size] ;
			m_OriginalIdx2CurrentIdxMap[m_CurrentIdx2OriginalIdxMap[i]] = i ;
			--m_size ;
			UpdateHeap_AfterRemove(i) ;
			return 1 ;
			}
		}

	return 0 ;
}


long CMauiHeap_Basic_ReductionWithPermutationMapping::Remove_idx(long idx, long *ret_key /* returned key will be written here */)
{
	if (m_size < 1 || idx < 1 || idx > m_size) return 0 ;
	if (NULL != ret_key) *ret_key = m_key[idx] ;
	m_OriginalIdx2CurrentIdxMap[m_CurrentIdx2OriginalIdxMap[idx]] = 0 ;
	if (idx < m_size) {
		m_key[idx] = m_key[m_size] ;
		m_CurrentIdx2OriginalIdxMap[idx] = m_CurrentIdx2OriginalIdxMap[m_size] ;
		m_OriginalIdx2CurrentIdxMap[m_CurrentIdx2OriginalIdxMap[idx]] = idx ;
		--m_size ;
		UpdateHeap_AfterRemove(idx) ;
		}
	else 
		m_size-- ;
	return 1 ;
}


/*
	Unlike remove(...) this function just returns the value of the first key
	in the heap (and the information associated with it), but does not delete the key from the heap.

	Returns FALSE if the heap is empty.
*/

long CMauiHeap_Basic_ReductionWithPermutationMapping::GetFirst(long *ret_key /* returned key will be written here */)
{
	if (m_size < 1) return 0 ;
	if (ret_key) *ret_key = m_key[1] ;
	return 1 ;
}

int CMauiHeap_Basic_ReductionWithPermutationMapping::operator=(const CMauiHeap_Basic_ReductionWithPermutationMapping & Heap)
{
	if (m_allocated_space != Heap.m_allocated_space) {
		if (0 == AllocateMemory(1+Heap.m_allocated_space)) 
			return 1 ;
		}
	m_re_allocation_size = Heap.m_re_allocation_size ;
	m_bSortOrderIsDecreasing = Heap.m_bSortOrderIsDecreasing ;
	m_size = Heap.m_size ;
	for (int i = 1 ; i <= m_size ; i++) {
		m_key[i] = Heap.m_key[i] ;
		m_CurrentIdx2OriginalIdxMap[i] = Heap.m_CurrentIdx2OriginalIdxMap[i] ;
		m_OriginalIdx2CurrentIdxMap[i] = Heap.m_OriginalIdx2CurrentIdxMap[i] ;
		}

	return 0 ;
}

