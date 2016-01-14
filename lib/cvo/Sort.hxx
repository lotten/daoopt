/*
	This module contains sorting function's declarations.

	Kalev Kask, March 2002.
*/

#ifndef SORT_HXX_INCLUDED

#ifdef LINUX
typedef int64_t INT64 ;
#else 
typedef __int64 INT64 ;
#endif

void QuickSortShort(short *input_key, unsigned long len, long *input_data, 
	// stack; passed in, to make this function thread-safe
	long left[32], long right[32]) ;

void QuickSortDouble(double *input_key, unsigned long len, long *input_data, 
	// stack; passed in, to make this function thread-safe
	long left[32], long right[32]) ;
void QuickSortDouble_Descending(double *input_key, unsigned long len, long *input_data, 
	// stack; passed in, to make this function thread-safe
	long left[32], long right[32]) ;
int SortCheckDouble(double *key, unsigned long len) ;

void QuickSortLong(long *input_key, unsigned long len, long *input_data, 
	// stack; passed in, to make this function thread-safe
	long left[32], long right[32]) ;
void QuickSortLong_Descending(long *input_key, unsigned long len, long *input_data, 
	// stack; passed in, to make this function thread-safe
	long left[32], long right[32]) ;
void QuickSortLong2(long *input_key, unsigned long len, 
	// stack; passed in, to make this function thread-safe
	long left[32], long right[32]) ;
int SortCheckLong(long *key, unsigned long len) ;
void QuickSortLong_i64(long *input_key, unsigned long len, INT64 *input_data,
	// stack; passed in, to make this function thread-safe
	long left[32], long right[32]) ;
void QuickSortLong_i64_Descending(long *input_key, unsigned long len, INT64 *input_data, 
	// stack; passed in, to make this function thread-safe
	long left[32], long right[32]) ;

void QuickSorti64(INT64 *input_key, unsigned long len, long *input_data,
	// stack; passed in, to make this function thread-safe
	long left[32], long right[32]) ;
int SortChecki64(INT64 *key, unsigned long len) ;

void QuickSortWchar_t(const wchar_t **input_key, unsigned long len, long *input_data, 
	// stack; passed in, to make this function thread-safe
	long left[32], long right[32]) ;
void QuickSortWchar_t(const wchar_t **input_key, unsigned long len, 
	// stack; passed in, to make this function thread-safe
	long left[32], long right[32]) ;
void QuickSortWchar_t_Descending(const wchar_t **input_key, unsigned long len, long *input_data, 
	// stack; passed in, to make this function thread-safe
	long left[32], long right[32]) ;

void QuickSortChar(const char **input_key, unsigned long len, long *input_data, 
	// stack; passed in, to make this function thread-safe
	long left[32], long right[32]) ;

void QuickSortMem(const char **input_key, unsigned long keysize, unsigned long len, long *input_data, 
	// stack; passed in, to make this function thread-safe
	long left[32], long right[32]) ;

#endif // SORT_HXX_INCLUDED
