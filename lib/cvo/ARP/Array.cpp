/*
	File Name :

		MAUI_ARRAY.CPP

	Purpose :

		CMauiArray class implementation.

	Started :

		Kalev Kask, February 2002.

	Last modified :

		February 12, 2002.

	Changes :

		02/12/2002 : This file started. KK.
*/

#include <stdlib.h>
#include <string.h>

#include "Array.hxx"

template<class TYPE> CMauiArray<TYPE>::CMauiArray(void)
	:
	mp_Data(NULL),
	m_Size(0),
	m_AllocatedSize(0),
	m_ReAllocationSize(8)
{
}


template<class TYPE> CMauiArray<TYPE>::CMauiArray(CMauiArray & MauiArray)
	:
	mp_Data(NULL),
	m_Size(0),
	m_AllocatedSize(0),
	m_ReAllocationSize(8)
{
	m_AllocatedSize = MauiArray.m_AllocatedSize ;
	m_Size = MauiArray.m_Size ;

	if (m_AllocatedSize > 0) {
		mp_Data = new TYPE[m_AllocatedSize] ;
		if (NULL == mp_Data) { // error, not enough memory
			m_Size = m_AllocatedSize = 0 ;
			return ;
			}
		}

//	memcpy(mp_Data, MauiArray.mp_Data, m_Size * sizeof(TYPE)) ;
	for (int i = 0 ; i < MauiArray.m_Size ; i++) {
		mp_Data[i] = MauiArray.mp_Data[i] ;
		}
}


template<class TYPE> CMauiArray<TYPE>::~CMauiArray(void)
{
	if (mp_Data)
		delete [] mp_Data ;
}

template<class TYPE> int CMauiArray<TYPE>::SetReAllocationSize(int ReAllocationSize)
{
	if (ReAllocationSize < 1) ReAllocationSize = 1 ;
	m_ReAllocationSize = ReAllocationSize ;
	return m_ReAllocationSize ;
}


//template<class TYPE> int CMauiArray<TYPE>::GetSize(void) const
//{
//	return m_Size ;
//}


template<class TYPE> void CMauiArray<TYPE>::Empty(void)
{
	m_Size = 0 ;
}


template<class TYPE> int CMauiArray<TYPE>::ReSize(int NewSize)
{
	if (NewSize <= 0) {
		Empty() ;
		}
	else if (NewSize < m_Size) {
		m_Size = NewSize ;
		}
	else {
		// need to add NewSize - m_Size elements; check if there is enough space
		if (NewSize >= m_AllocatedSize) { // need to reallocate the array
			int ns = m_AllocatedSize + m_ReAllocationSize ;
			if (ns < NewSize)
				ns = NewSize ;
			if (! ReAllocateArray(ns)) return 0 ;
			}
		for (; m_Size < NewSize ; m_Size) {
			mp_Data[m_Size++] = TYPE() ;
			}
		}

	return m_Size ;
}


template<class TYPE> int CMauiArray<TYPE>::ReAllocateArray(int size)
{
	if (size < m_AllocatedSize)
		return 0 ;
	if (size == m_AllocatedSize)
		return 0 ;

	TYPE *pNewData = new TYPE[size] ;
	if (NULL == pNewData)
		return 0 ;
	if (m_Size > 0) {
		// we should not do memcopy; some array members are objects with pointers in them; they cannot be moved with memcopy.
//		memcpy(pNewData, mp_Data, m_Size * sizeof(TYPE)) ;
		for (int i = 0 ; i < m_Size ; i++) {
			pNewData[i] = mp_Data[i] ;
			}
		}
	if (mp_Data)
		delete [] mp_Data ;
	mp_Data = pNewData ;
	m_AllocatedSize = size ;

	return 1 ;
}


template<class TYPE> TYPE & CMauiArray<TYPE>::GetAt(int idx) const
{
	if (idx < 0) {
		idx = 0 ;
		}
	else if (idx >= m_Size) {
		idx = m_Size - 1 ;
		}
	return mp_Data[idx] ;
}


template<class TYPE> TYPE & CMauiArray<TYPE>::operator[](int idx) const
{
	if (idx < 0) {
		idx = 0 ;
		}
	else if (idx >= m_Size) {
		idx = m_Size - 1 ;
		}
//	if (idx >= m_AllocatedSize) {
//		if (! ReAllocateStatesArray(idx + m_ReAllocationSize))
//			idx = m_Size - 1 ;
//		}
//	if (idx >= m_Size) m_Size = idx + 1 ;
	return mp_Data[idx] ;
}


template<class TYPE> int CMauiArray<TYPE>::Find(const TYPE & element) const
{
	if (NULL == mp_Data) return -1 ;
	for (int i = 0 ; i < m_Size ; i++) {
		if (mp_Data[i] == element) return i ;
		}
	return -1 ;
}


template<class TYPE> int CMauiArray<TYPE>::Insert(const TYPE & new_element) 
{
	if (m_Size >= m_AllocatedSize) { // need to reallocate the array
		if (! ReAllocateArray(m_AllocatedSize + m_ReAllocationSize)) return -1 ;
		}
	mp_Data[m_Size] = (TYPE &) new_element ;
	return m_Size++ ;
}


template<class TYPE> int CMauiArray<TYPE>::InsertAt(int idx, const TYPE & NewElement)
{
	if (idx < 0) 
		return -1 ;
	if (idx >= m_Size) {
		return Insert(NewElement) ;
		}
	else {
		if (m_Size >= m_AllocatedSize) {
			if (! ReAllocateArray(m_AllocatedSize + m_ReAllocationSize)) return -1 ;
			}
		for (int i = m_Size - 1 ; i >= idx ; i--) mp_Data[i+1] = mp_Data[i] ;
		mp_Data[idx] = (TYPE &) NewElement ;
		++m_Size ;
		}

	return idx ;
}


template<class TYPE> int CMauiArray<TYPE>::RemoveAt(int idx)
{
	if (idx >= m_Size || idx < 0) return 0 ;
	--m_Size ;
	if (idx < m_Size) {
//		memmove((mp_Data + idx), (mp_Data + idx + 1), sizeof(TYPE) * (m_Size - idx)) ;
		for (int i = idx ; i < m_Size ; i++) {
			mp_Data[i] = mp_Data[i+1] ;
			}
		}
	return 1 ;
}


template<class TYPE> TYPE *CMauiArray<TYPE>::Data(void)
{
	return mp_Data ;
}

