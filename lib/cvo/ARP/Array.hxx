/*
	File Name :

		MAUI_ARRAY.HXX

	Purpose :

		CMauiArray class definition.

	Started :

		Kalev Kask, February 2002.

	Last modified : 
	
		February 12, 2002.

	Changes :

		02/12/2002 : This file started. KK.
*/

#ifndef MAUI_UTIL_ARRAY_HXX_INCLUDED
#define MAUI_UTIL_ARRAY_HXX_INCLUDED

#pragma warning (disable: 4661)
#pragma warning (disable: 4251)
//disable warnings on 255 char debug symbols
//#pragma warning (disable: 4786)
//disable warnings on extern before template instantiation
#pragma warning (disable: 4231)

template<class TYPE> class CMauiArray
{

// *****************************************************************************************************
// Data.
// *****************************************************************************************************

protected :

	// 
	// Name :
	// 
	//		CMauiArray::mp_Data
	// 
	// Purpose :
	// 
	//		Array of elements.
	// 
	TYPE *mp_Data ;

	// 
	// Name :
	// 
	//		CMauiArray::m_Size
	// 
	// Purpose :
	// 
	//		Number of elements in this array.
	// 
	int m_Size ;

	// 
	// Name :
	// 
	//		CMauiArray::m_AllocatedSize
	// 
	// Purpose :
	// 
	//		Space allocated for the array.
	// 
	int m_AllocatedSize ;

	// 
	// Name :
	// 
	//		CMauiArray::m_ReAllocationSize
	// 
	// Purpose :
	// 
	//		Reallocation size.
	// 
	int m_ReAllocationSize ;

public :

	// 
	// Name :
	// 
	//		CMauiArray::SetReAllocationSize
	// 
	// Purpose :
	// 
	//		Set m_ReAllocationSize.
	// 
	// Returns :
	// 
	//		New size.
	// 
	int SetReAllocationSize(int ReAllocationSize) ;

	// 
	// Name :
	// 
	//		CMauiArray::GetSize
	// 
	// Purpose :
	// 
	//		Get the number of elements in the array.
	// 
	// Input :
	// 
	//		None.
	// 
	// Output :
	// 
	//		None.
	// 
	// Returns :
	// 
	//		Size.
	// 
	inline int GetSize(void) const { return m_Size ; }

	// 
	// Name :
	// 
	//		CMauiArray::ReSize
	// 
	// Purpose :
	// 
	//		After that, GetSize() will return NewSize.
	// 
	// Input :
	// 
	//		NewSize.
	// 
	// Output :
	// 
	//		None.
	// 
	// Returns :
	// 
	//		Size.
	// 
	int ReSize(int NewSize) ;

	// 
	// Name :
	// 
	//		CMauiArray::Empty
	// 
	// Purpose :
	// 
	//		Empty the array.
	// 
	// Input :
	// 
	//		None.
	// 
	// Output :
	// 
	//		None.
	// 
	// Returns :
	// 
	//		None.
	// 
	void Empty(void) ;

	// 
	// Name :
	// 
	//		CMauiArray::ReAllocateArray
	// 
	// Purpose :
	// 
	//		Reallocate the array.
	// 
	//		This function is used when a new element is added, but there is no space in the current array.
	//		This function fails when the new size is not more than the current allocated space.
	// 
	// Input :
	// 
	//		New size.
	// 
	// Output :
	// 
	//		None.
	// 
	// Returns :
	// 
	//		TRUE iff successful.
	// 
	int ReAllocateArray(int size) ;

	// 
	// Name :
	// 
	//		CMauiArray::GetAt
	// 
	// Purpose :
	// 
	//		Get a specific element from the array.
	// 
	// Input :
	// 
	//		Index. If the index is out of bounds, the result is undefined.
	// 
	// Output :
	// 
	//		None.
	// 
	// Returns :
	// 
	//		Reference to the element.
	// 
	TYPE & GetAt(int idx) const ;

	// 
	// Name :
	// 
	//		CMauiArray::Empty
	// 
	// Purpose :
	// 
	//		Get a specific element from the array.
	// 
	// Input :
	// 
	//		Index. If the index is out of bounds (> size), array size will be increased by filling 
	//		with objects created by default constructor.
	// 
	// Output :
	// 
	//		None.
	// 
	// Returns :
	// 
	//		Reference to the element.
	// 
	TYPE & operator[](int idx) const ;

	// 
	// Name :
	// 
	//		CMauiArray::Find
	// 
	// Purpose :
	// 
	//		Check if a given element is in the array.
	// 
	// Input :
	// 
	//		Element.
	// 
	// Output :
	// 
	//		None.
	// 
	// Returns :
	// 
	//		Index (>= 0) if yes, otherwise -1.
	// 
	int Find(const TYPE & element) const ;

	// 
	// Name :
	// 
	//		CMauiArray::Insert
	// 
	// Purpose :
	// 
	//		Insert a given element into the array.
	// 
	// Input :
	// 
	//		Element.
	// 
	// Output :
	// 
	//		None.
	// 
	// Returns :
	// 
	//		Index of the inserted element, or -1 iff fails.
	// 
	int Insert(const TYPE & NewElement) ;

	// 
	// Name :
	// 
	//		CMauiArray::InsertAt
	// 
	// Purpose :
	// 
	//		Insert a given element into the array at a specific location.
	// 
	// Input :
	// 
	//		Index.
	//		Element.
	// 
	// Output :
	// 
	//		None.
	// 
	// Returns :
	// 
	//		0 iff ok.
	// 
	int InsertAt(int idx, const TYPE & NewElement) ;

	// 
	// Name :
	// 
	//		CMauiArray::RemoveAt
	// 
	// Purpose :
	// 
	//		Remove an element at the given index from the array.
	// 
	// Input :
	// 
	//		Index.
	// 
	// Output :
	// 
	//		None.
	// 
	// Returns :
	// 
	//		None.
	// 
	int RemoveAt(int idx) ;

	// 
	// Name :
	// 
	//		CMauiArray::Data
	// 
	// Purpose :
	// 
	//		Get a pointer to the data array.
	// 
	// Input :
	// 
	//		None.
	// 
	// Output :
	// 
	//		None.
	// 
	// Returns :
	// 
	//		Ptr to data array.
	// 
	TYPE *Data(void) ;

// *****************************************************************************************************
// Construction/Destruction.
// *****************************************************************************************************

public :

	// 
	// Name :
	// 
	//		CMauiArray::CMauiArray(void)
	// 
	// Purpose :
	// 
	//		Default constructor.
	// 
	// Input :
	// 
	//		None.
	// 
	// Output :
	// 
	//		None.
	// 
	// Returns :
	// 
	//		None.
	// 
	CMauiArray(void) ;

	// 
	// Name :
	// 
	//		CMauiArray::CMauiArray(CMauiArray &)
	// 
	// Purpose :
	// 
	//		Copy constructor.
	// 
	// Input :
	// 
	//		A reference to the existing object.
	// 
	// Output :
	// 
	//		None.
	// 
	// Returns :
	// 
	//		None.
	// 
	CMauiArray(CMauiArray &) ;

	// 
	// Name :
	// 
	//		CMauiArray::~CMauiArray(void)
	// 
	// Purpose :
	// 
	//		(Virtual) destructor.
	// 
	// Input :
	// 
	//		None.
	// 
	// Output :
	// 
	//		None.
	// 
	// Returns :
	// 
	//		None.
	// 
	virtual ~CMauiArray(void);
} ;

#endif // MAUI_UTIL_ARRAY_HXX_INCLUDED
