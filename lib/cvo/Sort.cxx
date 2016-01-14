/*
	This module contains sorting routines.

	Kalev Kask, March 2002.
*/


/*
	Regular MS C++ include files.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <math.h>

#include "Sort.hxx"

/*
	Local variables.
*/

//// these are used in QuickSort to keep track of the stack
//static long left[32], right[32] ;

/* ********** ********** ********** ********** ********** ********** ********** ********** ********** *
 * In the following, there are two functions to sort an array of numbers.
 * The first function is for the case when keys are real numbers,
 * the second for the case when the keys are long integers.
 * ********** ********** ********** ********** ********** ********** ********** ********** ********** */

/*
	Function Quick_Sort(...) is used to sort an array of real numbers.
	Keys (real numbers) are in the array 'key'.
	The size of the array is 'len'. 
	Indeces in 'key' start from 0 and go to len-1 (incl.)

	Array 'data' contains data associated with every key. As the array of keys is rearranged,
	'data' is rearranged in exactly the same way. That is, the same permutation is applied to
	'data' as is applied to 'key'.

	Function DBM_QuickSort_Check_...(...) can be used to check if an array of real is sorted in a 
	non-decreasing order. It returns 1 iff the array is sorted and 0 otherwise.
*/

int SortCheckDouble(double *key, unsigned long len)
{
	unsigned long i ;

	for (i = 1 ; i < len ; i++) {
		if (key[i-1] > key[i]) return 0 ;
		}
	return 1 ;
}


void QuickSortDouble(double *input_key, unsigned long len, long *input_data, 
	// stack; passed in, to make this function thread-safe
	long left[32], long right[32])
{
	double temp, pivot ;
	long l, r, i, j, k, m, n, stack ;
	double *key ;
	long *data ;

	// qualification check
	if (len < 2) return ;

	// input array starts from 0, we want the key array to start from 1.
	key = input_key - 1 ;
	data = input_data - 1 ;

	l = 1 ;
	r = len ;
	stack = 0 ;
next_quicksort_call: ;

	i = r - l ;
	if (i < 1) { // at most one key -> take next interval
		goto take_next_quicksort_interval ;
		}
	else if (i == 1) { // two keys, sort directly
		if (key[l] > key[r]) {
			temp = key[l] ; key[l] = key[r] ; key[r] = temp ;
			n = data[l] ; data[l] = data[r] ; data[r] = n ;
			}
		goto take_next_quicksort_interval ;
		}
	else if (i == 2) { // three keys, sort directly
		k = l + 1 ;
		if (key[l] > key[k]) {
			if (key[r] >= key[l]) { // switch l and k
				temp = key[k] ; key[k] = key[l] ; key[l] = temp ;
				n = data[k] ; data[k] = data[l] ; data[l] = n ;
				}
			// else : now left one is the largest
			else if (key[k] < key[r]) { // rotate left				
				temp = key[l] ; key[l] = key[k] ; key[k] = key[r] ; key[r] = temp ;
				n = data[l] ; data[l] = data[k] ; data[k] = data[r] ; data[r] = n ;
				}
			else { // switch l and r (middle one is at least as large as r)
				temp = key[l] ; key[l] = key[r] ; key[r] = temp ;
				n = data[l] ; data[l] = data[r] ; data[r] = n ;
				}
			}
		else if (key[k] > key[r]) {
			if (key[l] > key[r]) { // rotate right 
				temp = key[r] ; key[r] = key[k] ; key[k] = key[l] ; key[l] = temp ;
				n = data[r] ; data[r] = data[k] ; data[k] = data[l] ; data[l] = n ;
				}
			else { // switch l+1 and r
				temp = key[k] ; key[k] = key[r] ; key[r] = temp ;
				n = data[k] ; data[k] = data[r] ; data[r] = n ;
				}
			}
		// else everything is fine
		goto take_next_quicksort_interval ;
		}

	// more than 3 elements left
	// ********** general QuickSort procedure **********
	// find median of the first, last and middle
	k = (l + r) >> 1 ; // k is the key in the middle
	if (key[l] > key[k]) {
		if (key[r] >= key[l]) {
			temp = key[k] ; key[k] = key[l] ; key[l] = temp ;
			n = data[k] ; data[k] = data[l] ; data[l] = n ;
			}
		else if (key[k] < key[r]) {
			temp = key[l] ; key[l] = key[k] ; key[k] = key[r] ; key[r] = temp ;
			n = data[l] ; data[l] = data[k] ; data[k] = data[r] ; data[r] = n ;
			}
		else {
			temp = key[l] ; key[l] = key[r] ; key[r] = temp ;
			n = data[l] ; data[l] = data[r] ; data[r] = n ;
			}
		}
	else if (key[k] > key[r]) {
		if (key[l] > key[r]) {
			temp = key[r] ; key[r] = key[k] ; key[k] = key[l] ; key[l] = temp ;
			n = data[r] ; data[r] = data[k] ; data[k] = data[l] ; data[l] = n ;
			}
		else {
			temp = key[k] ; key[k] = key[r] ; key[r] = temp ;
			n = data[k] ; data[k] = data[r] ; data[r] = n ;
			}
		}

	m = i = l + 1 ; // i scans from left
	temp = key[k] ; key[k] = key[i] ; key[i] = temp ; // switch i and k (k is the median of the first, middle and last)
	n = data[k] ; data[k] = data[i] ; data[i] = n ;
	j = r ; // j scans from right
	pivot = key[i] ;
	while (i < j) {
		++i ;
		while (key[i] < pivot) i++ ;
		--j ;
		while (key[j] > pivot) j-- ;
		temp = key[i] ; key[i] = key[j] ; key[j] = temp ;
		n = data[i] ; data[i] = data[j] ; data[j] = n ;
		}
	temp = key[j] ; key[j] = key[m] ; key[m] = key[i] ; key[i] = temp ;
	n = data[j] ; data[j] = data[m] ; data[m] = data[i] ; data[i] = n ;

	m = j - 1 ;
	k = j + 1 ;
	// sort [l,m] and [k,r]
	if ((m - l) > (r - k)) { // sort smaller (right) side ([k,k]) first -> push [l,m]
		left[stack] = l ;
		right[stack++] = m ;
		l = k ;
		goto next_quicksort_call ;
		}
	else { // left side first, push [k,r]
		left[stack] = k ;
		right[stack++] = r ;
		r = m ;
		goto next_quicksort_call ;
		}

take_next_quicksort_interval:
	if (stack < 1) {
		return ;
		}

	l = left[--stack] ;
	r = right[stack] ;
	goto next_quicksort_call ;
}


void QuickSortDouble_Descending(double *input_key, unsigned long len, long *input_data, 
	// stack; passed in, to make this function thread-safe
	long left[32], long right[32])
{
	double temp, pivot ;
	long l, r, i, j, k, m, n, stack ;
	double *key ;
	long *data ;

	// qualification check
	if (len < 2) return ;

	// input array starts from 0, we want the key array to start from 1.
	key = input_key - 1 ;
	data = input_data - 1 ;

	l = 1 ;
	r = len ;
	stack = 0 ;
next_quicksort_call: ;

	i = r - l ;
	if (i < 1) { // at most one key -> take next interval
		goto take_next_quicksort_interval ;
		}
	else if (i == 1) { // two keys, sort directly
		if (key[l] < key[r]) {
			temp = key[l] ; key[l] = key[r] ; key[r] = temp ;
			n = data[l] ; data[l] = data[r] ; data[r] = n ;
			}
		goto take_next_quicksort_interval ;
		}
	else if (i == 2) { // three keys, sort directly
		k = l + 1 ;
		if (key[l] < key[k]) {
			if (key[r] < key[l]) { // switch l and k
				temp = key[k] ; key[k] = key[l] ; key[l] = temp ;
				n = data[k] ; data[k] = data[l] ; data[l] = n ;
				}
			// else : now left one is the smallest
			else if (key[k] > key[r]) { // rotate left				
				temp = key[l] ; key[l] = key[k] ; key[k] = key[r] ; key[r] = temp ;
				n = data[l] ; data[l] = data[k] ; data[k] = data[r] ; data[r] = n ;
				}
			else { // switch l and r (middle one is at least as small as r)
				temp = key[l] ; key[l] = key[r] ; key[r] = temp ;
				n = data[l] ; data[l] = data[r] ; data[r] = n ;
				}
			}
		else if (key[k] < key[r]) {
			if (key[l] < key[r]) { // rotate right 
				temp = key[r] ; key[r] = key[k] ; key[k] = key[l] ; key[l] = temp ;
				n = data[r] ; data[r] = data[k] ; data[k] = data[l] ; data[l] = n ;
				}
			else { // switch l+1 and r
				temp = key[k] ; key[k] = key[r] ; key[r] = temp ;
				n = data[k] ; data[k] = data[r] ; data[r] = n ;
				}
			}
		// else everything is fine
		goto take_next_quicksort_interval ;
		}

	// more than 3 elements left
	// ********** general QuickSort procedure **********
	// find median of the first, last and middle
	k = (l + r) >> 1 ; // k is the key in the middle
	if (key[l] < key[k]) {
		if (key[r] < key[l]) {
			temp = key[k] ; key[k] = key[l] ; key[l] = temp ;
			n = data[k] ; data[k] = data[l] ; data[l] = n ;
			}
		else if (key[k] > key[r]) {
			temp = key[l] ; key[l] = key[k] ; key[k] = key[r] ; key[r] = temp ;
			n = data[l] ; data[l] = data[k] ; data[k] = data[r] ; data[r] = n ;
			}
		else {
			temp = key[l] ; key[l] = key[r] ; key[r] = temp ;
			n = data[l] ; data[l] = data[r] ; data[r] = n ;
			}
		}
	else if (key[k] < key[r]) {
		if (key[l] < key[r]) {
			temp = key[r] ; key[r] = key[k] ; key[k] = key[l] ; key[l] = temp ;
			n = data[r] ; data[r] = data[k] ; data[k] = data[l] ; data[l] = n ;
			}
		else {
			temp = key[k] ; key[k] = key[r] ; key[r] = temp ;
			n = data[k] ; data[k] = data[r] ; data[r] = n ;
			}
		}

	m = i = l + 1 ; // i scans from left
	temp = key[k] ; key[k] = key[i] ; key[i] = temp ; // switch i and k (k is the median of the first, middle and last)
	n = data[k] ; data[k] = data[i] ; data[i] = n ;
	j = r ; // j scans from right
	pivot = key[i] ;
	while (i < j) {
		++i ;
		while (key[i] > pivot) i++ ;
		--j ;
		while (key[j] < pivot) j-- ;
		temp = key[i] ; key[i] = key[j] ; key[j] = temp ;
		n = data[i] ; data[i] = data[j] ; data[j] = n ;
		}
	temp = key[j] ; key[j] = key[m] ; key[m] = key[i] ; key[i] = temp ;
	n = data[j] ; data[j] = data[m] ; data[m] = data[i] ; data[i] = n ;

	m = j - 1 ;
	k = j + 1 ;
	// sort [l,m] and [k,r]
	if ((m - l) > (r - k)) { // sort smaller (right) side ([k,k]) first -> push [l,m]
		left[stack] = l ;
		right[stack++] = m ;
		l = k ;
		goto next_quicksort_call ;
		}
	else { // left side first, push [k,r]
		left[stack] = k ;
		right[stack++] = r ;
		r = m ;
		goto next_quicksort_call ;
		}

take_next_quicksort_interval:
	if (stack < 1) {
		return ;
		}

	l = left[--stack] ;
	r = right[stack] ;
	goto next_quicksort_call ;
}


/*
	These functions sort an array of long integers.
	DBM_QuickSort_long() also computes a permutation of input-data array 
	whereas DBM_QuickSort_long2() just sorts.
*/

int SortCheckLong(long *key, unsigned long len)
{
	unsigned long i ;

	for (i = 1 ; i < len ; i++) {
		if (key[i-1] > key[i]) return 0 ;
		}
	return 1 ;
}


void QuickSortShort(short *input_key, unsigned long len, long *input_data, 
	// stack; passed in, to make this function thread-safe
	long left[32], long right[32])
{
	short temp, pivot ;
	long l, r, i, j, k, m, n, stack ;
	short *key ;
	long *data ;

	// qualification check
	if (len < 2) return ;

	// input array starts from 0, we want the key array to start from 1.
	key = input_key - 1 ;
	data = input_data - 1 ;

	l = 1 ;
	r = len ;
	stack = 0 ;
next_quicksort_call: ;

	i = r - l ;
	if (i < 1) { // at most one key -> take next interval
		goto take_next_quicksort_interval ;
		}
	else if (i == 1) { // two keys, sort directly
		if (key[l] > key[r]) {
			temp = key[l] ; key[l] = key[r] ; key[r] = temp ;
			n = data[l] ; data[l] = data[r] ; data[r] = n ;
			}
		goto take_next_quicksort_interval ;
		}
	else if (i == 2) { // three keys, sort directly
		k = l + 1 ;
		if (key[l] > key[k]) {
			if (key[r] >= key[l]) { // switch l and k
				temp = key[k] ; key[k] = key[l] ; key[l] = temp ;
				n = data[k] ; data[k] = data[l] ; data[l] = n ;
				}
			// else : now left one is the largest
			else if (key[k] < key[r]) { // rotate left				
				temp = key[l] ; key[l] = key[k] ; key[k] = key[r] ; key[r] = temp ;
				n = data[l] ; data[l] = data[k] ; data[k] = data[r] ; data[r] = n ;
				}
			else { // switch l and r (middle one is at least as large as r)
				temp = key[l] ; key[l] = key[r] ; key[r] = temp ;
				n = data[l] ; data[l] = data[r] ; data[r] = n ;
				}
			}
		else if (key[k] > key[r]) {
			if (key[l] > key[r]) { // rotate right 
				temp = key[r] ; key[r] = key[k] ; key[k] = key[l] ; key[l] = temp ;
				n = data[r] ; data[r] = data[k] ; data[k] = data[l] ; data[l] = n ;
				}
			else { // switch l+1 and r
				temp = key[k] ; key[k] = key[r] ; key[r] = temp ;
				n = data[k] ; data[k] = data[r] ; data[r] = n ;
				}
			}
		// else everything is fine
		goto take_next_quicksort_interval ;
		}

	// more than 3 elements left
	// ********** general QuickSort procedure **********
	// find median of the first, last and middle
	k = (l + r) >> 1 ; // k is the key in the middle
	if (key[l] > key[k]) {
		if (key[r] >= key[l]) {
			temp = key[k] ; key[k] = key[l] ; key[l] = temp ;
			n = data[k] ; data[k] = data[l] ; data[l] = n ;
			}
		else if (key[k] < key[r]) {
			temp = key[l] ; key[l] = key[k] ; key[k] = key[r] ; key[r] = temp ;
			n = data[l] ; data[l] = data[k] ; data[k] = data[r] ; data[r] = n ;
			}
		else {
			temp = key[l] ; key[l] = key[r] ; key[r] = temp ;
			n = data[l] ; data[l] = data[r] ; data[r] = n ;
			}
		}
	else if (key[k] > key[r]) {
		if (key[l] > key[r]) {
			temp = key[r] ; key[r] = key[k] ; key[k] = key[l] ; key[l] = temp ;
			n = data[r] ; data[r] = data[k] ; data[k] = data[l] ; data[l] = n ;
			}
		else {
			temp = key[k] ; key[k] = key[r] ; key[r] = temp ;
			n = data[k] ; data[k] = data[r] ; data[r] = n ;
			}
		}

	m = i = l + 1 ; // i scans from left
	temp = key[k] ; key[k] = key[i] ; key[i] = temp ; // switch i and k (k is the median of the first, middle and last)
	n = data[k] ; data[k] = data[i] ; data[i] = n ;
	j = r ; // j scans from right
	pivot = key[i] ;
	while (i < j) {
		++i ;
		while (key[i] < pivot) i++ ;
		--j ;
		while (key[j] > pivot) j-- ;
		temp = key[i] ; key[i] = key[j] ; key[j] = temp ;
		n = data[i] ; data[i] = data[j] ; data[j] = n ;
		}
	temp = key[j] ; key[j] = key[m] ; key[m] = key[i] ; key[i] = temp ;
	n = data[j] ; data[j] = data[m] ; data[m] = data[i] ; data[i] = n ;

	m = j - 1 ;
	k = j + 1 ;
	// sort [l,m] and [k,r]
	if ((m - l) > (r - k)) { // sort smaller (right) side ([k,k]) first -> push [l,m]
		left[stack] = l ;
		right[stack++] = m ;
		l = k ;
		goto next_quicksort_call ;
		}
	else { // left side first, push [k,r]
		left[stack] = k ;
		right[stack++] = r ;
		r = m ;
		goto next_quicksort_call ;
		}

take_next_quicksort_interval:
	if (stack < 1) {
		return ;
		}

	l = left[--stack] ;
	r = right[stack] ;
	goto next_quicksort_call ;
}


void QuickSortLong(long *input_key, unsigned long len, long *input_data, 
	// stack; passed in, to make this function thread-safe
	long left[32], long right[32])
{
	long l, r, i, j, k, m, stack, pivot ;
	long n ;
	long *key ;
	long *data ;

	// qualification check
	if (len < 2) return ;

	// input array starts from 0, we want the key array to start from 1.
	key = input_key - 1 ;
	data = input_data - 1 ;

	l = 1 ;
	r = len ;
	stack = 0 ;
next_quicksort_call: ;

	i = r - l ;
	if (i < 1) { // at most one key -> take next interval
		goto take_next_quicksort_interval ;
		}
	else if (i == 1) { // two keys, sort directly
		if (key[l] > key[r]) {
			j = key[l] ; key[l] = key[r] ; key[r] = j ;
			n = data[l] ; data[l] = data[r] ; data[r] = n ;
			}
		goto take_next_quicksort_interval ;
		}
	else if (i == 2) { // three keys, sort directly
		k = l + 1 ;
		if (key[l] > key[k]) {
			if (key[r] >= key[l]) { // switch l and k
				j = key[k] ; key[k] = key[l] ; key[l] = j ;
				n = data[k] ; data[k] = data[l] ; data[l] = n ;
				}
			// else : now left one is the largest
			else if (key[k] < key[r]) { // rotate left				
				j = key[l] ; key[l] = key[k] ; key[k] = key[r] ; key[r] = j ;
				n = data[l] ; data[l] = data[k] ; data[k] = data[r] ; data[r] = n ;
				}
			else { // switch l and r (middle one is at least as large as r)
				j = key[l] ; key[l] = key[r] ; key[r] = j ;
				n = data[l] ; data[l] = data[r] ; data[r] = n ;
				}
			}
		else if (key[k] > key[r]) {
			if (key[l] > key[r]) { // rotate right 
				j = key[r] ; key[r] = key[k] ; key[k] = key[l] ; key[l] = j ;
				n = data[r] ; data[r] = data[k] ; data[k] = data[l] ; data[l] = n ;
				}
			else { // switch l+1 and r
				j = key[k] ; key[k] = key[r] ; key[r] = j ;
				n = data[k] ; data[k] = data[r] ; data[r] = n ;
				}
			}
		// else everything is fine
		goto take_next_quicksort_interval ;
		}

	// more than 3 elements left
	// ********** general QuickSort procedure **********
	// find median of the first, last and middle
	k = (l + r) >> 1 ;
	if (key[l] > key[k]) {
		if (key[r] >= key[l]) {
			j = key[k] ; key[k] = key[l] ; key[l] = j ;
			n = data[k] ; data[k] = data[l] ; data[l] = n ;
			}
		else if (key[k] < key[r]) {
			j = key[l] ; key[l] = key[k] ; key[k] = key[r] ; key[r] = j ;
			n = data[l] ; data[l] = data[k] ; data[k] = data[r] ; data[r] = n ;
			}
		else {
			j = key[l] ; key[l] = key[r] ; key[r] = j ;
			n = data[l] ; data[l] = data[r] ; data[r] = n ;
			}
		}
	else if (key[k] > key[r]) {
		if (key[l] > key[r]) {
			j = key[r] ; key[r] = key[k] ; key[k] = key[l] ; key[l] = j ;
			n = data[r] ; data[r] = data[k] ; data[k] = data[l] ; data[l] = n ;
			}
		else {
			j = key[k] ; key[k] = key[r] ; key[r] = j ;
			n = data[k] ; data[k] = data[r] ; data[r] = n ;
			}
		}

	m = i = l + 1 ; // i scans from left
	j = key[k] ; key[k] = key[i] ; key[i] = j ; // switch i and k (k is the median of the first, middle and last)
	n = data[k] ; data[k] = data[i] ; data[i] = n ;
	j = r ; // j scans from right
	pivot = key[i] ;
	while (i < j) {
		++i ;
		while (key[i] < pivot) i++ ;
		--j ;
		while (key[j] > pivot) j-- ;
		k = key[i] ; key[i] = key[j] ; key[j] = k ;
		n = data[i] ; data[i] = data[j] ; data[j] = n ;
		}
	k = key[j] ; key[j] = key[m] ; key[m] = key[i] ; key[i] = k ;
	n = data[j] ; data[j] = data[m] ; data[m] = data[i] ; data[i] = n ;

	m = j - 1 ;
	k = j + 1 ;
	// sort [l,m] and [k,r]
	if ((m - l) > (r - k)) { // sort smaller (right) side ([k,k]) first -> push [l,m]
		left[stack] = l ;
		right[stack++] = m ;
		l = k ;
		goto next_quicksort_call ;
		}
	else { // left side first, push [k,r]
		left[stack] = k ;
		right[stack++] = r ;
		r = m ;
		goto next_quicksort_call ;
		}

take_next_quicksort_interval:
	if (stack < 1) {
		return ;
		}

	l = left[--stack] ;
	r = right[stack] ;
	goto next_quicksort_call ;
}


void QuickSortLong_Descending(long *input_key, unsigned long len, long *input_data, 
	// stack; passed in, to make this function thread-safe
	long left[32], long right[32])
{
	long l, r, i, j, k, m, stack, pivot ;
	long n ;
	long *key ;
	long *data ;

	// qualification check
	if (len < 2) return ;

	// input array starts from 0, we want the key array to start from 1.
	key = input_key - 1 ;
	data = input_data - 1 ;

	l = 1 ;
	r = len ;
	stack = 0 ;
next_quicksort_call: ;

	i = r - l ;
	if (i < 1) { // at most one key -> take next interval
		goto take_next_quicksort_interval ;
		}
	else if (i == 1) { // two keys, sort directly
		if (key[l] < key[r]) {
			j = key[l] ; key[l] = key[r] ; key[r] = j ;
			n = data[l] ; data[l] = data[r] ; data[r] = n ;
			}
		goto take_next_quicksort_interval ;
		}
	else if (i == 2) { // three keys, sort directly
		k = l + 1 ;
		if (key[l] < key[k]) {
			if (key[r] < key[l]) { // switch l and k
				j = key[k] ; key[k] = key[l] ; key[l] = j ;
				n = data[k] ; data[k] = data[l] ; data[l] = n ;
				}
			// else : now left one is the smallest
			else if (key[k] > key[r]) { // rotate left				
				j = key[l] ; key[l] = key[k] ; key[k] = key[r] ; key[r] = j ;
				n = data[l] ; data[l] = data[k] ; data[k] = data[r] ; data[r] = n ;
				}
			else { // switch l and r (middle one is at least as small as r)
				j = key[l] ; key[l] = key[r] ; key[r] = j ;
				n = data[l] ; data[l] = data[r] ; data[r] = n ;
				}
			}
		else if (key[k] < key[r]) {
			if (key[l] < key[r]) { // rotate right 
				j = key[r] ; key[r] = key[k] ; key[k] = key[l] ; key[l] = j ;
				n = data[r] ; data[r] = data[k] ; data[k] = data[l] ; data[l] = n ;
				}
			else { // switch l+1 and r
				j = key[k] ; key[k] = key[r] ; key[r] = j ;
				n = data[k] ; data[k] = data[r] ; data[r] = n ;
				}
			}
		// else everything is fine
		goto take_next_quicksort_interval ;
		}

	// more than 3 elements left
	// ********** general QuickSort procedure **********
	// find median of the first, last and middle
	k = (l + r) >> 1 ; // k is the key in the middle
	if (key[l] < key[k]) {
		if (key[r] < key[l]) {
			j = key[k] ; key[k] = key[l] ; key[l] = j ;
			n = data[k] ; data[k] = data[l] ; data[l] = n ;
			}
		else if (key[k] > key[r]) {
			j = key[l] ; key[l] = key[k] ; key[k] = key[r] ; key[r] = j ;
			n = data[l] ; data[l] = data[k] ; data[k] = data[r] ; data[r] = n ;
			}
		else {
			j = key[l] ; key[l] = key[r] ; key[r] = j ;
			n = data[l] ; data[l] = data[r] ; data[r] = n ;
			}
		}
	else if (key[k] < key[r]) {
		if (key[l] < key[r]) {
			j = key[r] ; key[r] = key[k] ; key[k] = key[l] ; key[l] = j ;
			n = data[r] ; data[r] = data[k] ; data[k] = data[l] ; data[l] = n ;
			}
		else {
			j = key[k] ; key[k] = key[r] ; key[r] = j ;
			n = data[k] ; data[k] = data[r] ; data[r] = n ;
			}
		}

	m = i = l + 1 ; // i scans from left
	j = key[k] ; key[k] = key[i] ; key[i] = j ; // switch i and k (k is the median of the first, middle and last)
	n = data[k] ; data[k] = data[i] ; data[i] = n ;
	j = r ; // j scans from right
	pivot = key[i] ;
	while (i < j) {
		++i ;
		while (key[i] > pivot) i++ ;
		--j ;
		while (key[j] < pivot) j-- ;
		k = key[i] ; key[i] = key[j] ; key[j] = k ;
		n = data[i] ; data[i] = data[j] ; data[j] = n ;
		}
	k = key[j] ; key[j] = key[m] ; key[m] = key[i] ; key[i] = k ;
	n = data[j] ; data[j] = data[m] ; data[m] = data[i] ; data[i] = n ;

	m = j - 1 ;
	k = j + 1 ;
	// sort [l,m] and [k,r]
	if ((m - l) > (r - k)) { // sort smaller (right) side ([k,k]) first -> push [l,m]
		left[stack] = l ;
		right[stack++] = m ;
		l = k ;
		goto next_quicksort_call ;
		}
	else { // left side first, push [k,r]
		left[stack] = k ;
		right[stack++] = r ;
		r = m ;
		goto next_quicksort_call ;
		}

take_next_quicksort_interval:
	if (stack < 1) {
		return ;
		}

	l = left[--stack] ;
	r = right[stack] ;
	goto next_quicksort_call ;
}


void QuickSortLong2(long *input_key, unsigned long len, 
	// stack; passed in, to make this function thread-safe
	long left[32], long right[32])
{
	long l, r, i, j, k, m, stack, pivot ;
	long *key ;

	// qualification check
	if (len < 2) return ;

	// input array starts from 0, we want the key array to start from 1.
	key = input_key - 1 ;

	l = 1 ;
	r = len ;
	stack = 0 ;
next_quicksort_call: ;

	i = r - l ;
	if (i < 1) { // at most one key -> take next interval
		goto take_next_quicksort_interval ;
		}
	else if (i == 1) { // two keys, sort directly
		if (key[l] > key[r]) {
			j = key[l] ; key[l] = key[r] ; key[r] = j ;
			}
		goto take_next_quicksort_interval ;
		}
	else if (i == 2) { // three keys, sort directly
		k = l + 1 ;
		if (key[l] > key[k]) {
			if (key[r] >= key[l]) { // switch l and k
				j = key[k] ; key[k] = key[l] ; key[l] = j ;
				}
			// else : now left one is the largest
			else if (key[k] < key[r]) { // rotate left				
				j = key[l] ; key[l] = key[k] ; key[k] = key[r] ; key[r] = j ;
				}
			else { // switch l and r (middle one is at least as large as r)
				j = key[l] ; key[l] = key[r] ; key[r] = j ;
				}
			}
		else if (key[k] > key[r]) {
			if (key[l] > key[r]) { // rotate right 
				j = key[r] ; key[r] = key[k] ; key[k] = key[l] ; key[l] = j ;
				}
			else { // switch l+1 and r
				j = key[k] ; key[k] = key[r] ; key[r] = j ;
				}
			}
		// else everything is fine
		goto take_next_quicksort_interval ;
		}

	// more than 3 elements left
	// ********** general QuickSort procedure **********
	// find median of the first, last and middle
	k = (l + r) >> 1 ;
	if (key[l] > key[k]) {
		if (key[r] >= key[l]) {
			j = key[k] ; key[k] = key[l] ; key[l] = j ;
			}
		else if (key[k] < key[r]) {
			j = key[l] ; key[l] = key[k] ; key[k] = key[r] ; key[r] = j ;
			}
		else {
			j = key[l] ; key[l] = key[r] ; key[r] = j ;
			}
		}
	else if (key[k] > key[r]) {
		if (key[l] > key[r]) {
			j = key[r] ; key[r] = key[k] ; key[k] = key[l] ; key[l] = j ;
			}
		else {
			j = key[k] ; key[k] = key[r] ; key[r] = j ;
			}
		}

	m = i = l + 1 ; // i scans from left
	j = key[k] ; key[k] = key[i] ; key[i] = j ; // switch i and k (k is the median of the first, middle and last)
	j = r ; // j scans from right
	pivot = key[i] ;
	while (i < j) {
		++i ;
		while (key[i] < pivot) i++ ;
		--j ;
		while (key[j] > pivot) j-- ;
		k = key[i] ; key[i] = key[j] ; key[j] = k ;
		}
	k = key[j] ; key[j] = key[m] ; key[m] = key[i] ; key[i] = k ;

	m = j - 1 ;
	k = j + 1 ;
	// sort [l,m] and [k,r]
	if ((m - l) > (r - k)) { // sort smaller (right) side ([k,k]) first -> push [l,m]
		left[stack] = l ;
		right[stack++] = m ;
		l = k ;
		goto next_quicksort_call ;
		}
	else { // left side first, push [k,r]
		left[stack] = k ;
		right[stack++] = r ;
		r = m ;
		goto next_quicksort_call ;
		}

take_next_quicksort_interval:
	if (stack < 1) {
		return ;
		}

	l = left[--stack] ;
	r = right[stack] ;
	goto next_quicksort_call ;
}


void QuickSortWchar_t(const wchar_t **input_key, unsigned long len, long *input_data, 
	// stack; passed in, to make this function thread-safe
	long left[32], long right[32])
{
	const wchar_t *temp, *pivot ;
	long l, r, i, j, k, m, n, stack ;
	const wchar_t **key ;
	long *data ;

	// qualification check
	if (len < 2) return ;

	// input array starts from 0, we want the key array to start from 1.
	key = input_key - 1 ;
	data = input_data - 1 ;

	l = 1 ;
	r = len ;
	stack = 0 ;
next_quicksort_call: ;

	i = r - l ;
	if (i < 1) { // at most one key -> take next interval
		goto take_next_quicksort_interval ;
		}
	else if (i == 1) { // two keys, sort directly
		if (wcscmp(key[l], key[r]) > 0) {
			temp = key[l] ; key[l] = key[r] ; key[r] = temp ;
			n = data[l] ; data[l] = data[r] ; data[r] = n ;
			}
		goto take_next_quicksort_interval ;
		}
	else if (i == 2) { // three keys, sort directly
		k = l + 1 ;
		if (wcscmp(key[l], key[k]) > 0) {
			if (wcscmp(key[r], key[l]) >= 0) { // switch l and k
				temp = key[k] ; key[k] = key[l] ; key[l] = temp ;
				n = data[k] ; data[k] = data[l] ; data[l] = n ;
				}
			// else : now left one is the largest
			else if (wcscmp(key[k], key[r]) < 0) { // rotate left				
				temp = key[l] ; key[l] = key[k] ; key[k] = key[r] ; key[r] = temp ;
				n = data[l] ; data[l] = data[k] ; data[k] = data[r] ; data[r] = n ;
				}
			else { // switch l and r (middle one is at least as large as r)
				temp = key[l] ; key[l] = key[r] ; key[r] = temp ;
				n = data[l] ; data[l] = data[r] ; data[r] = n ;
				}
			}
		else if (wcscmp(key[k], key[r]) > 0) {
			if (wcscmp(key[l], key[r]) > 0) { // rotate right 
				temp = key[r] ; key[r] = key[k] ; key[k] = key[l] ; key[l] = temp ;
				n = data[r] ; data[r] = data[k] ; data[k] = data[l] ; data[l] = n ;
				}
			else { // switch l+1 and r
				temp = key[k] ; key[k] = key[r] ; key[r] = temp ;
				n = data[k] ; data[k] = data[r] ; data[r] = n ;
				}
			}
		// else everything is fine
		goto take_next_quicksort_interval ;
		}

	// more than 3 elements left
	// ********** general QuickSort procedure **********
	// find median of the first, last and middle
	k = (l + r) >> 1 ; // k is the key in the middle
	if (wcscmp(key[l], key[k]) > 0) {
		if (wcscmp(key[r], key[l]) >= 0) {
			temp = key[k] ; key[k] = key[l] ; key[l] = temp ;
			n = data[k] ; data[k] = data[l] ; data[l] = n ;
			}
		else if (wcscmp(key[k], key[r]) < 0) {
			temp = key[l] ; key[l] = key[k] ; key[k] = key[r] ; key[r] = temp ;
			n = data[l] ; data[l] = data[k] ; data[k] = data[r] ; data[r] = n ;
			}
		else {
			temp = key[l] ; key[l] = key[r] ; key[r] = temp ;
			n = data[l] ; data[l] = data[r] ; data[r] = n ;
			}
		}
	else if (wcscmp(key[k], key[r]) > 0) {
		if (wcscmp(key[l], key[r]) > 0) {
			temp = key[r] ; key[r] = key[k] ; key[k] = key[l] ; key[l] = temp ;
			n = data[r] ; data[r] = data[k] ; data[k] = data[l] ; data[l] = n ;
			}
		else {
			temp = key[k] ; key[k] = key[r] ; key[r] = temp ;
			n = data[k] ; data[k] = data[r] ; data[r] = n ;
			}
		}

	m = i = l + 1 ; // i scans from left
	temp = key[k] ; key[k] = key[i] ; key[i] = temp ; // switch i and k (k is the median of the first, middle and last)
	n = data[k] ; data[k] = data[i] ; data[i] = n ;
	j = r ; // j scans from right
	pivot = key[i] ;
	while (i < j) {
		++i ;
		while (wcscmp(key[i], pivot) < 0) i++ ;
		--j ;
		while (wcscmp(key[j], pivot) > 0) j-- ;
		temp = key[i] ; key[i] = key[j] ; key[j] = temp ;
		n = data[i] ; data[i] = data[j] ; data[j] = n ;
		}
	temp = key[j] ; key[j] = key[m] ; key[m] = key[i] ; key[i] = temp ;
	n = data[j] ; data[j] = data[m] ; data[m] = data[i] ; data[i] = n ;

	m = j - 1 ;
	k = j + 1 ;
	// sort [l,m] and [k,r]
	if ((m - l) > (r - k)) { // sort smaller (right) side ([k,k]) first -> push [l,m]
		left[stack] = l ;
		right[stack++] = m ;
		l = k ;
		goto next_quicksort_call ;
		}
	else { // left side first, push [k,r]
		left[stack] = k ;
		right[stack++] = r ;
		r = m ;
		goto next_quicksort_call ;
		}

take_next_quicksort_interval:
	if (stack < 1) {
		return ;
		}

	l = left[--stack] ;
	r = right[stack] ;
	goto next_quicksort_call ;
}


void QuickSortWchar_t(const wchar_t **input_key, unsigned long len, 
	// stack; passed in, to make this function thread-safe
	long left[32], long right[32])
{
	const wchar_t *temp, *pivot ;
	long l, r, i, j, k, m, stack ;
	const wchar_t **key ;

	// qualification check
	if (len < 2) return ;

	// input array starts from 0, we want the key array to start from 1.
	key = input_key - 1 ;

	l = 1 ;
	r = len ;
	stack = 0 ;
next_quicksort_call: ;

	i = r - l ;
	if (i < 1) { // at most one key -> take next interval
		goto take_next_quicksort_interval ;
		}
	else if (i == 1) { // two keys, sort directly
		if (wcscmp(key[l], key[r]) > 0) {
			temp = key[l] ; key[l] = key[r] ; key[r] = temp ;
			}
		goto take_next_quicksort_interval ;
		}
	else if (i == 2) { // three keys, sort directly
		k = l + 1 ;
		if (wcscmp(key[l], key[k]) > 0) {
			if (wcscmp(key[r], key[l]) >= 0) { // switch l and k
				temp = key[k] ; key[k] = key[l] ; key[l] = temp ;
				}
			// else : now left one is the largest
			else if (wcscmp(key[k], key[r]) < 0) { // rotate left				
				temp = key[l] ; key[l] = key[k] ; key[k] = key[r] ; key[r] = temp ;
				}
			else { // switch l and r (middle one is at least as large as r)
				temp = key[l] ; key[l] = key[r] ; key[r] = temp ;
				}
			}
		else if (wcscmp(key[k], key[r]) > 0) {
			if (wcscmp(key[l], key[r]) > 0) { // rotate right 
				temp = key[r] ; key[r] = key[k] ; key[k] = key[l] ; key[l] = temp ;
				}
			else { // switch l+1 and r
				temp = key[k] ; key[k] = key[r] ; key[r] = temp ;
				}
			}
		// else everything is fine
		goto take_next_quicksort_interval ;
		}

	// more than 3 elements left
	// ********** general QuickSort procedure **********
	// find median of the first, last and middle
	k = (l + r) >> 1 ; // k is the key in the middle
	if (wcscmp(key[l], key[k]) > 0) {
		if (wcscmp(key[r], key[l]) >= 0) {
			temp = key[k] ; key[k] = key[l] ; key[l] = temp ;
			}
		else if (wcscmp(key[k], key[r]) < 0) {
			temp = key[l] ; key[l] = key[k] ; key[k] = key[r] ; key[r] = temp ;
			}
		else {
			temp = key[l] ; key[l] = key[r] ; key[r] = temp ;
			}
		}
	else if (wcscmp(key[k], key[r]) > 0) {
		if (wcscmp(key[l], key[r]) > 0) {
			temp = key[r] ; key[r] = key[k] ; key[k] = key[l] ; key[l] = temp ;
			}
		else {
			temp = key[k] ; key[k] = key[r] ; key[r] = temp ;
			}
		}

	m = i = l + 1 ; // i scans from left
	temp = key[k] ; key[k] = key[i] ; key[i] = temp ; // switch i and k (k is the median of the first, middle and last)
	j = r ; // j scans from right
	pivot = key[i] ;
	while (i < j) {
		++i ;
		while (wcscmp(key[i], pivot) < 0) i++ ;
		--j ;
		while (wcscmp(key[j], pivot) > 0) j-- ;
		temp = key[i] ; key[i] = key[j] ; key[j] = temp ;
		}
	temp = key[j] ; key[j] = key[m] ; key[m] = key[i] ; key[i] = temp ;

	m = j - 1 ;
	k = j + 1 ;
	// sort [l,m] and [k,r]
	if ((m - l) > (r - k)) { // sort smaller (right) side ([k,k]) first -> push [l,m]
		left[stack] = l ;
		right[stack++] = m ;
		l = k ;
		goto next_quicksort_call ;
		}
	else { // left side first, push [k,r]
		left[stack] = k ;
		right[stack++] = r ;
		r = m ;
		goto next_quicksort_call ;
		}

take_next_quicksort_interval:
	if (stack < 1) {
		return ;
		}

	l = left[--stack] ;
	r = right[stack] ;
	goto next_quicksort_call ;
}


void QuickSortWchar_t_Descending(const wchar_t **input_key, unsigned long len, long *input_data, 
	// stack; passed in, to make this function thread-safe
	long left[32], long right[32])
{
	const wchar_t *temp, *pivot ;
	long l, r, i, j, k, m, n, stack ;
	const wchar_t **key ;
	long *data ;

	// qualification check
	if (len < 2) return ;

	// input array starts from 0, we want the key array to start from 1.
	key = input_key - 1 ;
	data = input_data - 1 ;

	l = 1 ;
	r = len ;
	stack = 0 ;
next_quicksort_call: ;

	i = r - l ;
	if (i < 1) { // at most one key -> take next interval
		goto take_next_quicksort_interval ;
		}
	else if (i == 1) { // two keys, sort directly
		if (wcscmp(key[l], key[r]) < 0) {
			temp = key[l] ; key[l] = key[r] ; key[r] = temp ;
			n = data[l] ; data[l] = data[r] ; data[r] = n ;
			}
		goto take_next_quicksort_interval ;
		}
	else if (i == 2) { // three keys, sort directly
		k = l + 1 ;
		if (wcscmp(key[l], key[k]) < 0) {
			if (wcscmp(key[r], key[l]) < 0) { // switch l and k
				temp = key[k] ; key[k] = key[l] ; key[l] = temp ;
				n = data[k] ; data[k] = data[l] ; data[l] = n ;
				}
			// else : now left one is the smallest
			else if (wcscmp(key[k], key[r]) > 0) { // rotate left				
				temp = key[l] ; key[l] = key[k] ; key[k] = key[r] ; key[r] = temp ;
				n = data[l] ; data[l] = data[k] ; data[k] = data[r] ; data[r] = n ;
				}
			else { // switch l and r (middle one is at least as small as r)
				temp = key[l] ; key[l] = key[r] ; key[r] = temp ;
				n = data[l] ; data[l] = data[r] ; data[r] = n ;
				}
			}
		else if (wcscmp(key[k], key[r]) < 0) {
			if (wcscmp(key[l], key[r]) < 0) { // rotate right 
				temp = key[r] ; key[r] = key[k] ; key[k] = key[l] ; key[l] = temp ;
				n = data[r] ; data[r] = data[k] ; data[k] = data[l] ; data[l] = n ;
				}
			else { // switch l+1 and r
				temp = key[k] ; key[k] = key[r] ; key[r] = temp ;
				n = data[k] ; data[k] = data[r] ; data[r] = n ;
				}
			}
		// else everything is fine
		goto take_next_quicksort_interval ;
		}

	// more than 3 elements left
	// ********** general QuickSort procedure **********
	// find median of the first, last and middle
	k = (l + r) >> 1 ; // k is the key in the middle
	if (wcscmp(key[l], key[k]) < 0) {
		if (wcscmp(key[r], key[l]) < 0) {
			temp = key[k] ; key[k] = key[l] ; key[l] = temp ;
			n = data[k] ; data[k] = data[l] ; data[l] = n ;
			}
		else if (wcscmp(key[k], key[r]) > 0) {
			temp = key[l] ; key[l] = key[k] ; key[k] = key[r] ; key[r] = temp ;
			n = data[l] ; data[l] = data[k] ; data[k] = data[r] ; data[r] = n ;
			}
		else {
			temp = key[l] ; key[l] = key[r] ; key[r] = temp ;
			n = data[l] ; data[l] = data[r] ; data[r] = n ;
			}
		}
	else if (wcscmp(key[k], key[r]) < 0) {
		if (wcscmp(key[l], key[r]) < 0) {
			temp = key[r] ; key[r] = key[k] ; key[k] = key[l] ; key[l] = temp ;
			n = data[r] ; data[r] = data[k] ; data[k] = data[l] ; data[l] = n ;
			}
		else {
			temp = key[k] ; key[k] = key[r] ; key[r] = temp ;
			n = data[k] ; data[k] = data[r] ; data[r] = n ;
			}
		}

	m = i = l + 1 ; // i scans from left
	temp = key[k] ; key[k] = key[i] ; key[i] = temp ; // switch i and k (k is the median of the first, middle and last)
	n = data[k] ; data[k] = data[i] ; data[i] = n ;
	j = r ; // j scans from right
	pivot = key[i] ;
	while (i < j) {
		++i ;
		while (wcscmp(key[i], pivot) > 0) i++ ;
		--j ;
		while (wcscmp(key[j], pivot) < 0) j-- ;
		temp = key[i] ; key[i] = key[j] ; key[j] = temp ;
		n = data[i] ; data[i] = data[j] ; data[j] = n ;
		}
	temp = key[j] ; key[j] = key[m] ; key[m] = key[i] ; key[i] = temp ;
	n = data[j] ; data[j] = data[m] ; data[m] = data[i] ; data[i] = n ;

	m = j - 1 ;
	k = j + 1 ;
	// sort [l,m] and [k,r]
	if ((m - l) > (r - k)) { // sort smaller (right) side ([k,k]) first -> push [l,m]
		left[stack] = l ;
		right[stack++] = m ;
		l = k ;
		goto next_quicksort_call ;
		}
	else { // left side first, push [k,r]
		left[stack] = k ;
		right[stack++] = r ;
		r = m ;
		goto next_quicksort_call ;
		}

take_next_quicksort_interval:
	if (stack < 1) {
		return ;
		}

	l = left[--stack] ;
	r = right[stack] ;
	goto next_quicksort_call ;
}


void QuickSortChar(const char **input_key, unsigned long len, long *input_data, 
	// stack; passed in, to make this function thread-safe
	long left[32], long right[32])
{
	const char *temp, *pivot ;
	long l, r, i, j, k, m, n, stack ;
	const char **key ;
	long *data ;

	// qualification check
	if (len < 2) return ;

	// input array starts from 0, we want the key array to start from 1.
	key = input_key - 1 ;
	data = input_data - 1 ;

	l = 1 ;
	r = len ;
	stack = 0 ;
next_quicksort_call: ;

	i = r - l ;
	if (i < 1) { // at most one key -> take next interval
		goto take_next_quicksort_interval ;
		}
	else if (i == 1) { // two keys, sort directly
		if (strcmp(key[l], key[r]) > 0) {
			temp = key[l] ; key[l] = key[r] ; key[r] = temp ;
			n = data[l] ; data[l] = data[r] ; data[r] = n ;
			}
		goto take_next_quicksort_interval ;
		}
	else if (i == 2) { // three keys, sort directly
		k = l + 1 ;
		if (strcmp(key[l], key[k]) > 0) {
			if (strcmp(key[r], key[l]) >= 0) { // switch l and k
				temp = key[k] ; key[k] = key[l] ; key[l] = temp ;
				n = data[k] ; data[k] = data[l] ; data[l] = n ;
				}
			// else : now left one is the largest
			else if (strcmp(key[k], key[r]) < 0) { // rotate left				
				temp = key[l] ; key[l] = key[k] ; key[k] = key[r] ; key[r] = temp ;
				n = data[l] ; data[l] = data[k] ; data[k] = data[r] ; data[r] = n ;
				}
			else { // switch l and r (middle one is at least as large as r)
				temp = key[l] ; key[l] = key[r] ; key[r] = temp ;
				n = data[l] ; data[l] = data[r] ; data[r] = n ;
				}
			}
		else if (strcmp(key[k], key[r]) > 0) {
			if (strcmp(key[l], key[r]) > 0) { // rotate right 
				temp = key[r] ; key[r] = key[k] ; key[k] = key[l] ; key[l] = temp ;
				n = data[r] ; data[r] = data[k] ; data[k] = data[l] ; data[l] = n ;
				}
			else { // switch l+1 and r
				temp = key[k] ; key[k] = key[r] ; key[r] = temp ;
				n = data[k] ; data[k] = data[r] ; data[r] = n ;
				}
			}
		// else everything is fine
		goto take_next_quicksort_interval ;
		}

	// more than 3 elements left
	// ********** general QuickSort procedure **********
	// find median of the first, last and middle
	k = (l + r) >> 1 ; // k is the key in the middle
	if (strcmp(key[l], key[k]) > 0) {
		if (strcmp(key[r], key[l]) >= 0) {
			temp = key[k] ; key[k] = key[l] ; key[l] = temp ;
			n = data[k] ; data[k] = data[l] ; data[l] = n ;
			}
		else if (strcmp(key[k], key[r]) < 0) {
			temp = key[l] ; key[l] = key[k] ; key[k] = key[r] ; key[r] = temp ;
			n = data[l] ; data[l] = data[k] ; data[k] = data[r] ; data[r] = n ;
			}
		else {
			temp = key[l] ; key[l] = key[r] ; key[r] = temp ;
			n = data[l] ; data[l] = data[r] ; data[r] = n ;
			}
		}
	else if (strcmp(key[k], key[r]) > 0) {
		if (strcmp(key[l], key[r]) > 0) {
			temp = key[r] ; key[r] = key[k] ; key[k] = key[l] ; key[l] = temp ;
			n = data[r] ; data[r] = data[k] ; data[k] = data[l] ; data[l] = n ;
			}
		else {
			temp = key[k] ; key[k] = key[r] ; key[r] = temp ;
			n = data[k] ; data[k] = data[r] ; data[r] = n ;
			}
		}

	m = i = l + 1 ; // i scans from left
	temp = key[k] ; key[k] = key[i] ; key[i] = temp ; // switch i and k (k is the median of the first, middle and last)
	n = data[k] ; data[k] = data[i] ; data[i] = n ;
	j = r ; // j scans from right
	pivot = key[i] ;
	while (i < j) {
		++i ;
		while (strcmp(key[i], pivot) < 0) i++ ;
		--j ;
		while (strcmp(key[j], pivot) > 0) j-- ;
		temp = key[i] ; key[i] = key[j] ; key[j] = temp ;
		n = data[i] ; data[i] = data[j] ; data[j] = n ;
		}
	temp = key[j] ; key[j] = key[m] ; key[m] = key[i] ; key[i] = temp ;
	n = data[j] ; data[j] = data[m] ; data[m] = data[i] ; data[i] = n ;

	m = j - 1 ;
	k = j + 1 ;
	// sort [l,m] and [k,r]
	if ((m - l) > (r - k)) { // sort smaller (right) side ([k,k]) first -> push [l,m]
		left[stack] = l ;
		right[stack++] = m ;
		l = k ;
		goto next_quicksort_call ;
		}
	else { // left side first, push [k,r]
		left[stack] = k ;
		right[stack++] = r ;
		r = m ;
		goto next_quicksort_call ;
		}

take_next_quicksort_interval:
	if (stack < 1) {
		return ;
		}

	l = left[--stack] ;
	r = right[stack] ;
	goto next_quicksort_call ;
}


void QuickSortMem(const char **input_key, unsigned long keysize, unsigned long len, long *input_data, 
	// stack; passed in, to make this function thread-safe
	long left[32], long right[32])
{
	const char *temp, *pivot ;
	long l, r, i, j, k, m, n, stack ;
	const char **key ;
	long *data ;

	// qualification check
	if (len < 2) return ;

	// input array starts from 0, we want the key array to start from 1.
	key = input_key - 1 ;
	data = input_data - 1 ;

	l = 1 ;
	r = len ;
	stack = 0 ;
next_quicksort_call: ;

	i = r - l ;
	if (i < 1) { // at most one key -> take next interval
		goto take_next_quicksort_interval ;
		}
	else if (i == 1) { // two keys, sort directly
		if (memcmp(key[l], key[r], keysize) > 0) {
			temp = key[l] ; key[l] = key[r] ; key[r] = temp ;
			n = data[l] ; data[l] = data[r] ; data[r] = n ;
			}
		goto take_next_quicksort_interval ;
		}
	else if (i == 2) { // three keys, sort directly
		k = l + 1 ;
		if (memcmp(key[l], key[k], keysize) > 0) {
			if (memcmp(key[r], key[l], keysize) >= 0) { // switch l and k
				temp = key[k] ; key[k] = key[l] ; key[l] = temp ;
				n = data[k] ; data[k] = data[l] ; data[l] = n ;
				}
			// else : now left one is the largest
			else if (memcmp(key[k], key[r], keysize) < 0) { // rotate left				
				temp = key[l] ; key[l] = key[k] ; key[k] = key[r] ; key[r] = temp ;
				n = data[l] ; data[l] = data[k] ; data[k] = data[r] ; data[r] = n ;
				}
			else { // switch l and r (middle one is at least as large as r)
				temp = key[l] ; key[l] = key[r] ; key[r] = temp ;
				n = data[l] ; data[l] = data[r] ; data[r] = n ;
				}
			}
		else if (memcmp(key[k], key[r], keysize) > 0) {
			if (memcmp(key[l], key[r], keysize) > 0) { // rotate right 
				temp = key[r] ; key[r] = key[k] ; key[k] = key[l] ; key[l] = temp ;
				n = data[r] ; data[r] = data[k] ; data[k] = data[l] ; data[l] = n ;
				}
			else { // switch l+1 and r
				temp = key[k] ; key[k] = key[r] ; key[r] = temp ;
				n = data[k] ; data[k] = data[r] ; data[r] = n ;
				}
			}
		// else everything is fine
		goto take_next_quicksort_interval ;
		}

	// more than 3 elements left
	// ********** general QuickSort procedure **********
	// find median of the first, last and middle
	k = (l + r) >> 1 ; // k is the key in the middle
	if (memcmp(key[l], key[k], keysize) > 0) {
		if (memcmp(key[r], key[l], keysize) >= 0) {
			temp = key[k] ; key[k] = key[l] ; key[l] = temp ;
			n = data[k] ; data[k] = data[l] ; data[l] = n ;
			}
		else if (memcmp(key[k], key[r], keysize) < 0) {
			temp = key[l] ; key[l] = key[k] ; key[k] = key[r] ; key[r] = temp ;
			n = data[l] ; data[l] = data[k] ; data[k] = data[r] ; data[r] = n ;
			}
		else {
			temp = key[l] ; key[l] = key[r] ; key[r] = temp ;
			n = data[l] ; data[l] = data[r] ; data[r] = n ;
			}
		}
	else if (memcmp(key[k], key[r], keysize) > 0) {
		if (memcmp(key[l], key[r], keysize) > 0) {
			temp = key[r] ; key[r] = key[k] ; key[k] = key[l] ; key[l] = temp ;
			n = data[r] ; data[r] = data[k] ; data[k] = data[l] ; data[l] = n ;
			}
		else {
			temp = key[k] ; key[k] = key[r] ; key[r] = temp ;
			n = data[k] ; data[k] = data[r] ; data[r] = n ;
			}
		}

	m = i = l + 1 ; // i scans from left
	temp = key[k] ; key[k] = key[i] ; key[i] = temp ; // switch i and k (k is the median of the first, middle and last)
	n = data[k] ; data[k] = data[i] ; data[i] = n ;
	j = r ; // j scans from right
	pivot = key[i] ;
	while (i < j) {
		++i ;
		while (memcmp(key[i], pivot, keysize) < 0) i++ ;
		--j ;
		while (memcmp(key[j], pivot, keysize) > 0) j-- ;
		temp = key[i] ; key[i] = key[j] ; key[j] = temp ;
		n = data[i] ; data[i] = data[j] ; data[j] = n ;
		}
	temp = key[j] ; key[j] = key[m] ; key[m] = key[i] ; key[i] = temp ;
	n = data[j] ; data[j] = data[m] ; data[m] = data[i] ; data[i] = n ;

	m = j - 1 ;
	k = j + 1 ;
	// sort [l,m] and [k,r]
	if ((m - l) > (r - k)) { // sort smaller (right) side ([k,k]) first -> push [l,m]
		left[stack] = l ;
		right[stack++] = m ;
		l = k ;
		goto next_quicksort_call ;
		}
	else { // left side first, push [k,r]
		left[stack] = k ;
		right[stack++] = r ;
		r = m ;
		goto next_quicksort_call ;
		}

take_next_quicksort_interval:
	if (stack < 1) {
		return ;
		}

	l = left[--stack] ;
	r = right[stack] ;
	goto next_quicksort_call ;
}


void QuickSortLong_i64(long *input_key, unsigned long len, INT64 *input_data, long left[32], long right[32])
{
	long l, r, i, j, k, m, stack, pivot ;
	INT64 n ;
	long *key ;
	INT64 *data ;

	// qualification check
	if (len < 2) return ;

	// input array starts from 0, we want the key array to start from 1.
	key = input_key - 1 ;
	data = input_data - 1 ;

	l = 1 ;
	r = len ;
	stack = 0 ;
next_quicksort_call: ;

	i = r - l ;
	if (i < 1) { // at most one key -> take next interval
		goto take_next_quicksort_interval ;
		}
	else if (i == 1) { // two keys, sort directly
		if (key[l] > key[r]) {
			j = key[l] ; key[l] = key[r] ; key[r] = j ;
			n = data[l] ; data[l] = data[r] ; data[r] = n ;
			}
		goto take_next_quicksort_interval ;
		}
	else if (i == 2) { // three keys, sort directly
		k = l + 1 ;
		if (key[l] > key[k]) {
			if (key[r] >= key[l]) { // switch l and k
				j = key[k] ; key[k] = key[l] ; key[l] = j ;
				n = data[k] ; data[k] = data[l] ; data[l] = n ;
				}
			// else : now left one is the largest
			else if (key[k] < key[r]) { // rotate left				
				j = key[l] ; key[l] = key[k] ; key[k] = key[r] ; key[r] = j ;
				n = data[l] ; data[l] = data[k] ; data[k] = data[r] ; data[r] = n ;
				}
			else { // switch l and r (middle one is at least as large as r)
				j = key[l] ; key[l] = key[r] ; key[r] = j ;
				n = data[l] ; data[l] = data[r] ; data[r] = n ;
				}
			}
		else if (key[k] > key[r]) {
			if (key[l] > key[r]) { // rotate right 
				j = key[r] ; key[r] = key[k] ; key[k] = key[l] ; key[l] = j ;
				n = data[r] ; data[r] = data[k] ; data[k] = data[l] ; data[l] = n ;
				}
			else { // switch l+1 and r
				j = key[k] ; key[k] = key[r] ; key[r] = j ;
				n = data[k] ; data[k] = data[r] ; data[r] = n ;
				}
			}
		// else everything is fine
		goto take_next_quicksort_interval ;
		}

	// more than 3 elements left
	// ********** general QuickSort procedure **********
	// find median of the first, last and middle
	k = (l + r) >> 1 ;
	if (key[l] > key[k]) {
		if (key[r] >= key[l]) {
			j = key[k] ; key[k] = key[l] ; key[l] = j ;
			n = data[k] ; data[k] = data[l] ; data[l] = n ;
			}
		else if (key[k] < key[r]) {
			j = key[l] ; key[l] = key[k] ; key[k] = key[r] ; key[r] = j ;
			n = data[l] ; data[l] = data[k] ; data[k] = data[r] ; data[r] = n ;
			}
		else {
			j = key[l] ; key[l] = key[r] ; key[r] = j ;
			n = data[l] ; data[l] = data[r] ; data[r] = n ;
			}
		}
	else if (key[k] > key[r]) {
		if (key[l] > key[r]) {
			j = key[r] ; key[r] = key[k] ; key[k] = key[l] ; key[l] = j ;
			n = data[r] ; data[r] = data[k] ; data[k] = data[l] ; data[l] = n ;
			}
		else {
			j = key[k] ; key[k] = key[r] ; key[r] = j ;
			n = data[k] ; data[k] = data[r] ; data[r] = n ;
			}
		}

	m = i = l + 1 ; // i scans from left
	j = key[k] ; key[k] = key[i] ; key[i] = j ; // switch i and k (k is the median of the first, middle and last)
	n = data[k] ; data[k] = data[i] ; data[i] = n ;
	j = r ; // j scans from right
	pivot = key[i] ;
	while (i < j) {
		++i ;
		while (key[i] < pivot) i++ ;
		--j ;
		while (key[j] > pivot) j-- ;
		k = key[i] ; key[i] = key[j] ; key[j] = k ;
		n = data[i] ; data[i] = data[j] ; data[j] = n ;
		}
	k = key[j] ; key[j] = key[m] ; key[m] = key[i] ; key[i] = k ;
	n = data[j] ; data[j] = data[m] ; data[m] = data[i] ; data[i] = n ;

	m = j - 1 ;
	k = j + 1 ;
	// sort [l,m] and [k,r]
	if ((m - l) > (r - k)) { // sort smaller (right) side ([k,k]) first -> push [l,m]
		left[stack] = l ;
		right[stack++] = m ;
		l = k ;
		goto next_quicksort_call ;
		}
	else { // left side first, push [k,r]
		left[stack] = k ;
		right[stack++] = r ;
		r = m ;
		goto next_quicksort_call ;
		}

take_next_quicksort_interval:
	if (stack < 1) {
		return ;
		}

	l = left[--stack] ;
	r = right[stack] ;
	goto next_quicksort_call ;
}


void QuickSortLong_i64_Descending(long *input_key, unsigned long len, INT64 *input_data, long left[32], long right[32])
{
	long l, r, i, j, k, m, stack, pivot ;
	long *key ;
	INT64 *data ;
	INT64 n ;

	// qualification check
	if (len < 2) return ;

	// input array starts from 0, we want the key array to start from 1.
	key = input_key - 1 ;
	data = input_data - 1 ;

	l = 1 ;
	r = len ;
	stack = 0 ;
next_quicksort_call: ;

	i = r - l ;
	if (i < 1) { // at most one key -> take next interval
		goto take_next_quicksort_interval ;
		}
	else if (i == 1) { // two keys, sort directly
		if (key[l] < key[r]) {
			j = key[l] ; key[l] = key[r] ; key[r] = j ;
			n = data[l] ; data[l] = data[r] ; data[r] = n ;
			}
		goto take_next_quicksort_interval ;
		}
	else if (i == 2) { // three keys, sort directly
		k = l + 1 ;
		if (key[l] < key[k]) {
			if (key[r] < key[l]) { // switch l and k
				j = key[k] ; key[k] = key[l] ; key[l] = j ;
				n = data[k] ; data[k] = data[l] ; data[l] = n ;
				}
			// else : now left one is the smallest
			else if (key[k] > key[r]) { // rotate left				
				j = key[l] ; key[l] = key[k] ; key[k] = key[r] ; key[r] = j ;
				n = data[l] ; data[l] = data[k] ; data[k] = data[r] ; data[r] = n ;
				}
			else { // switch l and r (middle one is at least as small as r)
				j = key[l] ; key[l] = key[r] ; key[r] = j ;
				n = data[l] ; data[l] = data[r] ; data[r] = n ;
				}
			}
		else if (key[k] < key[r]) {
			if (key[l] < key[r]) { // rotate right 
				j = key[r] ; key[r] = key[k] ; key[k] = key[l] ; key[l] = j ;
				n = data[r] ; data[r] = data[k] ; data[k] = data[l] ; data[l] = n ;
				}
			else { // switch l+1 and r
				j = key[k] ; key[k] = key[r] ; key[r] = j ;
				n = data[k] ; data[k] = data[r] ; data[r] = n ;
				}
			}
		// else everything is fine
		goto take_next_quicksort_interval ;
		}

	// more than 3 elements left
	// ********** general QuickSort procedure **********
	// find median of the first, last and middle
	k = (l + r) >> 1 ; // k is the key in the middle
	if (key[l] < key[k]) {
		if (key[r] < key[l]) {
			j = key[k] ; key[k] = key[l] ; key[l] = j ;
			n = data[k] ; data[k] = data[l] ; data[l] = n ;
			}
		else if (key[k] > key[r]) {
			j = key[l] ; key[l] = key[k] ; key[k] = key[r] ; key[r] = j ;
			n = data[l] ; data[l] = data[k] ; data[k] = data[r] ; data[r] = n ;
			}
		else {
			j = key[l] ; key[l] = key[r] ; key[r] = j ;
			n = data[l] ; data[l] = data[r] ; data[r] = n ;
			}
		}
	else if (key[k] < key[r]) {
		if (key[l] < key[r]) {
			j = key[r] ; key[r] = key[k] ; key[k] = key[l] ; key[l] = j ;
			n = data[r] ; data[r] = data[k] ; data[k] = data[l] ; data[l] = n ;
			}
		else {
			j = key[k] ; key[k] = key[r] ; key[r] = j ;
			n = data[k] ; data[k] = data[r] ; data[r] = n ;
			}
		}

	m = i = l + 1 ; // i scans from left
	j = key[k] ; key[k] = key[i] ; key[i] = j ; // switch i and k (k is the median of the first, middle and last)
	n = data[k] ; data[k] = data[i] ; data[i] = n ;
	j = r ; // j scans from right
	pivot = key[i] ;
	while (i < j) {
		++i ;
		while (key[i] > pivot) i++ ;
		--j ;
		while (key[j] < pivot) j-- ;
		k = key[i] ; key[i] = key[j] ; key[j] = k ;
		n = data[i] ; data[i] = data[j] ; data[j] = n ;
		}
	k = key[j] ; key[j] = key[m] ; key[m] = key[i] ; key[i] = k ;
	n = data[j] ; data[j] = data[m] ; data[m] = data[i] ; data[i] = n ;

	m = j - 1 ;
	k = j + 1 ;
	// sort [l,m] and [k,r]
	if ((m - l) > (r - k)) { // sort smaller (right) side ([k,k]) first -> push [l,m]
		left[stack] = l ;
		right[stack++] = m ;
		l = k ;
		goto next_quicksort_call ;
		}
	else { // left side first, push [k,r]
		left[stack] = k ;
		right[stack++] = r ;
		r = m ;
		goto next_quicksort_call ;
		}

take_next_quicksort_interval:
	if (stack < 1) {
		return ;
		}

	l = left[--stack] ;
	r = right[stack] ;
	goto next_quicksort_call ;
}


int SortChecki64(INT64 *key, unsigned long len)
{
	unsigned long i ;

	for (i = 1 ; i < len ; i++) {
		if (key[i-1] > key[i]) return 0 ;
		}
	return 1 ;
}


void QuickSorti64(INT64 *input_key, unsigned long len, long *input_data, long left[32], long right[32])
{
	INT64 temp, pivot ;
	long l, r, i, j, k, m, n, stack ;
	INT64 *key ;
	long *data ;

	// qualification check
	if (len < 2) return ;

	// input array starts from 0, we want the key array to start from 1.
	key = input_key - 1 ;
	data = input_data - 1 ;

	l = 1 ;
	r = len ;
	stack = 0 ;
next_quicksort_call: ;

	i = r - l ;
	if (i < 1) { // at most one key -> take next interval
		goto take_next_quicksort_interval ;
		}
	else if (i == 1) { // two keys, sort directly
		if (key[l] > key[r]) {
			temp = key[l] ; key[l] = key[r] ; key[r] = temp ;
			n = data[l] ; data[l] = data[r] ; data[r] = n ;
			}
		goto take_next_quicksort_interval ;
		}
	else if (i == 2) { // three keys, sort directly
		k = l + 1 ;
		if (key[l] > key[k]) {
			if (key[r] >= key[l]) { // switch l and k
				temp = key[k] ; key[k] = key[l] ; key[l] = temp ;
				n = data[k] ; data[k] = data[l] ; data[l] = n ;
				}
			// else : now left one is the largest
			else if (key[k] < key[r]) { // rotate left				
				temp = key[l] ; key[l] = key[k] ; key[k] = key[r] ; key[r] = temp ;
				n = data[l] ; data[l] = data[k] ; data[k] = data[r] ; data[r] = n ;
				}
			else { // switch l and r (middle one is at least as large as r)
				temp = key[l] ; key[l] = key[r] ; key[r] = temp ;
				n = data[l] ; data[l] = data[r] ; data[r] = n ;
				}
			}
		else if (key[k] > key[r]) {
			if (key[l] > key[r]) { // rotate right 
				temp = key[r] ; key[r] = key[k] ; key[k] = key[l] ; key[l] = temp ;
				n = data[r] ; data[r] = data[k] ; data[k] = data[l] ; data[l] = n ;
				}
			else { // switch l+1 and r
				temp = key[k] ; key[k] = key[r] ; key[r] = temp ;
				n = data[k] ; data[k] = data[r] ; data[r] = n ;
				}
			}
		// else everything is fine
		goto take_next_quicksort_interval ;
		}

	// more than 3 elements left
	// ********** general QuickSort procedure **********
	// find median of the first, last and middle
	k = (l + r) >> 1 ; // k is the key in the middle
	if (key[l] > key[k]) {
		if (key[r] >= key[l]) {
			temp = key[k] ; key[k] = key[l] ; key[l] = temp ;
			n = data[k] ; data[k] = data[l] ; data[l] = n ;
			}
		else if (key[k] < key[r]) {
			temp = key[l] ; key[l] = key[k] ; key[k] = key[r] ; key[r] = temp ;
			n = data[l] ; data[l] = data[k] ; data[k] = data[r] ; data[r] = n ;
			}
		else {
			temp = key[l] ; key[l] = key[r] ; key[r] = temp ;
			n = data[l] ; data[l] = data[r] ; data[r] = n ;
			}
		}
	else if (key[k] > key[r]) {
		if (key[l] > key[r]) {
			temp = key[r] ; key[r] = key[k] ; key[k] = key[l] ; key[l] = temp ;
			n = data[r] ; data[r] = data[k] ; data[k] = data[l] ; data[l] = n ;
			}
		else {
			temp = key[k] ; key[k] = key[r] ; key[r] = temp ;
			n = data[k] ; data[k] = data[r] ; data[r] = n ;
			}
		}

	m = i = l + 1 ; // i scans from left
	temp = key[k] ; key[k] = key[i] ; key[i] = temp ; // switch i and k (k is the median of the first, middle and last)
	n = data[k] ; data[k] = data[i] ; data[i] = n ;
	j = r ; // j scans from right
	pivot = key[i] ;
	while (i < j) {
		++i ;
		while (key[i] < pivot) i++ ;
		--j ;
		while (key[j] > pivot) j-- ;
		temp = key[i] ; key[i] = key[j] ; key[j] = temp ;
		n = data[i] ; data[i] = data[j] ; data[j] = n ;
		}
	temp = key[j] ; key[j] = key[m] ; key[m] = key[i] ; key[i] = temp ;
	n = data[j] ; data[j] = data[m] ; data[m] = data[i] ; data[i] = n ;

	m = j - 1 ;
	k = j + 1 ;
	// sort [l,m] and [k,r]
	if ((m - l) > (r - k)) { // sort smaller (right) side ([k,k]) first -> push [l,m]
		left[stack] = l ;
		right[stack++] = m ;
		l = k ;
		goto next_quicksort_call ;
		}
	else { // left side first, push [k,r]
		left[stack] = k ;
		right[stack++] = r ;
		r = m ;
		goto next_quicksort_call ;
		}

take_next_quicksort_interval:
	if (stack < 1) {
		return ;
		}

	l = left[--stack] ;
	r = right[stack] ;
	goto next_quicksort_call ;
}


